// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This example shows the implementation of a standalone process peripheral.
 * This process gets three arguments passed to it by the simulation side:
 *
 * - main socket: this socket is used for requests initiated by the emulator
 * - irq socket: this socket is used for requests initiated by this process
 * - address: the IP address of the emulator
 *
 * By connecting back to the emulator using the supplied sockets, we can then
 * communicate with the emulator side and get messages sent to us whenever
 * firmware reads or writes some memory assigned to this particular device.
 *
 * This example is intentionally kept raw and very simple - using raw unix
 * sockets. This is to show the core concept without any dependencies. Remember
 * that sockets are byte streams - so ideally there should be a packet protocol
 * on top. Here it works anyway because we are running very close to the
 * simulation and connections never break.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cstdlib>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <array>

#include <instruments/protocol/instrulink.h>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>

extern "C" {
#include "dc_motor.h"
}

#include <instruments/dcmotor.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct ScrollingBuffer {
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;
	ScrollingBuffer(int max_size = 2000)
	{
		MaxSize = max_size;
		Offset = 0;
		Data.reserve(MaxSize);
	}
	void AddPoint(float x, float y)
	{
		if (Data.size() < MaxSize) {
			Data.push_back(ImVec2(x, y));
		} else {
			Data[Offset] = ImVec2(x, y);
			Offset = (Offset + 1) % MaxSize;
		}
	}
	void Erase()
	{
		if (Data.size() > 0) {
			Data.shrink(0);
			Offset = 0;
		}
	}
};

struct device_state {
	pthread_mutex_t lock;
	struct model_dc_motor dc_motor;
	struct dcmotor_instrument data;
};

#define PLOT_SIZE 1000

struct application {
	struct instrulink *instrulink;
	struct {
		float t;
		ScrollingBuffer omega;
		ScrollingBuffer current;
		ScrollingBuffer reference;
		ScrollingBuffer control;
		ScrollingBuffer error;
	} plot;
	bool done;
	struct device_state state;
};

#define MEM_BASE 0x70000000
#define MEM_ADC1_START (MEM_BASE + 0x400)
#define MEM_ADC1_END (MEM_ADC1_START + 0x400)

void *_communication_thread(void *data)
{
	struct application *self = (struct application *)data;
	while (true) {
		struct instrulink *instrulink = self->instrulink;
		struct instrulink_packet req;
		struct instrulink_packet res;

		if (instrulink_wait_request(instrulink, &req) != 0) {
			fprintf(stderr, "Failed to receive packet\n");
			break;
		}

		pthread_mutex_lock(&self->state.lock);

		uint8_t *data = (uint8_t *)&self->state.data;
		res.type = MSG_TYPE_ERROR;

		switch (req.type) {
		case MSG_TYPE_HANDSHAKE:
			res.type = MSG_TYPE_HANDSHAKE;
			break;
		case MSG_TYPE_WRITE:
			if (req.addr == __builtin_offsetof(struct dcmotor_instrument, tick)) {
				model_dc_motor_step(&self->state.dc_motor);
			} else {
				if (req.addr >= sizeof(self->state.data)) {
					fprintf(stderr, "Error: %08x: offset out of bounds!\n",
						(uint32_t)req.addr);
				} else {
					memcpy(data + req.addr, &req.value, sizeof(uint32_t));
				}
			}
			res.type = MSG_TYPE_OK;
			break;
		case MSG_TYPE_READ:
			if (req.addr >= sizeof(self->state.data)) {
				fprintf(stderr, "Error: %08x: offset out of bounds!\n",
					(uint32_t)req.addr);
			} else {
				memcpy(&res.value, data + req.addr, sizeof(uint32_t));
			}
			res.type = MSG_TYPE_OK;
			// interrupt flags are reset when they are read
			if (req.addr == offsetof(struct dcmotor_instrument, INTF)) {
				self->state.data.INTF = 0;
			}
			break;
		case MSG_TYPE_DISCONNECT:
			exit(0);
			//pthread_mutex_unlock(&self->state.lock);
			//self->done = true;
			return NULL;
		}

		pthread_mutex_unlock(&self->state.lock);

		if (instrulink_send_response(instrulink, &res) != 0) {
			fprintf(stderr, "Failed to send packet");
			break;
		}
	}
	return NULL;
}

void implot_radial_line(const char *name, float inner_radius, float outer_radius, float angle)
{
	const float cos_angle = cosf(angle);
	const float sin_angle = sinf(angle);
	std::array<float, 2> xs = { inner_radius * cos_angle, outer_radius * cos_angle };
	std::array<float, 2> ys = { inner_radius * sin_angle, outer_radius * sin_angle };
	ImPlot::PlotLine(name, xs.data(), ys.data(), 2);
}

int main(int argc, char **argv)
{
	struct application app;
	struct application *self = &app;

	self->state.data.pid.Kp = 0.683;
	self->state.data.pid.Ki = 0.008;
	self->state.data.pid.Kd = 2.225;
	self->state.data.pid.d = 0.85;
	self->state.data.Kff = 0;

	self->state.data.lqi.L[0] = 3.338;
	self->state.data.lqi.L[1] = 3.357;
	self->state.data.lqi.Li = 0.040;

	self->plot.t = 0;
	self->plot.omega.AddPoint(0, 0);
	self->plot.current.AddPoint(0, 0);
	self->plot.reference.AddPoint(0, 0);
	self->plot.control.AddPoint(0, 0);
	self->plot.error.AddPoint(0, 0);

	model_dc_motor_init(&self->state.dc_motor);
	pthread_mutex_init(&self->state.lock, NULL);
	self->instrulink = instrulink_new();

	if (argc != 4) {
		printf("Usage: %s <mainPort> <irqPort> <address>\n", argv[0]);
		//return 1;
	} else {
		int mainPort = atoi(argv[1]);
		int irqPort = atoi(argv[2]);
		const char *address = argv[3];

		printf("Connecting to %s %d %d\n", address, mainPort, irqPort);

		if (instrulink_connect(self->instrulink, mainPort, irqPort, address) != 0) {
			fprintf(stderr, "Could not connect to instrulink (IP: %s)\n", address);
			return -1;
		}
		printf("Connected to %s %d %d\n", address, mainPort, irqPort);
	}

	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// d updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
							 SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window *window = SDL_CreateWindow("Controller example", SDL_WINDOWPOS_CENTERED,
					      SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	pthread_t thread;
	pthread_create(&thread, NULL, _communication_thread, self);
	while (!self->done) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				self->done = true;
			if (event.type == SDL_WINDOWEVENT &&
			    event.window.event == SDL_WINDOWEVENT_CLOSE &&
			    event.window.windowID == SDL_GetWindowID(window))
				self->done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		pthread_mutex_lock(&self->state.lock);

		self->state.dc_motor.u[0] = self->state.data.control;

		ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::Begin("DC Motor Simulation", NULL,
			     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 220);

		if (ImPlot::BeginPlot("##Rotor Angle", ImVec2(200, 200))) {
			ImPlot::SetupAxisLimits(ImAxis_X1, -1.0, 1.0);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0);
			ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels);
			ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoTickLabels);
			ImPlot::PushStyleColor(ImPlotCol_Line,
					       (uint32_t)ImColor(1.0f, 1.0f, 1.0f, 1.0));
			implot_radial_line("##Rotor Angle", 0.0f, 1.0f,
					   self->state.dc_motor.position);
			ImPlot::PushStyleColor(ImPlotCol_Line,
					       (uint32_t)ImColor(1.0f, 1.0f, 1.0f, 0.2));
			ImPlot::PopStyleColor(2);
			ImPlot::EndPlot();
		}

		ImGui::NextColumn();

		if (ImGui::CollapsingHeader("DC Motor simulation",
					    ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SliderFloat("Drive voltage (V)", &self->state.dc_motor.u[0], -25.0f,
					   25.0f);
			ImGui::SliderFloat("Angular velocity (Rad/sec)", &self->state.dc_motor.x[0],
					   -2.5f, 2.5f);
			ImGui::SliderFloat("Armature current (Amp)", &self->state.dc_motor.x[1],
					   -25.f, 25.f);
			ImGui::SliderFloat("Rotor position (Rad)", &self->state.dc_motor.position,
					   -M_PI, M_PI);
			ImGui::SliderFloat("Rotor inertia (J) (kg.m^2)", &self->state.dc_motor.J,
					   0.0f, 0.25f);
			ImGui::SliderFloat("Motor viscous friction constant (b) (N.m.s)",
					   &self->state.dc_motor.b, 0.0f, 0.2f);
			ImGui::SliderFloat("Electromotiva force (Ke) (V/rad/sec)",
					   &self->state.dc_motor.K, 0.0f, 2.5f);
			ImGui::SliderFloat("Torque constant (Ki) (N.m/Amp)",
					   &self->state.dc_motor.K, 0.0f, 0.5f);
			ImGui::SliderFloat("Electrical resistance (R) (Ohm)",
					   &self->state.dc_motor.R, 0.0f, 15.f);
			ImGui::SliderFloat("Electrical inductance (L) (Henry)",
					   &self->state.dc_motor.L, 0.0f, 5.0f);
		}

		if (ImGui::CollapsingHeader("Controller", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char *controllers[] = { "PID Controller", "LQI Controller" };
			static const char *controller = NULL;

			if (!controller)
				controller = controllers[0];

			if (ImGui::BeginCombo("##combo", controller)) {
				for (int n = 0; n < IM_ARRAYSIZE(controllers); n++) {
					bool is_selected = (controller == controllers[n]);
					if (ImGui::Selectable(controllers[n], is_selected))
						controller = controllers[n];
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (controller == controllers[0]) {
				self->state.data.controller = 0;
				ImGui::SliderFloat("Proportional gain (Kp)",
						   &self->state.data.pid.Kp, 0.f, 10.f);
				ImGui::SliderFloat("Integral gain (Ki)", &self->state.data.pid.Ki,
						   0.f, 1.f);
				ImGui::SliderFloat("Derivative gain (Kd)", &self->state.data.pid.Kd,
						   0.f, 100.0f);
				ImGui::SliderFloat("Derivative filter pole (d)",
						   &self->state.data.pid.d, 0.f, 1.f);
			} else {
				self->state.data.controller = 1;
				ImGui::SliderFloat("Angular velocity error gain (L[0])",
						   &self->state.data.lqi.L[0], 0.f, 5.f);
				ImGui::SliderFloat("Current gain (L[1])",
						   &self->state.data.lqi.L[1], 0.f, 5.f);
				ImGui::SliderFloat("Setpoing integral gain (Li)",
						   &self->state.data.lqi.Li, 0.f, 1.0f);
			}
			ImGui::SliderFloat("Feedforward gain (Kff)", &self->state.data.Kff, 0.f,
					   10.f);
			ImGui::SliderFloat("Control action voltage (u)", &self->state.data.control,
					   -25.f, 25.f);
			ImGui::SliderFloat("Reference angular velocity (r)",
					   &self->state.data.reference, -2.4f, 2.4f);
		}

		ImGui::Columns(1);

		if (ImPlot::BeginPlot("Motor output")) {
			ImPlot::SetupAxisLimits(ImAxis_X1, self->plot.t - 2000.0, self->plot.t,
						ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -5, 5);
			ImPlot::PlotLine("Omega (w)", &self->plot.omega.Data[0].x,
					 &self->plot.omega.Data[0].y, self->plot.omega.Data.size(),
					 0, self->plot.omega.Offset, 2 * sizeof(float));
			ImPlot::PlotLine("Current (I)", &self->plot.current.Data[0].x,
					 &self->plot.current.Data[0].y,
					 self->plot.current.Data.size(), 0,
					 self->plot.current.Offset, 2 * sizeof(float));
			ImPlot::PlotLine("Reference (w)", &self->plot.reference.Data[0].x,
					 &self->plot.reference.Data[0].y,
					 self->plot.reference.Data.size(), 0,
					 self->plot.reference.Offset, 2 * sizeof(float));
			ImPlot::PlotLine("Control action (u)", &self->plot.control.Data[0].x,
					 &self->plot.control.Data[0].y,
					 self->plot.control.Data.size(), 0,
					 self->plot.control.Offset, 2 * sizeof(float));
			ImPlot::EndPlot();
		}
		if (ImPlot::BeginPlot("Controller error")) {
			ImPlot::SetupAxisLimits(ImAxis_X1, self->plot.t - 2000.0, self->plot.t,
						ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -5, 5);
			ImPlot::PlotLine("Control error (e)", &self->plot.error.Data[0].x,
					 &self->plot.error.Data[0].y, self->plot.error.Data.size(),
					 0, self->plot.error.Offset, 2 * sizeof(float));
			ImPlot::EndPlot();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);

		self->state.data.omega = self->state.dc_motor.y[0];

		float t = self->plot.t++;

		self->plot.omega.AddPoint(t, self->state.data.omega);
		self->plot.current.AddPoint(t, self->state.dc_motor.x[1]);
		self->plot.reference.AddPoint(t, self->state.data.reference);
		self->plot.control.AddPoint(t, self->state.data.control);
		self->plot.error.AddPoint(t, self->state.data.reference - self->state.data.omega);

		pthread_mutex_unlock(&self->state.lock);

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
			     clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
		/*
		printf("float A[ADIM * ADIM] = { %f, %f, %f, %f };\n",
						self->state.dc_motor.A[0],
						self->state.dc_motor.A[1],
						self->state.dc_motor.A[2],
						self->state.dc_motor.A[3]
						);
		printf("float B[ADIM * RDIM] = { %f, %f };\n",
						self->state.dc_motor.B[0],
						self->state.dc_motor.B[1]
						);
		printf("float C[YDIM * ADIM] = { %f, %f };\n",
						self->state.dc_motor.C[0],
						self->state.dc_motor.C[1]
						);
		printf("float D[YDIM * RDIM] = { %f };\n",
						self->state.dc_motor.D[0]
						);
*/
	}

	instrulink_disconnect(self->instrulink);
	instrulink_free(&self->instrulink);

	ImPlot::DestroyContext();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

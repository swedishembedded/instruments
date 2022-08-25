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

struct device_state {
	pthread_mutex_t lock;
	struct virtual_device data;
};

#define PLOT_SIZE 1000

struct application {
	struct instrulink *instrulink;
	struct device_state state;
	struct {
		float time[PLOT_SIZE];
		float omega[PLOT_SIZE];
		float reference[PLOT_SIZE];
		unsigned int x;
	} plot;
	bool done;
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
			if (req.addr >= MEM_ADC1_START && req.addr <= MEM_ADC1_END) {
				memcpy((uint8_t *)(&self->state.data.adc1) + req.addr -
					       MEM_ADC1_START,
				       &req.value, sizeof(uint32_t));
			} else {
				memcpy(data + req.addr, &req.value, sizeof(uint32_t));
			}
			res.type = MSG_TYPE_OK;
			break;
		case MSG_TYPE_READ:
			if (req.addr >= MEM_ADC1_START && req.addr <= MEM_ADC1_END) {
				memcpy(&res.value,
				       (uint8_t *)(&self->state.data.adc1) + req.addr -
					       MEM_ADC1_START,
				       sizeof(uint32_t));
			} else {
				memcpy(&res.value, data + req.addr, sizeof(uint32_t));
			}
			res.type = MSG_TYPE_OK;
			// interrupt flags are reset when they are read
			if (req.addr == offsetof(struct virtual_device, INTF)) {
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

void implot_radial_line(const char* name, float inner_radius,
                        float outer_radius, float angle) {
    const float cos_angle = cosf(angle);
    const float sin_angle = sinf(angle);
    std::array<float, 2> xs = {inner_radius * cos_angle,
                               outer_radius * cos_angle};
    std::array<float, 2> ys = {inner_radius * sin_angle,
                               outer_radius * sin_angle};
    ImPlot::PlotLine(name, xs.data(), ys.data(), 2);
}

int main(int argc, char **argv)
{
	if (argc != 4) {
		printf("Usage: %s <mainPort> <irqPort> <address>\n", argv[0]);
		return 1;
	}
	int mainPort = atoi(argv[1]);
	int irqPort = atoi(argv[2]);
	const char *address = argv[3];
	struct application app;
	struct application *self = &app;
	struct model_dc_motor dc_motor;

	memset(&app, 0, sizeof(app));

	self->state.data.Kp = 1;
	self->state.data.Ki = 1;
	self->state.data.Kd = 1;
	self->state.data.d = 0.85;
	self->state.data.Kff = 0;

	for (unsigned int c = 0; c < PLOT_SIZE; c++) {
		self->plot.time[c] = c;
	}

	model_dc_motor_init(&dc_motor);
	pthread_mutex_init(&self->state.lock, NULL);
	self->instrulink = instrulink_new();

	printf("Connecting to %s %d %d\n", address, mainPort, irqPort);

	if (instrulink_connect(self->instrulink, mainPort, irqPort, address) != 0) {
		fprintf(stderr, "Could not connect to instrulink (IP: %s)\n", address);
		return -1;
	}

	printf("Connected to %s %d %d\n", address, mainPort, irqPort);
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
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

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

		dc_motor.u[0] = self->state.data.control;
		model_dc_motor_step(&dc_motor);

		ImGui::Begin("DC Motor Simulation", NULL, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("DC Motor Simulation");
		ImGui::PushItemWidth(400);

		ImGui::SliderFloat("Drive voltage (V)", &dc_motor.u[0], -25.0f, 25.0f);
		ImGui::SliderFloat("Angular velocity (Rad/sec)", &dc_motor.x[0], -25.f, 25.f);
		ImGui::SliderFloat("Armature current (Amp)", &dc_motor.x[1], -25.f, 25.f);
		ImGui::SliderFloat("Rotor position (Rad)", &dc_motor.position, -M_PI, M_PI);
		ImGui::SliderFloat("Rotor inertia (J) (kg.m^2)", &dc_motor.J, -2.5f, 2.5f);
		ImGui::SliderFloat("Motor viscous friction constant (b) (N.m.s)", &dc_motor.b,
				   -2.5f, 2.5f);
		ImGui::SliderFloat("Electromotiva force (Ke) (V/rad/sec)", &dc_motor.K, -2.5f,
				   2.5f);
		ImGui::SliderFloat("Torque constant (Ki) (N.m/Amp)", &dc_motor.K, -2.5f, 2.5f);
		ImGui::SliderFloat("Electrical resistance (R) (Ohm)", &dc_motor.R, -2.5f, 2.5f);
		ImGui::SliderFloat("Electrical inductance (L) (Henry)", &dc_motor.L, -2.5f, 2.5f);

		/*
		uint32_t u32_zero = 0;
		uint32_t u32_max = 0x7FFFFFFF;
		uint32_t u16_max = 0xFFFF;
		ImGui::SliderScalar("ARR", ImGuiDataType_U32, &self->state.data.tim1.ARR, &u32_zero,
				    &u32_max, "0x%08X");
		ImGui::SliderScalar("ADC", ImGuiDataType_U32, &self->state.data.adc1.DR, &u32_zero,
				    &u16_max, "0x%04X");
		*/

		ImGui::PopItemWidth();

		ImGui::SetNextItemWidth(100);
		ImGui::PushItemWidth(100);
		if (ImPlot::BeginPlot("##Rotor Angle")){
			ImPlot::SetupAxisLimits(ImAxis_X1, -1.0, 1.0);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0);
			ImPlot::PushStyleColor(ImPlotCol_Line,
					       (uint32_t)ImColor(1.0f, 1.0f, 1.0f, 1.0));
			implot_radial_line("Rotor Angle", 0.0f, 1.0f, dc_motor.position);
			ImPlot::PushStyleColor(ImPlotCol_Line,
					       (uint32_t)ImColor(1.0f, 1.0f, 1.0f, 0.2));
			//ImPlot::PlotLine("Rotor Circle", viz_data.circle_xs.data(),
			//		 viz_data.circle_ys.data(), viz_data.circle_xs.size());
			ImPlot::PopStyleColor(2);
			ImPlot::EndPlot();
		}
		ImGui::PopItemWidth();

		if (ImPlot::BeginPlot("Sqrt")) {
			ImPlot::SetupAxis(ImAxis_X1, "Sample");
			ImPlot::SetupAxisLimits(ImAxis_X1, 0, PLOT_SIZE);
			ImPlot::SetupAxis(ImAxis_Y1, "Action");
			ImPlot::SetupAxisLimits(ImAxis_X1, -10, 50);
			ImPlot::PlotLine("##data", self->plot.time, self->plot.omega, PLOT_SIZE);
			ImPlot::PlotLine("##data", self->plot.time, self->plot.reference,
					 PLOT_SIZE);
			ImPlot::EndPlot();
		}

		ImGui::End();

		ImGui::Begin("DC Motor Controller", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::SliderFloat("Proportional gain (Kp)", &self->state.data.Kp, 0.f, 200.f);
		ImGui::SliderFloat("Integral gain (Ki)", &self->state.data.Ki, 0.f, 250.f);
		ImGui::SliderFloat("Derivative gain (Kd)", &self->state.data.Kd, 0.f, 25.f);
		ImGui::SliderFloat("Derivative filter pole (d)", &self->state.data.d, 0.f, 1.f);
		ImGui::SliderFloat("Feedforward gain (Kff)", &self->state.data.Kff, 0.f, 10.f);
		ImGui::SliderFloat("Control action (u)", &self->state.data.control, -250.f, 250.f);
		ImGui::SliderFloat("Target (r)", &self->state.data.reference, -25.f, 25.f);
		ImGui::End();

		self->state.data.omega = dc_motor.y[0];

		self->plot.omega[self->plot.x] = self->state.data.omega;
		self->plot.reference[self->plot.x] = self->state.data.reference;
		self->plot.x = (self->plot.x + 1) % PLOT_SIZE;

		pthread_mutex_unlock(&self->state.lock);

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
			     clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
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

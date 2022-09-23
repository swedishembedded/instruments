// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This is a memory view instrument for debugging memory access.
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
#include "InstrumentContainer.h"

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>

InstrumentContainer::InstrumentContainer()
{
	pthread_mutex_init(&this->lock, NULL);
	this->instrulink = instrulink_new();
}

void *_communication_thread(void *data)
{
	InstrumentContainer *self = (InstrumentContainer *)data;

	while (self->isRunning()) {
		if (self->handleBusRequests() != 0) {
			break;
		}
	}
	return NULL;
}

void InstrumentContainer::emitIRQ()
{
	instrulink_irq_notify(this->instrulink);
}

int InstrumentContainer::addInstrument(IInstrument *i)
{
	i->onIRQ(std::bind(&InstrumentContainer::emitIRQ, this));
	this->instruments.push_back(i);
	return 0;
}

int InstrumentContainer::write32(uint64_t addr, uint64_t value)
{
	int ret = -EIO;
	for (auto i : instruments) {
		if (i->write32(addr, value) == 0) {
			ret = 0;
		}
	}
	return ret;
}

int InstrumentContainer::write16(uint64_t addr, uint64_t value)
{
	int ret = -EIO;
	for (auto i : instruments) {
		if (i->write16(addr, value) == 0) {
			ret = 0;
		}
	}
	return ret;
}

int InstrumentContainer::write8(uint64_t addr, uint64_t value)
{
	int ret = -EIO;
	for (auto i : instruments) {
		if (i->write8(addr, value) == 0) {
			ret = 0;
		}
	}
	return ret;
}

int InstrumentContainer::read32(uint64_t addr, uint64_t *value)
{
	int ret = -EIO;
	*value = ~0;
	for (auto i : instruments) {
		uint64_t tmp;
		if (i->read32(addr, &tmp) == 0) {
			*value &= tmp;
			ret = 0;
		}
	}
	return ret;
}

int InstrumentContainer::read16(uint64_t addr, uint64_t *value)
{
	int ret = -EIO;
	*value = ~0;
	for (auto i : instruments) {
		uint64_t tmp;
		if (i->read16(addr, &tmp) == 0) {
			*value &= tmp;
			ret = 0;
		}
	}
	return ret;
}

int InstrumentContainer::read8(uint64_t addr, uint64_t *value)
{
	int ret = -EIO;
	*value = ~0;
	for (auto i : instruments) {
		uint64_t tmp;
		if (i->read8(addr, &tmp) == 0) {
			*value &= tmp;
			ret = 0;
		}
	}
	return ret;
}

int InstrumentContainer::handleBusRequests()
{
	struct instrulink *instrulink = this->instrulink;
	struct instrulink_packet req;
	struct instrulink_packet res;

	if (instrulink_wait_request(instrulink, &req) != 0) {
		fprintf(stderr, "Failed to receive packet\n");
		return -EIO;
	}

	pthread_mutex_lock(&this->lock);

	uint64_t value = ~0;
	res.value = ~0;
	res.type = MSG_TYPE_ERROR;

	switch (req.type) {
	case MSG_TYPE_HANDSHAKE:
		res.type = MSG_TYPE_HANDSHAKE;
		break;
	case MSG_TYPE_WRITE8:
		if (write8(req.addr, req.value) != 0) {
			fprintf(stderr, "Error: failed write8 to %08x\n", (uint32_t)req.addr);
		} else {
			res.type = MSG_TYPE_OK;
		}
		break;
	case MSG_TYPE_WRITE16:
		if (write16(req.addr, req.value) != 0) {
			fprintf(stderr, "Error: failed write8 to %08x\n", (uint32_t)req.addr);
		} else {
			res.type = MSG_TYPE_OK;
		}
		break;
	case MSG_TYPE_WRITE32:
		if (write32(req.addr, req.value) != 0) {
			fprintf(stderr, "Error: failed write8 to %08x\n", (uint32_t)req.addr);
		} else {
			res.type = MSG_TYPE_OK;
		}
		break;
	case MSG_TYPE_READ8:
		if (read8(req.addr, &value) != 0) {
			fprintf(stderr, "Error: failed read8 from %08x\n", (uint32_t)req.addr);
		} else {
			res.value &= ~value;
			res.type = MSG_TYPE_OK;
		}
		break;
	case MSG_TYPE_READ16:
		if (read16(req.addr, &value) != 0) {
			fprintf(stderr, "Error: failed read16 from %08x\n", (uint32_t)req.addr);
		} else {
			res.value &= value;
			res.type = MSG_TYPE_OK;
		}
		break;
	case MSG_TYPE_READ32:
		if (read32(req.addr, &value) != 0) {
			fprintf(stderr, "Error: failed read32 from %08x\n", (uint32_t)req.addr);
		} else {
			res.value &= value;
			res.type = MSG_TYPE_OK;
		}
		break;
	case MSG_TYPE_DISCONNECT:
		this->is_running = false;
	}

	pthread_mutex_unlock(&this->lock);

	if (instrulink_send_response(instrulink, &res) != 0) {
		fprintf(stderr, "Failed to send packet");
		return -EIO;
	}
	return 0;
}

bool InstrumentContainer::isRunning()
{
	return this->is_running;
}

int InstrumentContainer::init(int argc, char **argv)
{
	if (argc != 4) {
		printf("Usage: %s <mainPort> <irqPort> <address>\n", argv[0]);
		return -1;
	} else {
		int mainPort = atoi(argv[1]);
		int irqPort = atoi(argv[2]);
		const char *address = argv[3];

		printf("Connecting to %s %d %d\n", address, mainPort, irqPort);

		if (instrulink_connect(this->instrulink, mainPort, irqPort, address) != 0) {
			fprintf(stderr, "Could not connect to instrulink (IP: %s)\n", address);
			return -1;
		}
		printf("Connected to %s %d %d\n", address, mainPort, irqPort);
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// GL 3.0 + GLSL 130
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	return 0;
}

int InstrumentContainer::show()
{
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
	ImGui_ImplOpenGL3_Init("#version 130");
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	this->is_running = true;

	pthread_t thread;
	pthread_create(&thread, NULL, _communication_thread, this);
	while (this->is_running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT) {
				this->is_running = false;
			}
			if (event.type == SDL_WINDOWEVENT &&
			    event.window.event == SDL_WINDOWEVENT_CLOSE &&
			    event.window.windowID == SDL_GetWindowID(window)) {
				this->is_running = false;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		// Instrument render begin
		ImGui::Begin("Memory view", NULL,
			     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

		pthread_mutex_lock(&this->lock);

		for (auto i : this->instruments) {
			i->render();
		}

		pthread_mutex_unlock(&this->lock);

		ImGui::End();
		ImGui::PopStyleVar(1);

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
			     clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	this->is_running = false;
	pthread_join(thread, NULL);

	instrulink_disconnect(this->instrulink);
	instrulink_free(&this->instrulink);

	ImPlot::DestroyContext();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

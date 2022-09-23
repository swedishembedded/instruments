// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This is a memory view instrument for debugging memory access.
 **/

#include "DCMotorInstrument.h"

#include <cstdio>
#include <implot.h>
#include <math.h>

void implot_radial_line(const char *name, float inner_radius, float outer_radius, float angle)
{
	const float cos_angle = cosf(angle);
	const float sin_angle = sinf(angle);
	std::array<float, 2> xs = { inner_radius * cos_angle, outer_radius * cos_angle };
	std::array<float, 2> ys = { inner_radius * sin_angle, outer_radius * sin_angle };
	ImPlot::PlotLine(name, xs.data(), ys.data(), 2);
}

DCMotorInstrument::DCMotorInstrument()
{
	memset(&this->regs, 0, sizeof(this->regs));
	this->regs.pid.Kp = 0.683;
	this->regs.pid.Ki = 0.008;
	this->regs.pid.Kd = 2.225;
	this->regs.pid.d = 0.85;
	this->regs.Kff = 0;

	this->regs.lqi.L[0] = 3.338;
	this->regs.lqi.L[1] = 3.357;
	this->regs.lqi.Li = 0.040;

	this->plot.t = 0;
	this->plot.omega.AddPoint(0, 0);
	this->plot.current.AddPoint(0, 0);
	this->plot.reference.AddPoint(0, 0);
	this->plot.control.AddPoint(0, 0);
	this->plot.error.AddPoint(0, 0);

	model_dc_motor_init(&this->dc_motor);
}

int DCMotorInstrument::read32(uint64_t addr, uint64_t *data)
{
	if (addr == __builtin_offsetof(struct dcmotor_instrument, INTF)) {
		BaseInstrument::read<uint32_t>(addr, data);
		// reading interrupt flag register resets it
		this->regs.INTF = 0;
		return 0;
	}
	return BaseInstrument::read<uint32_t>(addr, data);
}

int DCMotorInstrument::write32(uint64_t addr, uint64_t data)
{
	if (addr == __builtin_offsetof(struct dcmotor_instrument, tick)) {
		model_dc_motor_step(&this->dc_motor);
		return 0;
	}
	return BaseInstrument::write<uint32_t>(addr, data);
}

void DCMotorInstrument::render()
{
	ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	this->dc_motor.u[0] = this->regs.control;

	ImGui::Begin("DC Motor Simulation", NULL,
		     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 220);

	if (ImPlot::BeginPlot("##Rotor Angle", ImVec2(200, 200))) {
		ImPlot::SetupAxisLimits(ImAxis_X1, -1.0, 1.0);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0);
		ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels);
		ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoTickLabels);
		ImPlot::PushStyleColor(ImPlotCol_Line, (uint32_t)ImColor(1.0f, 1.0f, 1.0f, 1.0));
		implot_radial_line("##Rotor Angle", 0.0f, 1.0f, this->dc_motor.position);
		ImPlot::PushStyleColor(ImPlotCol_Line, (uint32_t)ImColor(1.0f, 1.0f, 1.0f, 0.2));
		ImPlot::PopStyleColor(2);
		ImPlot::EndPlot();
	}

	ImGui::NextColumn();

	if (ImGui::CollapsingHeader("DC Motor simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Drive voltage (V)", &this->dc_motor.u[0], -25.0f, 25.0f);
		ImGui::SliderFloat("Angular velocity (Rad/sec)", &this->dc_motor.x[0], -2.5f, 2.5f);
		ImGui::SliderFloat("Armature current (Amp)", &this->dc_motor.x[1], -25.f, 25.f);
		ImGui::SliderFloat("Rotor position (Rad)", &this->dc_motor.position, -M_PI, M_PI);
		ImGui::SliderFloat("Rotor inertia (J) (kg.m^2)", &this->dc_motor.J, 0.0f, 0.25f);
		ImGui::SliderFloat("Motor viscous friction constant (b) (N.m.s)", &this->dc_motor.b,
				   0.0f, 0.2f);
		ImGui::SliderFloat("Electromotiva force (Ke) (V/rad/sec)", &this->dc_motor.K, 0.0f,
				   2.5f);
		ImGui::SliderFloat("Torque constant (Ki) (N.m/Amp)", &this->dc_motor.K, 0.0f, 0.5f);
		ImGui::SliderFloat("Electrical resistance (R) (Ohm)", &this->dc_motor.R, 0.0f,
				   15.f);
		ImGui::SliderFloat("Electrical inductance (L) (Henry)", &this->dc_motor.L, 0.0f,
				   5.0f);
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
			this->regs.controller = 0;
			ImGui::SliderFloat("Proportional gain (Kp)", &this->regs.pid.Kp, 0.f, 10.f);
			ImGui::SliderFloat("Integral gain (Ki)", &this->regs.pid.Ki, 0.f, 1.f);
			ImGui::SliderFloat("Derivative gain (Kd)", &this->regs.pid.Kd, 0.f, 100.0f);
			ImGui::SliderFloat("Derivative filter pole (d)", &this->regs.pid.d, 0.f,
					   1.f);
		} else {
			this->regs.controller = 1;
			ImGui::SliderFloat("Angular velocity error gain (L[0])",
					   &this->regs.lqi.L[0], 0.f, 5.f);
			ImGui::SliderFloat("Current gain (L[1])", &this->regs.lqi.L[1], 0.f, 5.f);
			ImGui::SliderFloat("Setpoing integral gain (Li)", &this->regs.lqi.Li, 0.f,
					   1.0f);
		}
		ImGui::SliderFloat("Feedforward gain (Kff)", &this->regs.Kff, 0.f, 10.f);
		ImGui::SliderFloat("Control action voltage (u)", &this->regs.control, -25.f, 25.f);
		ImGui::SliderFloat("Reference angular velocity (r)", &this->regs.reference, -2.4f,
				   2.4f);
	}

	ImGui::Columns(1);

	if (ImPlot::BeginPlot("Motor output")) {
		ImPlot::SetupAxisLimits(ImAxis_X1, this->plot.t - 2000.0, this->plot.t,
					ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -5, 5);
		ImPlot::PlotLine("Omega (w)", &this->plot.omega.Data[0].x,
				 &this->plot.omega.Data[0].y, this->plot.omega.Data.size(), 0,
				 this->plot.omega.Offset, 2 * sizeof(float));
		ImPlot::PlotLine("Current (I)", &this->plot.current.Data[0].x,
				 &this->plot.current.Data[0].y, this->plot.current.Data.size(), 0,
				 this->plot.current.Offset, 2 * sizeof(float));
		ImPlot::PlotLine("Reference (w)", &this->plot.reference.Data[0].x,
				 &this->plot.reference.Data[0].y, this->plot.reference.Data.size(),
				 0, this->plot.reference.Offset, 2 * sizeof(float));
		ImPlot::PlotLine("Control action (u)", &this->plot.control.Data[0].x,
				 &this->plot.control.Data[0].y, this->plot.control.Data.size(), 0,
				 this->plot.control.Offset, 2 * sizeof(float));
		ImPlot::EndPlot();
	}
	if (ImPlot::BeginPlot("Controller error")) {
		ImPlot::SetupAxisLimits(ImAxis_X1, this->plot.t - 2000.0, this->plot.t,
					ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -5, 5);
		ImPlot::PlotLine("Control error (e)", &this->plot.error.Data[0].x,
				 &this->plot.error.Data[0].y, this->plot.error.Data.size(), 0,
				 this->plot.error.Offset, 2 * sizeof(float));
		ImPlot::EndPlot();
	}

	this->regs.omega = this->dc_motor.y[0];

	float t = this->plot.t++;

	this->plot.omega.AddPoint(t, this->regs.omega);
	this->plot.current.AddPoint(t, this->dc_motor.x[1]);
	this->plot.reference.AddPoint(t, this->regs.reference);
	this->plot.control.AddPoint(t, this->regs.control);
	this->plot.error.AddPoint(t, this->regs.reference - this->regs.omega);

	ImGui::End();
	ImGui::PopStyleVar(1);
}

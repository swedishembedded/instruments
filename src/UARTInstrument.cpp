// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/

#include "UARTInstrument.h"

#include <cstdio>

UARTInstrument::UARTInstrument(std::unique_ptr<IPeripheral> uart)
{
	memset(&this->regs, 0, sizeof(this->regs));
	this->uart = std::move(uart);
	this->uart->onIRQ(std::bind(&UARTInstrument::onIRQ, this));
	this->uart->write32(0, 0xaa);
}

int UARTInstrument::write32(uint64_t addr, uint64_t data)
{
	return this->uart->write32(addr, data);
}

void UARTInstrument::onIRQ()
{
	// Currently not used
}

void UARTInstrument::render()
{
	ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	// Instrument render begin
	ImGui::Begin("UART", NULL,
		     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

	ImGui::Text("UART visualization is currently not implemented\n");

	ImGui::End();
	ImGui::PopStyleVar(1);
}

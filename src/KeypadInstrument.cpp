// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This is a memory view instrument for debugging memory access.
 **/

#include "KeypadInstrument.h"

#include <cstdio>

KeypadInstrument::KeypadInstrument()
{
	memset(&this->regs, 0, sizeof(this->regs));
}

int KeypadInstrument::read32(uint64_t addr, uint64_t *data)
{
	if (addr == __builtin_offsetof(struct keypad_instrument, keys_changed)) {
		BaseInstrument::read<uint32_t>(addr, data);
		// reading this register resets it
		this->regs.keys_changed = 0;
		return 0;
	}
	return BaseInstrument::read<uint32_t>(addr, data);
}

int KeypadInstrument::setKeyState(unsigned key, bool state)
{
	uint32_t bit = (1 << key);
	if (state && !(this->regs.keys & bit)) {
		this->regs.keys |= bit;
		this->regs.keys_changed |= bit;
	}
	if (!state && (this->regs.keys & bit)) {
		this->regs.keys &= ~bit;
		this->regs.keys_changed |= bit;
	}
	return 0;
}

void KeypadInstrument::render()
{
	ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	// Instrument render begin
	ImGui::Begin("Keypad", NULL,
		     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

	uint32_t key_state = 0;
	uint32_t prev_keys_changed = this->regs.keys_changed;

	for (unsigned int c = 1; c < 10; c++) {
		uint32_t bit = (1 << c);

		char str[16];
		snprintf(str, sizeof(str), "%d", c);

		if (ImGui::Button(str, ImVec2(40, 40))) {
			this->setKeyState(c, true);
		} else {
			this->setKeyState(c, false);
		}

		if (c != 0 && (c % 3) != 0) {
			ImGui::SameLine();
		}
	}

	if (this->regs.keys_changed != prev_keys_changed) {
		notifyIRQ();
	}

	ImGui::End();
	ImGui::PopStyleVar(1);
}

// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This is a memory view instrument for debugging memory access.
 **/

#include "instruments/keypad.h"
#include "BaseInstrument.h"

class KeypadInstrument : public BaseInstrument<struct keypad_instrument> {
    public:
	KeypadInstrument();
	void render() override;
	int read32(uint64_t addr, uint64_t *value) override;
	int setKeyState(unsigned key, bool state);
};

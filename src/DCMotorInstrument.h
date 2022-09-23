// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This is a memory view instrument for debugging memory access.
 **/

extern "C" {
#include <control/model/dc_motor.h>
}

#include "instruments/dcmotor.h"
#include "BaseInstrument.h"
#include "ScrollingBuffer.h"

class DCMotorInstrument : public BaseInstrument<struct dcmotor_instrument> {
    public:
	DCMotorInstrument();
	void render() override;
	int read32(uint64_t addr, uint64_t *value) override;
	int write32(uint64_t addr, uint64_t value) override;

    private:
	struct model_dc_motor dc_motor;
	struct dcmotor_instrument data;
	struct {
		float t;
		ScrollingBuffer omega;
		ScrollingBuffer current;
		ScrollingBuffer reference;
		ScrollingBuffer control;
		ScrollingBuffer error;
	} plot;
};

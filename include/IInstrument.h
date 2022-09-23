// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This is a memory view instrument for debugging memory access.
 **/
#pragma once

#include <functional>

class IInstrument {
    public:
	virtual ~IInstrument()
	{
	}
	virtual void render() = 0;

	virtual int write32(uint64_t addr, uint64_t data) = 0;
	virtual int write16(uint64_t addr, uint64_t data) = 0;
	virtual int write8(uint64_t addr, uint64_t data) = 0;
	virtual int read32(uint64_t addr, uint64_t *data) = 0;
	virtual int read16(uint64_t addr, uint64_t *data) = 0;
	virtual int read8(uint64_t addr, uint64_t *data) = 0;

	/** Register interrupt callback */
	virtual void onIRQ(std::function<void()>) = 0;
};

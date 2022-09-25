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

#include "IInstrument.h"
#include "instruments/keypad.h"

#include <errno.h>
#include <imgui.h>

template <typename T> class BaseInstrument : public IInstrument {
    public:
	virtual ~BaseInstrument()
	{
	}
	virtual int write32(uint64_t addr, uint64_t data)
	{
		return -ENOTSUP;
	}
	virtual int write16(uint64_t addr, uint64_t data)
	{
		return -ENOTSUP;
	}
	virtual int write8(uint64_t addr, uint64_t data)
	{
		return -ENOTSUP;
	}
	virtual int read32(uint64_t addr, uint64_t *data)
	{
		return -ENOTSUP;
	}
	virtual int read16(uint64_t addr, uint64_t *data)
	{
		return -ENOTSUP;
	}
	virtual int read8(uint64_t addr, uint64_t *data)
	{
		return -ENOTSUP;
	}
	virtual void tick() override{};
	virtual void onIRQ(std::function<void()> cb)
	{
		notifyIRQ = cb;
	}

    protected:
	template <typename W> int write(uint64_t addr, uint64_t data)
	{
		if ((addr + sizeof(W)) > sizeof(regs)) {
			return -EIO;
		}
		*((W *)((uint8_t *)&regs + addr)) = (W)data;
		return 0;
	}
	template <typename W> int read(uint64_t addr, uint64_t *data)
	{
		if ((addr + sizeof(W)) > sizeof(regs)) {
			// by default assume pullups
			*data = ~0;
			return -EIO;
		}
		*data = *((W *)((uint8_t *)&regs + addr));
		return 0;
	}
	T regs;
	std::function<void()> notifyIRQ;
};

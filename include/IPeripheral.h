// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/
/**
 * This is a peripheral interface which all verilog peripherals inherit.
 **/
#pragma once

#include <functional>

/**
 * \brief Generic peripheral interface
 * \details
 *		This interface should be implemented by any peripheral.
 *
 *		If a verilog peripheral is designed to connect to a bus (Wishbone, AXI
 *		etc) then these functions should be implemented by the bus controller
 *		and then it is the bus controller that in turn communicates with the
 *		peripheral and to the instrumentation the bus controller then
 *		represents the connected peripheral.
 **/
class IPeripheral {
    public:
	virtual ~IPeripheral()
	{
	}

	virtual void tick() = 0;

	virtual int write32(uint64_t addr, uint64_t data) = 0;
	virtual int write16(uint64_t addr, uint64_t data) = 0;
	virtual int write8(uint64_t addr, uint64_t data) = 0;
	virtual int read32(uint64_t addr, uint64_t *data) = 0;
	virtual int read16(uint64_t addr, uint64_t *data) = 0;
	virtual int read8(uint64_t addr, uint64_t *data) = 0;

	/** Register interrupt callback */
	virtual void onIRQ(std::function<void()>) = 0;
};

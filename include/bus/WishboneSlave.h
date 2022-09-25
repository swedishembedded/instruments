// SPDX-License-Identifier: MIT
// Copyright (c) 2010-2022 Antmicro
// Copyright (c) 2022 Martin Schröder <info@swedishembedded.com>
//
// This file is licensed under the MIT License.
// Full license text is available in 'licenses/MIT.txt'.
//
#pragma once
#include "IPeripheral.h"
#include "InternalBus.h"

template <typename T> class WishboneSlave : public IPeripheral, public BaseTargetBus {
    protected:
	T *dev;
	std::function<void()> cbOnIRQ;

    public:
	WishboneSlave(T *dev)
	{
		this->dev = dev;
	}

	~WishboneSlave()
	{
	}

	virtual void tick()
	{
		tick(1);
	}

	virtual void tick(uint64_t steps)
	{
		for (uint32_t i = 0; i < steps; i++) {
			dev->wb_clk = 1;
			dev->eval();
			dev->wb_clk = 0;
			dev->eval();
		}
	}

	virtual int write(uint64_t addr, uint64_t value)
	{
		this->dev->wb_we = 1;
		this->dev->wb_sel = 0xF;
		this->dev->wb_cyc = 1;
		this->dev->wb_stb = 1;
		//  According to WishboneSlave B4 spec when using 32 bit bus with byte granularity
		//  we drop 2 youngest bits
		dev->wb_addr = addr >> 2;
		dev->wb_wr_dat = value;

		if (timeoutTick((uint8_t *)&dev->wb_ack, 1) != 0) {
			return -ETIMEDOUT;
		}

		dev->wb_stb = 0;
		dev->wb_cyc = 0;
		dev->wb_we = 0;
		dev->wb_sel = 0;

		if (timeoutTick((uint8_t *)&dev->wb_ack, 0) != 0) {
			return -ETIMEDOUT;
		}
		return 0;
	}

	virtual int read(uint64_t addr, uint64_t *value)
	{
		dev->wb_we = 0;
		dev->wb_sel = 0xF;
		dev->wb_cyc = 1;
		dev->wb_stb = 1;
		dev->wb_addr = addr >> 2;

		if (timeoutTick((uint8_t *)&dev->wb_ack, 1) != 0) {
			return -ETIMEDOUT;
		}

		uint64_t result = dev->wb_rd_dat;

		dev->wb_cyc = 0;
		dev->wb_stb = 0;
		dev->wb_sel = 0;

		if (timeoutTick((uint8_t *)&dev->wb_ack, 0) != 0) {
			return -ETIMEDOUT;
		}

		*value = result;
		return 0;
	}

	virtual void reset()
	{
		dev->wb_rst = 1;
		tick();
		dev->wb_rst = 0;
		tick();
	}

	int timeoutTick(uint8_t *signal, uint8_t value, int timeout = 20)
	{
		do {
			tick();
			timeout--;
		} while (*signal != value && timeout > 0);

		// This additional tick prevents WishboneSlave controller from reacting instantly
		// after the signal is set, as the change should be recognized after the next
		// rising edge (`tick` function returns right before the rising edge). It's only
		// an option because it may break communication with LiteX-generated IP cores.
#ifdef WISHBONE_EXTRA_WAIT_TICK
		tick();
#endif

		if (timeout == 0) {
			return -ETIMEDOUT;
		}

		return 0;
	}

    public: /* IPeripheral interface */
	virtual int write32(uint64_t addr, uint64_t data) override
	{
		return write(addr, data);
	}
	virtual int write16(uint64_t addr, uint64_t data) override
	{
		return -ENOTSUP;
	}
	virtual int write8(uint64_t addr, uint64_t data) override
	{
		return -ENOTSUP;
	}

	virtual int read32(uint64_t addr, uint64_t *data) override
	{
		return read(addr, data);
	}

	virtual int read16(uint64_t addr, uint64_t *data) override
	{
		return -ENOTSUP;
	}

	virtual int read8(uint64_t addr, uint64_t *data) override
	{
		return -ENOTSUP;
	}

	/** Register interrupt callback */
	virtual void onIRQ(std::function<void()> cb) override
	{
		cbOnIRQ = cb;
	}
};

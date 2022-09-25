//
// Copyright (c) 2010-2022 Antmicro
//
// This file is licensed under the MIT License.
// Full license text is available in 'licenses/MIT.txt'.
//
#ifndef BaseBus_H
#define BaseBus_H

#include <cstdint>

class BaseBus {
    public:
	BaseBus()
	{
	}
	virtual void tick(uint64_t steps) = 0;
	virtual int timeoutTick(uint8_t *signal, uint8_t expectedValue, int timeout) = 0;
	virtual void reset() = 0;

    protected:
};

class BaseTargetBus : public BaseBus {
    public:
	virtual int write(uint64_t addr, uint64_t value) = 0;
	virtual int read(uint64_t addr, uint64_t *value) = 0;
};

class BaseInitiatorBus : public BaseBus {
    public:
	virtual void readWord(uint64_t addr, uint8_t sel) = 0;
	virtual void writeWord(uint64_t addr, uint64_t data, uint8_t sel) = 0;
	virtual void readHandler() = 0;
	virtual void writeHandler() = 0;
	virtual void clearSignals() = 0;
	virtual bool hasSpecifiedAdress() = 0;
	virtual uint64_t getSpecifiedAdress() = 0;
};
#endif

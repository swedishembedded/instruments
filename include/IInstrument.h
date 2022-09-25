// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/
/**
 * This is an instrument interface as it appears to the simulated firmware from
 * through the memory mapped interface of the simulator.
 **/
#pragma once

#include <IPeripheral.h>

#include <functional>

class IInstrument : public IPeripheral {
    public:
	virtual ~IInstrument()
	{
	}
	/** This will be called by InstrumentContainer to render the instrument */
	virtual void render() = 0;
};

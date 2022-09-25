// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/
/**
 * This is a uart peripheral interface which all verilog peripherals inherit.
 **/
#pragma once

#include "IUARTInstrument.h"
#include "IPeripheral.h"

#include <functional>

class IUARTPeripheral {
    public:
	virtual ~IUARTPeripheral()
	{
	}
	void setInstrument(IUARTInstrument *ins)
	{
		instrument = ins;
	}

    protected:
	IUARTInstrument *instrument;
};

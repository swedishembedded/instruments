// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/
/**
 * This is an interface for calling back to instrument from a peripheral.
 **/
#pragma once

#include <functional>

class IUARTInstrument {
    public:
	virtual void onStartBit() = 0;
	virtual void onTXBit(bool value) = 0;
};

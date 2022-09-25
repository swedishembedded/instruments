// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/

#include "InstrumentContainer.h"
#include "UARTInstrument.h"
#include "LiteUART.h"

int main(int argc, char **argv)
{
	InstrumentContainer window;

	// Create a Wishbone verilog uart
	std::unique_ptr<IPeripheral> liteuart(new LiteUART());

	// Create instrumentation for visualizing its state
	UARTInstrument *ins = new UARTInstrument(std::move(liteuart));

	// Add the new instrument to the window
	window.addInstrument(ins);

	window.show();

	return 0;
}

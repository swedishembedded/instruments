// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 *
 * This example shows the implementation of a standalone process peripheral.
 * This process gets three arguments passed to it by the simulation side:
 *
 * - main socket: this socket is used for requests initiated by the emulator
 * - irq socket: this socket is used for requests initiated by this process
 * - address: the IP address of the emulator
 *
 * By connecting back to the emulator using the supplied sockets, we can then
 * communicate with the emulator side and get messages sent to us whenever
 * firmware reads or writes some memory assigned to this particular device.
 *
 * This example is intentionally kept raw and very simple - using raw unix
 * sockets. This is to show the core concept without any dependencies. Remember
 * that sockets are byte streams - so ideally there should be a packet protocol
 * on top. Here it works anyway because we are running very close to the
 * simulation and connections never break.
 **/

#include "InstrumentContainer.h"
#include "KeypadInstrument.h"

int main(int argc, char **argv)
{
	InstrumentContainer window;
	KeypadInstrument keypad;
	window.init(argc, argv);
	window.addInstrument(&keypad);
	window.show();

	return 0;
}

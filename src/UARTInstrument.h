// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/

#include "instruments/uart.h"
#include "IUARTInstrument.h"
#include "IUARTPeripheral.h"
#include "BaseInstrument.h"
#include <memory>

/**
 * \brief UART Instrumentation object
 * \details
 *		This object provides means of visually representing any UART peripheral
 *		device - simulated or otherwise.
 *
 **/
class UARTInstrument : public BaseInstrument<struct uart_instrument> {
    public:
	UARTInstrument(std::unique_ptr<IPeripheral> uart);
	void render() override;
	int write32(uint64_t addr, uint64_t value) override;
	void onIRQ();

    private:
	std::unique_ptr<IPeripheral> uart;
};

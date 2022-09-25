#pragma once

#include <memory>

#include "VLiteUART.h"
#include "bus/WishboneSlave.h"

class VLiteUART;

/**
 * \brief Wishbone UART peripheral that implements standard peripheral interface
 * \details
 *		LiteUART -> Wishbone -> Verilated UART
 **/
class LiteUART : public WishboneSlave<VLiteUART> {
    public:
	LiteUART();
	~LiteUART();
	int tx(uint8_t data);
	bool txo();
	int rx(uint8_t *data);

    private:
	/** \brief write 32 bit regsiter **/
	virtual int write32(uint64_t addr, uint64_t data) override;
	virtual int read32(uint64_t addr, uint64_t *data) override;

    private:
	uint32_t freq;
	uint32_t baud;
	uint64_t debug;
};

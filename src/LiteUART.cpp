#include "LiteUART.h"
#include "bus/WishboneSlave.h"
#include "VLiteUART.h"

#define LITEUART_REG_RXTX 0x800
#define LITEUART_REG_TXFULL 0x804
#define LITEUART_REG_RXEMPTY 0x808
#define LITEUART_REG_EV_STATUS 0x80c
#define LITEUART_REG_EV_PENDING 0x810
#define LITEUART_REG_EV_ENABLE 0x814
#define LITEUART_REG_TX_EMPTY 0x818
#define LITEUART_REG_RX_FULL 0x81c

LiteUART::LiteUART() : WishboneSlave<VLiteUART>(new VLiteUART())
{
	this->debug = 0;
	this->freq = 100000000;
	this->baud = 115200;
}

LiteUART::~LiteUART()
{
}

int LiteUART::read32(uint64_t addr, uint64_t *data)
{
	if (addr == 0) {
		printf("Instrumentation read!\n");
		*data = this->debug;
		return 0;
	}
	return WishboneSlave<VLiteUART>::read32(addr, data);
}

int LiteUART::write32(uint64_t addr, uint64_t data)
{
	return WishboneSlave<VLiteUART>::write32(addr, data);
}

bool LiteUART::txo()
{
	return this->dev->serial_tx;
}

int LiteUART::rx(uint8_t *data)
{
	return -ENOTSUP;
}

int LiteUART::tx(uint8_t data)
{
	// write to wishbone register
	this->write32(LITEUART_REG_RXTX, data);

	/*
	const int prescaler = UART_FREQ / (BAUDRATE * 8);

	// transmit start bit
	this->tick(true);
	this->tick(true, (prescaler * 8) / 2);
    this->tick(true, prescaler * 8);
    for(int i = 0; i < 8; i++) {
        printf("TXD: %08x\n", this->dev->serial_tx);

		this->debug |= (this->dev->serial_tx << i);
		this->cbOnIRQ();

        this->tick(true, prescaler * 8);
    }
    this->tick(true, prescaler * 8);
*/
	return 0;
}

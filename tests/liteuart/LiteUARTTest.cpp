#include "UARTInstrument.h"
#include "LiteUART.h"

#include <gtest/gtest.h>
#include <errno.h>
#include <stdio.h>

#define LITEUART_REG_RXTX 0x800
#define LITEUART_REG_TXFULL 0x804
#define LITEUART_REG_RXEMPTY 0x808
#define LITEUART_REG_EV_STATUS 0x80c
#define LITEUART_REG_EV_PENDING 0x810
#define LITEUART_REG_EV_ENABLE 0x814
#define LITEUART_REG_TX_EMPTY 0x818
#define LITEUART_REG_RX_FULL 0x81c

#define UART_FREQ 100000000
#define UART_BAUD 115200
#define TICKS_PER_BIT (UART_FREQ / UART_BAUD)

void test_tx_data(uint8_t data)
{
	std::unique_ptr<LiteUART> uart(new LiteUART());

	uart->tick();

	// write a byte to the tx register
	uart->tx(data);

	// tick it until middle of start bit
	uart->tick(TICKS_PER_BIT / 2);
	EXPECT_EQ(0, uart->txo());

	// check data transmission
	for (int i = 0; i < 8; i++) {
		uart->tick(TICKS_PER_BIT);
		EXPECT_EQ((data >> i) & 1, uart->txo());
	}

	// check stop bit
	uart->tick(TICKS_PER_BIT);
	EXPECT_EQ(1, uart->txo());
}

TEST(Test, UartShouldTransmitAAData)
{
	test_tx_data(0xaa);
	test_tx_data(0xaa);
	test_tx_data(0xaa);
}

TEST(Test, UartShouldTransmit00Data)
{
	test_tx_data(0x00);
	test_tx_data(0x00);
	test_tx_data(0x00);
}

TEST(Test, UartShouldTransmitFFData)
{
	test_tx_data(0xff);
	test_tx_data(0xff);
	test_tx_data(0xff);
}

TEST(Test, UnsupportedReadsShouldReturnENOTSUP)
{
	uint64_t value = 0;
	std::unique_ptr<LiteUART> liteuart(new LiteUART());
	UARTInstrument ins(std::move(liteuart));

	EXPECT_EQ(-ENOTSUP, ins.read8(0, &value));
	EXPECT_EQ(-ENOTSUP, ins.read16(0, &value));
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

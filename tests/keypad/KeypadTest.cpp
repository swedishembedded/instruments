#include "KeypadInstrument.h"

#include <gtest/gtest.h>
#include <errno.h>
#include <stdio.h>

TEST(Test, RegistersShouldBeReadableAndZeroByDefault)
{
	uint64_t value = 0;
	KeypadInstrument ins;
	EXPECT_EQ(0, ins.read32(KEYPAD_REG_KEYS, &value));
	EXPECT_EQ(0, value);
	EXPECT_EQ(0, ins.read32(KEYPAD_REG_KEYS_CHANGED, &value));
	EXPECT_EQ(0, value);
}

TEST(Test, InvalidReadsShouldReturnEIO)
{
	uint64_t value = 0;
	KeypadInstrument ins;
	EXPECT_EQ(-EIO, ins.read32(sizeof(struct keypad_instrument), &value));
}

TEST(Test, UnsupportedReadsShouldReturnENOTSUP)
{
	uint64_t value = 0;
	KeypadInstrument ins;

	EXPECT_EQ(-ENOTSUP, ins.read8(0, &value));
	EXPECT_EQ(-ENOTSUP, ins.read16(0, &value));
}

TEST(Test, RegistersShouldReflectKeyPress)
{
	KeypadInstrument ins;
	uint64_t value = 0;
	for (unsigned int c = 0; c < 10; c++) {
		// first time we read changed register it should be zero
		EXPECT_EQ(0, ins.read32(KEYPAD_REG_KEYS_CHANGED, &value));
		EXPECT_EQ(0, value);
		// press a key
		ins.setKeyState(c, true);
		EXPECT_EQ(0, ins.read32(KEYPAD_REG_KEYS, &value));
		EXPECT_EQ((1 << c), value);
		EXPECT_EQ(0, ins.read32(KEYPAD_REG_KEYS_CHANGED, &value));
		EXPECT_EQ((1 << c), value);
		// release a key
		ins.setKeyState(c, false);
		EXPECT_EQ(0, ins.read32(KEYPAD_REG_KEYS, &value));
		EXPECT_EQ(0, value);
		EXPECT_EQ(0, ins.read32(KEYPAD_REG_KEYS_CHANGED, &value));
		EXPECT_EQ((1 << c), value);
	}
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

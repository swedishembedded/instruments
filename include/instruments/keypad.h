/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 */

#pragma once

#include <stdint.h>

enum {
	KEYPAD_REG_KEYS = 0,
	KEYPAD_REG_KEYS_CHANGED = 4,
};

/** This defines our virtual device */
struct keypad_instrument {
	/** Keypad key state */
	uint32_t keys;
	/** Key state change register (reading resets value) */
	uint32_t keys_changed;
} __attribute__((packed)) __attribute__((aligned(4)));

/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 */

#pragma once

#include <stdint.h>

/** This defines our virtual device */
struct uart_instrument {
	/** Dummy register for now */
	uint32_t zero;
} __attribute__((packed)) __attribute__((aligned(4)));

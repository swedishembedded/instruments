/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/consulting
 * Training: https://swedishembedded.com/tag/training
 */

#pragma once

#include <stdint.h>

/** Instrulink packet */
struct instrulink_packet {
	/** Packet type (instrulink_message_type) */
	uint32_t type;
	/** Address for current operation (if applicable) */
	uint64_t addr;
	/** Value for current operation (if applicable) */
	uint64_t value;
} __attribute__((packed, aligned(1)));

/** Instrulink packet type codes */
enum instrulink_message_type {
	/** Invalid message */
	MSG_TYPE_INVALID = 0,
	/** Standard "tick" message */
	MSG_TYPE_TICK_CLOCK = 1,
	/** Write value to address */
	MSG_TYPE_WRITE32 = 2,
	/** Read value from address */
	MSG_TYPE_READ32 = 3,
	/** Reset the device */
	MSG_TYPE_RESET = 4,
	/** Interrupt request */
	MSG_TYPE_IRQ = 5,
	/** Error response */
	MSG_TYPE_ERROR = 6,
	/** Success response */
	MSG_TYPE_OK = 7,
	/** Disconnect from socket */
	MSG_TYPE_DISCONNECT = 8,
	/** Handshake (first message sent by master) */
	MSG_TYPE_HANDSHAKE = 9,
	/** Write short */
	MSG_TYPE_WRITE16 = 10,
	/** Read short */
	MSG_TYPE_READ16 = 11,
	/** Write byte */
	MSG_TYPE_WRITE8 = 12,
	/** Read byte */
	MSG_TYPE_READ8 = 13,
};

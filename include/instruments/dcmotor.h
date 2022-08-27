/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 */

#pragma once

#include <stdint.h>

/** This defines our virtual device */
struct dcmotor_instrument {
	/** Instrument controller selection */
	uint32_t controller;
	/** LQI regulator parameters */
	struct {
		/** LQI controller L gain */
		float L[2];
		/** LQI controller integrator gain */
		float Li;
	} lqi;
	/** PID controller parameters */
	struct {
		/** Proportional gain */
		float Kp;
		/** Integral gain */
		float Ki;
		/** Derivative gain */
		float Kd;
		/** Derivative filter pole (0..1) */
		float d;
	} pid;
	/** Feedforward gain */
	float Kff;
	/** Reference angular velocity */
	float reference;
	/** Measured angular velocity */
	float omega;
	/** Control signal (voltage) */
	float control;
	/** Execute a tick */
	uint32_t tick;
	/** Interrupt flag register */
	uint32_t INTF;
} __attribute__((packed)) __attribute__((aligned(4)));

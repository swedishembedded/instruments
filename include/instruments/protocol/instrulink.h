/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/consulting
 * Training: https://swedishembedded.com/tag/training
 **/

#pragma once

/*!
 * @defgroup instrulink-library-api Instrulink Library API
 * @{
 **/

#include "protocol.h"

struct instrulink;

/**
 * \brief Create a new instrulink connection instance
 * \returns new instance or NULL on error
 **/
struct instrulink *instrulink_new(void);

/**
 * \brief Frees instrulink connection instance and sets self to NULL
 * \param self instrulink instance to free
 **/
void instrulink_free(struct instrulink **self);

/**
 * \brief Connect to instrulink
 * \param self poitner to instrulink instance
 * \param mainPort main connection port supplied by instrulink on cli
 * \param irqPort irq connection port supplied by instrulink on cli
 * \param ip ip address to connect to (usually 127.0.0.1)
 * \returns 0 on success or negative on error
 * \retval -EINVAL invalid argument supplied
 * \retval -ECONN connection was refused
 **/
int instrulink_connect(struct instrulink *self, int mainPort, int irqPort, const char *ip);

/**
 * \brief Close connections to instrulink and cleanup
 * \param self pointer to instrulink instance
 * \returns 0 on success, non-zero on error
 **/
int instrulink_disconnect(struct instrulink *self);

/**
 * \brief Receives a packet from instrulink (blocking)
 * \param self instrulink instance
 * \param req request received
 * \returns 0 on success and negative error on failure
 **/
int instrulink_wait_request(struct instrulink *self, struct instrulink_packet *req);

/**
 * \brief Sends a response packet.
 * \details Must be called after every received packet
 * \param self instrulink instance
 * \param res response received
 * \returns 0 on success and negative error on failure
 **/
int instrulink_send_response(struct instrulink *self, struct instrulink_packet *res);

/**
 * \brief Sends an interrupt notification.
 * \details Currently you can only raise a request and application should then
 * check what needs to be done by reading interrupt flags.
 * \param self instrulink instance
 * \returns 0 on success and negative error on failure
 **/
int instrulink_irq_notify(struct instrulink *self);

/*!
 * @}
 **/

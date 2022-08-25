// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/consulting
 * Training: https://swedishembedded.com/tag/training
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifdef __linux__
#include <unistd.h>
#else
#include <net/socket.h>
#endif

#include <instruments/protocol/instrulink.h>

struct instrulink {
	/** Main socket from simulator to us (req/res) */
	int mainSocket;
	/** Special socket from us to simulator (req/res) */
	int irqSocket;
};

struct instrulink *instrulink_new(void)
{
	struct instrulink *self = (struct instrulink *)malloc(sizeof(struct instrulink));

	memset(self, 0, sizeof(*self));
	return self;
}

void instrulink_free(struct instrulink **self)
{
	free(*self);
	*self = NULL;
}

int instrulink_connect(struct instrulink *self, int mainPort, int irqPort, const char *address)
{
	self->mainSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (self->mainSocket == -1) {
		return -ECONNREFUSED;
	}

	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, address, &addr.sin_addr);
	addr.sin_port = htons(mainPort);

	if (connect(self->mainSocket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		close(self->mainSocket);
		return -ECONNREFUSED;
	}

	self->irqSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (self->irqSocket == -1) {
		return -ECONNREFUSED;
	}

	addr.sin_port = htons(irqPort);

	if (connect(self->irqSocket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		fprintf(stderr, "Could not connect to %s:%d\n", address, irqPort);
		close(self->mainSocket);
		close(self->irqSocket);
		return -ECONNREFUSED;
	}
	return 0;
}

int instrulink_disconnect(struct instrulink *self)
{
	close(self->mainSocket);
	close(self->irqSocket);
	return 0;
}

int instrulink_wait_request(struct instrulink *self, struct instrulink_packet *req)
{
	int r = recv(self->mainSocket, req, sizeof(*req), 0);

	if (r < 0) {
		return -EIO;
	} else if (r != sizeof(*req)) {
		return -EINVAL;
	}
	return 0;
}

int instrulink_send_response(struct instrulink *self, struct instrulink_packet *res)
{
	int r = send(self->mainSocket, res, sizeof(*res), 0);

	if (r < 0) {
		return -EIO;
	} else if (r != sizeof(*res)) {
		return -EINVAL;
	}
	return 0;
}

int instrulink_irq_notify(struct instrulink *self)
{
	struct instrulink_packet res;

	res.type = MSG_TYPE_IRQ;

	int r = send(self->irqSocket, &res, sizeof(res), 0);

	if (r < 0) {
		return -EIO;
	} else if (r != sizeof(res)) {
		return -EINVAL;
	}
	return 0;
}

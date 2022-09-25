// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 *
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/

#include "BaseInstrument.h"
#include <pthread.h>
#include <list>

class InstrumentContainer {
    public:
	InstrumentContainer();
	int init(int argc, char **argv);
	int addInstrument(IInstrument *i);
	int show();
	friend void *_communication_thread(void *data);

    protected:
	int write32(uint64_t addr, uint64_t data);
	int write16(uint64_t addr, uint64_t data);
	int write8(uint64_t addr, uint64_t data);
	int read32(uint64_t addr, uint64_t *data);
	int read16(uint64_t addr, uint64_t *data);
	int read8(uint64_t addr, uint64_t *data);

	bool isRunning();
	int handleBusRequests();
	void emitIRQ();

    private:
	/** Main lock */
	pthread_mutex_t lock;
	/** Connection to simulator */
	struct instrulink *instrulink;
	/** Flag showing whether we are still active or waiting for threads to finish */
	bool is_running;
	/** List of instruments */
	std::list<IInstrument *> instruments;
};

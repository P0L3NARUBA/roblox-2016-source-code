/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

class LcmRand {
public:
    LcmRand() : seed(1337u) {}

    uint32_t value() {
        const static uint32_t a = 214013u;
        const static uint32_t c = 2531011u;
        seed = seed * a + c;
        return (seed >> 16u) & 0x7FFFu;
    }

	void setSeed(uint32_t newSeed) {
		seed = newSeed;
	}

private:
    uint32_t seed;
};
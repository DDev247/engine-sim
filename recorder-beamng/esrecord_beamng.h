#ifndef ESRECORD_BEAMNG_ESRECORD_H
#define ESRECORD_BEAMNG_ESRECORD_H

// include STL headers
#include <cstdint>
#include <string>

// define structures etc.
struct Config {
	int sampleLength;

	int sampleCount;
	int* sampleRPMValues;
	int* sampleRPMFrequencyValues;

	int sampleThrottleValuesCount;
	int *sampleThrottleValues;
};

#endif

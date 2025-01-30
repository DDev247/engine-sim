
#ifndef ESRECORD_ESRECORD_CONFIG_H
#define ESRECORD_ESRECORD_CONFIG_H

#include <fstream>
#include <string>

// define structures etc.
struct Config {
	int prerunCount;

	int sampleLength;

	int sampleCount;
	int* sampleRPMValues;
	int* sampleRPMFrequencyValues;

	int sampleThrottleValuesCount;
	int* sampleThrottleValues;

	bool overrideRevlimit;
};

bool loadConfig(Config& out);
void enableAnsi();

#endif

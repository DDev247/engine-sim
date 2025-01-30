#ifndef ESRECORD_ESRECORD_H
#define ESRECORD_ESRECORD_H

// include STL headers
#include <cstdint>
#include <string>

#include "esrecord_config.h"

// include engine-sim headers
#include "../include/piston_engine_simulator.h"
#include "../include/simulator.h"

#include "../include/engine.h"
#include "../include/transmission.h"
#include "../include/vehicle.h"

// include engine-sim-scripting-interface headers
#include "../scripting/include/compiler.h"

// include delta-basic headers
#include <delta-studio/include/yds_core.h>
#include <delta-studio/engines/basic/include/delta_basic_engine.h>

// define private binding variables
extern Simulator* ESRecord_Simulator;
extern Engine* ESRecord_Engine;
extern Transmission* ESRecord_Transmission;
extern Vehicle* ESRecord_Vehicle;

void initialise(const char* path);
void initialise();
double update(float averagefps);

struct SampleConfig {
	int prerunCount;
	int rpm, throttle, frequency, length;
	std::string output;
	bool overrideRevlimit;
};

#endif

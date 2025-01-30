#ifndef ESRECORD_ESRECORD_H
#define ESRECORD_ESRECORD_H

// include STL headers
#include <cstdint>
#include <string>

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

#define ESRECORD_API extern "C" __declspec(dllexport)

// define private binding variables
extern Simulator* ESRecord_Simulator[];
extern Engine* ESRecord_Engine[];
extern Transmission* ESRecord_Transmission[];
extern Vehicle* ESRecord_Vehicle[];

ESRECORD_API bool ESRecord_Compile(int instanceId, const char* path);
ESRECORD_API bool ESRecord_Initialise(int instanceId);
ESRECORD_API double ESRecord_Update(int instanceId, float averagefps);

enum ESRecordState {
	ESRECORD_STATE_IDLE,
	ESRECORD_STATE_COMPILING,
	ESRECORD_STATE_PREPARING,
	ESRECORD_STATE_WARMUP,
	ESRECORD_STATE_RECORDING
};

extern ESRecordState ESRecord_CurrentState[];
extern int ESRecord_Progress[];

ESRECORD_API ESRecordState ESRecord_GetState(int instanceId, int& progress);
ESRECORD_API int ESRecord_GetVersion();

ESRECORD_API char* ESRecord_Engine_GetName(int instanceId=0);
ESRECORD_API float ESRecord_Engine_GetRedline(int instanceId=0);
ESRECORD_API float ESRecord_Engine_GetDisplacement(int instanceId=0);

struct SampleConfig {
	bool overrideRevlimit;
	int prerunCount;
	int rpm, throttle, frequency, length;
	char output[256];
};

struct SampleResult {
	bool success;
	float power, torque, ratio;
	long long millis;
};

ESRECORD_API SampleResult ESRecord_Record(int instanceId, SampleConfig config);

#endif

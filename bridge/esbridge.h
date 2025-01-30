#ifndef DDEV_ESBRIDGE_H
#define DDEV_ESBRIDGE_H

// include STL headers
#include <cstdint>

// include engine-sim headers
#include "../include/units.h"
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

// define API interface
#define ESBRIDGE_API extern "C" __declspec(dllexport)

// define private binding variables
extern Simulator* ESBridge_Simulator;
extern Engine* ESBridge_Engine;
extern Transmission* ESBridge_Transmission;
extern Vehicle* ESBridge_Vehicle;

// define API functions
ESBRIDGE_API void ESBridge_Initialise();
ESBRIDGE_API double ESBridge_Update(float averagefps);
ESBRIDGE_API int ESBridge_ReadBuffer(int count, int16_t* buffer);

ESBRIDGE_API void ESBridge_SetSimulatorFrequency(int frequency);

ESBRIDGE_API void ESBridge_ParseScript(const char* path);

ESBRIDGE_API double ESBridge_GetRpm();

ESBRIDGE_API void ESBridge_SetStarter(bool value);
ESBRIDGE_API void ESBridge_SetIgnition(bool value);
ESBRIDGE_API void ESBridge_SetSpeedControl(double value);

#endif

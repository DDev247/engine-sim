#ifndef DDEV_ESBRIDGE_H
#define DDEV_ESBRIDGE_H

// include STL headers
#include <cstdint>

// include engine-sim headers
#include "../include/piston_engine_simulator.h"
#include "../include/simulator.h"

#include "../include/engine.h"
#include "../include/direct_throttle_linkage.h"
#include "../include/standard_valvetrain.h"
#include "../include/vtec_valvetrain.h"

#include "../include/transmission.h"
#include "../include/vehicle.h"

// define API interface
#define ESBRIDGE_API __declspec(dllexport)

// define private binding variables
extern Simulator* ESBridge_Simulator;
extern Engine* ESBridge_Engine;
extern Transmission* ESBridge_Transmission;
extern Vehicle* ESBridge_Vehicle;

// define API functions
ESBRIDGE_API void ESBridge_Initialise();
ESBRIDGE_API double ESBridge_Update(float averagefps);
ESBRIDGE_API int ESBridge_ReadBuffer(int count, int16_t* buffer);
ESBRIDGE_API double ESBridge_MomentOfInertia(double mass, double radius);

// define structures
struct ESBRIDGE_API ESBridge_ValvetrainStruct {
    int crankshaft;
    int lobes;

    Function* intake;
    double* intakeLobes;
    Function* exhaust;
    double* exhaustLobes;

    // 0 is standard 1 is vtec
    bool vtec;
    
    Function* vtecIntake;
    double* vtecIntakeLobes;
    Function* vtecExhaust;
    double* vtecExhaustLobes;

    double vtecMinRpm;
    double vtecMinSpeed;
    double vtecMinThrottlePosition;
    double vtecManifoldVacuum;
};

// define Engine API functions
ESBRIDGE_API void ESBridge_CreateEngine(int cylinderBanks, int cylinderCount, int crankshaftCount, int exhaustCount, int intakeCount);
ESBRIDGE_API void ESBridge_CreateBank(
    int bank, double bankAngle, double bankBore, double bankDeckHeight, double bankDisplayDepth, int bankCylinderCount,
    int crankshaft, double crankMass, double crankFlywheelMass, double crankMomentOfInertia, double crankThrow, double crankTdc, double crankFrictionTorque, double* crankJournals);
ESBRIDGE_API Function* ESBridge_CreateFunction(int length, double* x, double* y, double filterRadius);
ESBRIDGE_API ESBridge_ValvetrainStruct* ESBridge_CreateStandardValvetrain(int crankshaft, int lobes,
    Function* intake, double* intakeLobes, Function* exhaust, double* exhaustLobes);
ESBRIDGE_API ESBridge_ValvetrainStruct* ESBridge_CreateVTECValvetrain(int crankshaft, int lobes,
    Function* intake, double* intakeLobes, Function* exhaust, double* exhaustLobes,
    Function* vtecIntake, double* vtecIntakeLobes, Function* vtecExhaust, double* vtecExhaustLobes,
    double vtecMinRpm, double vtecMinSpeed, double vtecMinThrottlePosition, double vtecManifoldVacuum);
ESBRIDGE_API void ESBridge_CreateCylinderHead(
    int head, int bank,
    Function* intakePortFlow, Function* exhaustPortFlow, ESBridge_ValvetrainStruct* valvetrain,
    double headCombustionVolume, double headIntakeRunnerVolume, double headIntakeRunnerArea, double headExhaustRunnerVolume, double headExhaustRunnerArea);
ESBRIDGE_API void ESBridge_CreateCylinder(int bank, int cylinder, int crankshaft, int intake, int exhaust,
    double pistonBlowby, double pistonCompressionHeight, double pistonWristPinPosition, double pistonDisplacement, double pistonMass,
    double rodMass, double rodMomentOfInertia, double rodCenterOfMass, double rodLength);
ESBRIDGE_API void ESBridge_CreateIntake(int intake);

#endif

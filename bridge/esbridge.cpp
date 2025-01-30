
#include "esbridge.h"

Simulator* ESBridge_Simulator = nullptr;
Engine* ESBridge_Engine = nullptr;
Transmission* ESBridge_Transmission = nullptr;
Vehicle* ESBridge_Vehicle = nullptr;

ESBRIDGE_API double ESBridge_MomentOfInertia(double mass, double radius) {
    return mass * (radius * radius);
}

ESBRIDGE_API void ESBridge_Initialise() {
    // do init stuff
    /*ESBridge_CreateEngine(1, 1, 1, 1, 1);
    ESBridge_CreateBank(
        0, 0, 80 * units::mm, 1 * units::mm, 0.0, 1,
        0, 1 * units::kg, 1 * units::kg, ESBridge_MomentOfInertia(2 * units::kg, 10 * units::inch), 80 * units::mm, 0, 1 * units::N
    );


    Vehicle::Parameters vehParams;
    vehParams.mass = units::mass(1597, units::kg);
    vehParams.diffRatio = 3.42;
    vehParams.tireRadius = units::distance(10, units::inch);
    vehParams.dragCoefficient = 0.25;
    vehParams.crossSectionArea = units::distance(6.0, units::foot) * units::distance(6.0, units::foot);
    vehParams.rollingResistance = 2000.0;
    ESBridge_Vehicle = new Vehicle;
    ESBridge_Vehicle->initialize(vehParams);
    
    const double gearRatios[] = { 2.97, 2.07, 1.43, 1.00, 0.84, 0.56 };
    Transmission::Parameters tParams;
    tParams.GearCount = 6;
    tParams.GearRatios = gearRatios;
    tParams.MaxClutchTorque = units::torque(1000.0, units::ft_lb);
    ESBridge_Transmission = new Transmission;
    ESBridge_Transmission->initialize(tParams);*/
}

ESBRIDGE_API double ESBridge_Update(float averagefps) {
    const double avgFramerate = clamp(averagefps, 30.0f, 1000.0f);
    ESBridge_Simulator->startFrame(1 / avgFramerate);

    auto proc_t0 = std::chrono::steady_clock::now();
    const int iterationCount = ESBridge_Simulator->getFrameIterationCount();
    while (ESBridge_Simulator->simulateStep()) {
        continue; // do nothing
    }

    auto proc_t1 = std::chrono::steady_clock::now();

    ESBridge_Simulator->endFrame();

    auto duration = proc_t1 - proc_t0;

    if (iterationCount > 0) {
        return (duration.count() / 1E9) / iterationCount;
    }

    return 0.0f;
}

ESBRIDGE_API int ESBridge_ReadBuffer(int count, int16_t* buffer) {
	return ESBridge_Simulator->readAudioOutput(count, buffer);
}

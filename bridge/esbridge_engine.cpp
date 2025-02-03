
#include "esbridge.h"

ESBRIDGE_API void ESBridge_ParseScript(const char* path) {
    
    Engine* engine = nullptr;
    Vehicle* vehicle = nullptr;
    Transmission* transmission = nullptr;

    es_script::Compiler compiler;
    compiler.initialize(0);
    const bool compiled = compiler.compile(path);
    if (compiled) {
        const es_script::Compiler::Output output = compiler.execute();

        engine = output.engine;
        vehicle = output.vehicle;
        transmission = output.transmission;
    }
    else {
        engine = nullptr;
        vehicle = nullptr;
        transmission = nullptr;
    }

    compiler.destroy();

    if (vehicle == nullptr) {
        Vehicle::Parameters vehParams;
        vehParams.mass = units::mass(1597, units::kg);
        vehParams.diffRatio = 3.42;
        vehParams.tireRadius = units::distance(10, units::inch);
        vehParams.dragCoefficient = 0.25;
        vehParams.crossSectionArea = units::distance(6.0, units::foot) * units::distance(6.0, units::foot);
        vehParams.rollingResistance = 2000.0;
        vehicle = new Vehicle;
        vehicle->initialize(vehParams);
    }

    if (transmission == nullptr) {
        const double gearRatios[] = { 2.97, 2.07, 1.43, 1.00, 0.84, 0.56 };
        Transmission::Parameters tParams;
        tParams.GearCount = 6;
        tParams.GearRatios = gearRatios;
        tParams.MaxClutchTorque = units::torque(1000.0, units::ft_lb);
        transmission = new Transmission;
        transmission->initialize(tParams);
    }

    if (ESBridge_Simulator != nullptr) {
        ESBridge_Simulator->releaseSimulation();
        delete ESBridge_Simulator;
    }

    if (ESBridge_Vehicle != nullptr) {
        delete ESBridge_Vehicle;
        ESBridge_Vehicle = nullptr;
    }

    if (ESBridge_Transmission != nullptr) {
        delete ESBridge_Transmission;
        ESBridge_Transmission = nullptr;
    }

    if (ESBridge_Engine != nullptr) {
        ESBridge_Engine->destroy();
        delete ESBridge_Engine;
    }

    ESBridge_Engine = engine;
    ESBridge_Vehicle = vehicle;
    ESBridge_Transmission = transmission;

    ESBridge_Simulator = engine->createSimulator(vehicle, transmission);

    if (engine == nullptr || vehicle == nullptr || transmission == nullptr) {
        ESBridge_Engine = nullptr;

        return;
    }

    engine->calculateDisplacement();
    ESBridge_Simulator->setSimulationFrequency(engine->getSimulationFrequency());

    Synthesizer::AudioParameters audioParams = ESBridge_Simulator->synthesizer().getAudioParameters();
    audioParams.convolution = static_cast<float>(engine->getInitialConvolution());
    audioParams.inputSampleNoise = static_cast<float>(engine->getInitialJitter());
    audioParams.airNoise = static_cast<float>(engine->getInitialNoise());
    audioParams.dF_F_mix = static_cast<float>(engine->getInitialHighFrequencyGain());
    ESBridge_Simulator->synthesizer().setAudioParameters(audioParams);

    for (int i = 0; i < engine->getExhaustSystemCount(); ++i) {
        ImpulseResponse* response = engine->getExhaustSystem(i)->getImpulseResponse();

        ysWindowsAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        ESBridge_Simulator->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t*>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            response->getVolume(),
            i
        );

        waveFile.DestroyInternalBuffer();
    }

    ESBridge_Simulator->startAudioRenderingThread();
}

ESBRIDGE_API double ESBridge_GetRpm() {
    return ESBridge_Engine->getRpm();
}

ESBRIDGE_API void ESBridge_SetSimulatorFrequency(int frequency) {
    ESBridge_Simulator->setSimulationFrequency(frequency);
}

ESBRIDGE_API void ESBridge_SetStarter(bool value) {
    ESBridge_Simulator->m_starterMotor.m_enabled = value;
}

ESBRIDGE_API void ESBridge_SetIgnition(bool value) {
    ESBridge_Engine->getIgnitionModule()->m_enabled = value;
}

ESBRIDGE_API void ESBridge_SetSpeedControl(double value) {
    ESBridge_Engine->setSpeedControl(value);
}

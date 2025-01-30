
#include "esrecord_lib.h"

ESRECORD_API bool ESRecord_Compile(int instanceId, const char* path) {
	ESRecord_CurrentState[instanceId] = ESRECORD_STATE_COMPILING;

    Engine* engine = nullptr;
    Vehicle* vehicle = nullptr;
    Transmission* transmission = nullptr;

    es_script::Compiler compiler;
    compiler.initialize(instanceId);
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

    if (ESRecord_Simulator[instanceId] != nullptr) {
        ESRecord_Simulator[instanceId]->releaseSimulation();
        delete ESRecord_Simulator[instanceId];
        ESRecord_Simulator[instanceId] = nullptr;
    }

    if (ESRecord_Vehicle[instanceId] != nullptr) {
        delete ESRecord_Vehicle[instanceId];
        ESRecord_Vehicle[instanceId] = nullptr;
    }

    if (ESRecord_Transmission[instanceId] != nullptr) {
        delete ESRecord_Transmission[instanceId];
        ESRecord_Transmission[instanceId] = nullptr;
    }

    if (ESRecord_Engine[instanceId] != nullptr) {
        ESRecord_Engine[instanceId]->destroy();
        delete ESRecord_Engine[instanceId];
    }

	if (engine == nullptr) {
        ESRecord_CurrentState[instanceId] = ESRECORD_STATE_IDLE;
        return false;
	}

    ESRecord_Engine[instanceId] = engine;
    ESRecord_Vehicle[instanceId] = vehicle;
    ESRecord_Transmission[instanceId] = transmission;

    ESRecord_CurrentState[instanceId] = ESRECORD_STATE_IDLE;

    return ESRecord_Initialise(instanceId);
}

ESRECORD_API bool ESRecord_Initialise(int instanceId) {
    if (ESRecord_Simulator[instanceId] != nullptr) {
        ESRecord_Simulator[instanceId]->releaseSimulation();
        delete ESRecord_Simulator[instanceId];
        ESRecord_Simulator[instanceId] = nullptr;
    }

    if (ESRecord_Engine[instanceId] == nullptr || ESRecord_Vehicle[instanceId] == nullptr || ESRecord_Transmission[instanceId] == nullptr) {
        ESRecord_Engine[instanceId] = nullptr;

        return false;
    }

    ESRecord_Simulator[instanceId] = ESRecord_Engine[instanceId]->createSimulator(ESRecord_Vehicle[instanceId], ESRecord_Transmission[instanceId]);

    ESRecord_Engine[instanceId]->calculateDisplacement();
    ESRecord_Simulator[instanceId]->setSimulationFrequency(ESRecord_Engine[instanceId]->getSimulationFrequency());

    Synthesizer::AudioParameters audioParams = ESRecord_Simulator[instanceId]->synthesizer().getAudioParameters();
    audioParams.convolution = static_cast<float>(ESRecord_Engine[instanceId]->getInitialConvolution());
    audioParams.inputSampleNoise = static_cast<float>(ESRecord_Engine[instanceId]->getInitialJitter());
    audioParams.airNoise = static_cast<float>(ESRecord_Engine[instanceId]->getInitialNoise());
    audioParams.dF_F_mix = static_cast<float>(ESRecord_Engine[instanceId]->getInitialHighFrequencyGain());
    ESRecord_Simulator[instanceId]->synthesizer().setAudioParameters(audioParams);

    for (int i = 0; i < ESRecord_Engine[instanceId]->getExhaustSystemCount(); ++i) {
        ImpulseResponse* response = ESRecord_Engine[instanceId]->getExhaustSystem(i)->getImpulseResponse();

        ysWindowsAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        ESRecord_Simulator[instanceId]->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t*>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            response->getVolume(),
            i
        );

        waveFile.DestroyInternalBuffer();
    }

    ESRecord_Simulator[instanceId]->startAudioRenderingThread();

    return true;
}

ESRECORD_API double ESRecord_Update(int instanceId, float averagefps) {
    const double avgFramerate = clamp(averagefps, 30.0f, 1000.0f);
    ESRecord_Simulator[instanceId]->startFrame(1 / avgFramerate);

    auto proc_t0 = std::chrono::steady_clock::now();
    const int iterationCount = ESRecord_Simulator[instanceId]->getFrameIterationCount();
    while (ESRecord_Simulator[instanceId]->simulateStep()) {
        continue; // do nothing
    }

    auto proc_t1 = std::chrono::steady_clock::now();

    ESRecord_Simulator[instanceId]->endFrame();

    auto duration = proc_t1 - proc_t0;

    if (iterationCount > 0) {
        return (duration.count() / 1E9) / iterationCount;
    }

    return 0.0f;
}

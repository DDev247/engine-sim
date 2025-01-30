
#include "esrecord_lib.h"

ESRECORD_API bool ESRecord_Compile(const char* path) {
	ESRecord_CurrentState = ESRECORD_STATE_COMPILING;

    Engine* engine = nullptr;
    Vehicle* vehicle = nullptr;
    Transmission* transmission = nullptr;

    es_script::Compiler compiler;
    compiler.initialize();
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

    if (ESRecord_Simulator != nullptr) {
        ESRecord_Simulator->releaseSimulation();
        delete ESRecord_Simulator;
        ESRecord_Simulator = nullptr;
    }

    if (ESRecord_Vehicle != nullptr) {
        delete ESRecord_Vehicle;
        ESRecord_Vehicle = nullptr;
    }

    if (ESRecord_Transmission != nullptr) {
        delete ESRecord_Transmission;
        ESRecord_Transmission = nullptr;
    }

    if (ESRecord_Engine != nullptr) {
        ESRecord_Engine->destroy();
        delete ESRecord_Engine;
    }

	if (engine == nullptr) {
        ESRecord_CurrentState = ESRECORD_STATE_IDLE;
        return false;
	}

    ESRecord_Engine = engine;
    ESRecord_Vehicle = vehicle;
    ESRecord_Transmission = transmission;

    ESRecord_CurrentState = ESRECORD_STATE_IDLE;

    return ESRecord_Initialise();
}

ESRECORD_API bool ESRecord_Initialise() {
    if (ESRecord_Simulator != nullptr) {
        ESRecord_Simulator->releaseSimulation();
        delete ESRecord_Simulator;
        ESRecord_Simulator = nullptr;
    }

    if (ESRecord_Engine == nullptr || ESRecord_Vehicle == nullptr || ESRecord_Transmission == nullptr) {
        ESRecord_Engine = nullptr;

        return false;
    }

    ESRecord_Simulator = ESRecord_Engine->createSimulator(ESRecord_Vehicle, ESRecord_Transmission);

    ESRecord_Engine->calculateDisplacement();
    ESRecord_Simulator->setSimulationFrequency(ESRecord_Engine->getSimulationFrequency());

    Synthesizer::AudioParameters audioParams = ESRecord_Simulator->synthesizer().getAudioParameters();
    audioParams.convolution = static_cast<float>(ESRecord_Engine->getInitialConvolution());
    audioParams.inputSampleNoise = static_cast<float>(ESRecord_Engine->getInitialJitter());
    audioParams.airNoise = static_cast<float>(ESRecord_Engine->getInitialNoise());
    audioParams.dF_F_mix = static_cast<float>(ESRecord_Engine->getInitialHighFrequencyGain());
    ESRecord_Simulator->synthesizer().setAudioParameters(audioParams);

    for (int i = 0; i < ESRecord_Engine->getExhaustSystemCount(); ++i) {
        ImpulseResponse* response = ESRecord_Engine->getExhaustSystem(i)->getImpulseResponse();

        ysWindowsAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        ESRecord_Simulator->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t*>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            response->getVolume(),
            i
        );

        waveFile.DestroyInternalBuffer();
    }

    ESRecord_Simulator->startAudioRenderingThread();

    return true;
}

ESRECORD_API double ESRecord_Update(float averagefps) {
    const double avgFramerate = clamp(averagefps, 30.0f, 1000.0f);
    ESRecord_Simulator->startFrame(1 / avgFramerate);

    auto proc_t0 = std::chrono::steady_clock::now();
    const int iterationCount = ESRecord_Simulator->getFrameIterationCount();
    while (ESRecord_Simulator->simulateStep()) {
        continue; // do nothing
    }

    auto proc_t1 = std::chrono::steady_clock::now();

    ESRecord_Simulator->endFrame();

    auto duration = proc_t1 - proc_t0;

    if (iterationCount > 0) {
        return (duration.count() / 1E9) / iterationCount;
    }

    return 0.0f;
}

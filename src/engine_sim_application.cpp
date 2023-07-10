#include "../include/engine_sim_application.h"

#include "../include/piston_object.h"
#include "../include/connecting_rod_object.h"
#include "../include/constants.h"
#include "../include/units.h"
#include "../include/crankshaft_object.h"
#include "../include/cylinder_bank_object.h"
#include "../include/cylinder_head_object.h"
#include "../include/ui_button.h"
#include "../include/combustion_chamber_object.h"
#include "../include/csv_io.h"
#include "../include/exhaust_system.h"
#include "../include/feedback_comb_filter.h"
#include "../include/utilities.h"

#include "../scripting/include/compiler.h"

#include <chrono>
#include <stdlib.h>
#include <sstream>

#if ATG_ENGINE_SIM_DISCORD_ENABLED
#include "../discord/Discord.h"
#endif

std::string EngineSimApplication::s_buildVersion = "0.1.12a EFI 0.1";

EngineSimApplication::EngineSimApplication() : tscpp("COM3", 115200) {
    m_assetPath = "";

    m_geometryVertexBuffer = nullptr;
    m_geometryIndexBuffer = nullptr;

    m_paused = false;
    m_recording = false;
    m_screenResolutionIndex = 0;
    for (int i = 0; i < ScreenResolutionHistoryLength; ++i) {
        m_screenResolution[i][0] = m_screenResolution[i][1] = 0;
    }

    m_background = ysColor::srgbiToLinear(0x0E1012);
    m_foreground = ysColor::srgbiToLinear(0xFFFFFF);
    m_shadow = ysColor::srgbiToLinear(0x0E1012);
    m_highlight1 = ysColor::srgbiToLinear(0xEF4545);
    m_highlight2 = ysColor::srgbiToLinear(0xFFFFFF);
    m_pink = ysColor::srgbiToLinear(0xF394BE);
    m_red = ysColor::srgbiToLinear(0xEE4445);
    m_orange = ysColor::srgbiToLinear(0xF4802A);
    m_yellow = ysColor::srgbiToLinear(0xFDBD2E);
    m_blue = ysColor::srgbiToLinear(0x77CEE0);
    m_green = ysColor::srgbiToLinear(0xBDD869);

    m_displayHeight = (float)units::distance(2.0, units::foot);
    m_outputAudioBuffer = nullptr;
    m_audioSource = nullptr;

    m_torque = 0;
    m_dynoSpeed = 0;

    m_simulator = nullptr;
    m_engineView = nullptr;
    m_rightGaugeCluster = nullptr;
    m_temperatureGauge = nullptr;
    m_oscCluster = nullptr;
    m_performanceCluster = nullptr;
    m_loadSimulationCluster = nullptr;
    m_mixerCluster = nullptr;
    m_infoCluster = nullptr;
    m_iceEngine = nullptr;
    m_mainRenderTarget = nullptr;

    m_vehicle = nullptr;
    m_transmission = nullptr;

    m_oscillatorSampleOffset = 0;
    m_gameWindowHeight = 256;
    m_screenWidth = 256;
    m_screenHeight = 256;
    m_screen = 0;
    m_viewParameters.Layer0 = 0;
    m_viewParameters.Layer1 = 0;

    m_displayAngle = 0.0f;

}

EngineSimApplication::~EngineSimApplication() {
    /* void */
}

void EngineSimApplication::initialize(void *instance, ysContextObject::DeviceAPI api) {
    dbasic::Path modulePath = dbasic::GetModulePath();
    dbasic::Path confPath = modulePath.Append("delta.conf");

    std::string enginePath = "../dependencies/submodules/delta-studio/engines/basic";
    m_assetPath = "../assets";
    if (confPath.Exists()) {
        std::fstream confFile(confPath.ToString(), std::ios::in);

        std::getline(confFile, enginePath);
        std::getline(confFile, m_assetPath);
        enginePath = modulePath.Append(enginePath).ToString();
        m_assetPath = modulePath.Append(m_assetPath).ToString();

        confFile.close();
    }

    m_engine.GetConsole()->SetDefaultFontDirectory(enginePath + "/fonts/");

    const std::string shaderPath = enginePath + "/shaders/";
    const std::string winTitle = "Engine Sim | AngeTheGreat | v" + s_buildVersion;
    dbasic::DeltaEngine::GameEngineSettings settings;
    settings.API = api;
    settings.DepthBuffer = false;
    settings.Instance = instance;
    settings.ShaderDirectory = shaderPath.c_str();
    settings.WindowTitle = winTitle.c_str();
    settings.WindowPositionX = 0;
    settings.WindowPositionY = 0;
    settings.WindowStyle = ysWindow::WindowStyle::Windowed;
    settings.WindowWidth = 1920;
    settings.WindowHeight = 1080;

    m_engine.CreateGameWindow(settings);

    m_engine.GetDevice()->CreateSubRenderTarget(
        &m_mainRenderTarget,
        m_engine.GetScreenRenderTarget(),
        0,
        0,
        0,
        0);

    m_engine.InitializeShaderSet(&m_shaderSet);
    m_shaders.Initialize(
        &m_shaderSet,
        m_mainRenderTarget,
        m_engine.GetScreenRenderTarget(),
        m_engine.GetDefaultShaderProgram(),
        m_engine.GetDefaultInputLayout());
    m_engine.InitializeConsoleShaders(&m_shaderSet);
    m_engine.SetShaderSet(&m_shaderSet);

    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));

    m_assetManager.SetEngine(&m_engine);

    m_engine.GetDevice()->CreateIndexBuffer(
        &m_geometryIndexBuffer, sizeof(unsigned short) * 200000, nullptr);
    m_engine.GetDevice()->CreateVertexBuffer(
        &m_geometryVertexBuffer, sizeof(dbasic::Vertex) * 100000, nullptr);

    m_geometryGenerator.initialize(100000, 200000);

    initialize();
}

void EngineSimApplication::initialize() {
    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));
    m_assetManager.CompileInterchangeFile((m_assetPath + "/assets").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((m_assetPath + "/assets").c_str(), true);

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());

    loadScript();

    m_audioBuffer.initialize(44100, 44100);
    m_audioBuffer.m_writePointer = (int)(44100 * 0.1);

    ysAudioParameters params;
    params.m_bitsPerSample = 16;
    params.m_channelCount = 1;
    params.m_sampleRate = 44100;
    m_outputAudioBuffer =
        m_engine.GetAudioDevice()->CreateBuffer(&params, 44100);

    m_audioSource = m_engine.GetAudioDevice()->CreateSource(m_outputAudioBuffer);
    m_audioSource->SetMode((m_simulator->getEngine() != nullptr)
        ? ysAudioSource::Mode::Loop
        : ysAudioSource::Mode::Stop);
    m_audioSource->SetPan(0.0f);
    m_audioSource->SetVolume(1.0f);

#ifdef ATG_ENGINE_SIM_DISCORD_ENABLED
    // Create a global instance of discord-rpc
    CDiscord::CreateInstance();

    // Enable it, this needs to be set via a config file of some sort. 
    GetDiscordManager()->SetUseDiscord(true);
    DiscordRichPresence passMe = { 0 };

    std::string engineName = (m_iceEngine != nullptr)
        ? m_iceEngine->getName()
        : "Broken Engine";

    GetDiscordManager()->SetStatus(passMe, engineName, s_buildVersion);
#endif /* ATG_ENGINE_SIM_DISCORD_ENABLED */
}

void EngineSimApplication::process(float frame_dt) {
    frame_dt = static_cast<float>(clamp(frame_dt, 1 / 200.0f, 1 / 30.0f));

    double speed = 1.0 / 1.0;
    if (m_engine.IsKeyDown(ysKey::Code::N1)) {
        speed = 1 / 10.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N2)) {
        speed = 1 / 100.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N3)) {
        speed = 1 / 200.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N4)) {
        speed = 1 / 500.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N5)) {
        speed = 1 / 1000.0;
    }

    if (m_engine.IsKeyDown(ysKey::Code::F1)) {
        m_displayAngle += frame_dt * 1.0f;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::F2)) {
        m_displayAngle -= frame_dt * 1.0f;
    }
    else if (m_engine.ProcessKeyDown(ysKey::Code::F3)) {
        m_displayAngle = 0.0f;
    }

    m_simulator->setSimulationSpeed(speed);

    const double avgFramerate = clamp(m_engine.GetAverageFramerate(), 30.0f, 1000.0f);
    m_simulator->startFrame(1 / avgFramerate);

    auto proc_t0 = std::chrono::steady_clock::now();
    const int iterationCount = m_simulator->getFrameIterationCount();
    while (m_simulator->simulateStep()) {
        m_oscCluster->sample();
    }

    auto proc_t1 = std::chrono::steady_clock::now();

    m_simulator->endFrame();

    auto duration = proc_t1 - proc_t0;
    if (iterationCount > 0) {
        m_performanceCluster->addTimePerTimestepSample(
            (duration.count() / 1E9) / iterationCount);
    }

    const SampleOffset safeWritePosition = m_audioSource->GetCurrentWritePosition();
    const SampleOffset writePosition = m_audioBuffer.m_writePointer;

    SampleOffset targetWritePosition =
        m_audioBuffer.getBufferIndex(safeWritePosition, (int)(44100 * 0.1));
    SampleOffset maxWrite = m_audioBuffer.offsetDelta(writePosition, targetWritePosition);

    SampleOffset currentLead = m_audioBuffer.offsetDelta(safeWritePosition, writePosition);
    SampleOffset newLead = m_audioBuffer.offsetDelta(safeWritePosition, targetWritePosition);

    if (currentLead > 44100 * 0.5) {
        m_audioBuffer.m_writePointer = m_audioBuffer.getBufferIndex(safeWritePosition, (int)(44100 * 0.05));
        currentLead = m_audioBuffer.offsetDelta(safeWritePosition, m_audioBuffer.m_writePointer);
        maxWrite = m_audioBuffer.offsetDelta(m_audioBuffer.m_writePointer, targetWritePosition);
    }

    if (currentLead > newLead) {
        maxWrite = 0;
    }

    int16_t *samples = new int16_t[maxWrite];
    const int readSamples = m_simulator->readAudioOutput(maxWrite, samples);

    for (SampleOffset i = 0; i < (SampleOffset)readSamples && i < maxWrite; ++i) {
        const int16_t sample = samples[i];
        if (m_oscillatorSampleOffset % 4 == 0) {
            m_oscCluster->getAudioWaveformOscilloscope()->addDataPoint(
                m_oscillatorSampleOffset,
                sample / (float)(INT16_MAX));
        }

        m_audioBuffer.writeSample(sample, m_audioBuffer.m_writePointer, (int)i);

        m_oscillatorSampleOffset = (m_oscillatorSampleOffset + 1) % (44100 / 10);
    }

    delete[] samples;

    if (readSamples > 0) {
        SampleOffset size0, size1;
        void *data0, *data1;
        m_audioSource->LockBufferSegment(
            m_audioBuffer.m_writePointer, readSamples, &data0, &size0, &data1, &size1);

        m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data0), m_audioBuffer.m_writePointer, size0);
        m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data1),
            m_audioBuffer.getBufferIndex(m_audioBuffer.m_writePointer, size0),
            size1);

        m_audioSource->UnlockBufferSegments(data0, size0, data1, size1);
        m_audioBuffer.commitBlock(readSamples);
    }

    m_performanceCluster->addInputBufferUsageSample(
        (double)m_simulator->getSynthesizerInputLatency() / m_simulator->getSynthesizerInputLatencyTarget());
    m_performanceCluster->addAudioLatencySample(
        m_audioBuffer.offsetDelta(m_audioSource->GetCurrentWritePosition(), m_audioBuffer.m_writePointer) / (44100 * 0.1));
}

void EngineSimApplication::render() {
    for (SimulationObject *object : m_objects) {
        object->generateGeometry();
    }

    m_viewParameters.Sublayer = 0;
    for (SimulationObject *object : m_objects) {
        object->render(&getViewParameters());
    }

    m_viewParameters.Sublayer = 1;
    for (SimulationObject *object : m_objects) {
        object->render(&getViewParameters());
    }

    m_viewParameters.Sublayer = 2;
    for (SimulationObject *object : m_objects) {
        object->render(&getViewParameters());
    }

    m_uiManager.render();
}

float EngineSimApplication::pixelsToUnits(float pixels) const {
    const float f = m_displayHeight / m_engineView->m_bounds.height();
    return pixels * f;
}

float EngineSimApplication::unitsToPixels(float units) const {
    const float f = m_engineView->m_bounds.height() / m_displayHeight;
    return units * f;
}

void EngineSimApplication::run() {
    while (true) {
        m_engine.StartFrame();

        if (!m_engine.IsOpen()) break;
        if (m_engine.ProcessKeyDown(ysKey::Code::Escape)) {
            break;
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::Return)) {
            m_audioSource->SetMode(ysAudioSource::Mode::Stop);
            loadScript();
            if (m_simulator->getEngine() != nullptr) {
                m_audioSource->SetMode(ysAudioSource::Mode::Loop);
            }
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::Tab)) {
            m_screen++;
            if (m_screen > 5) m_screen = 0;
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::F)) {
            if (m_engine.GetGameWindow()->GetWindowStyle() != ysWindow::WindowStyle::Fullscreen) {
                m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Fullscreen);
                m_infoCluster->setLogMessage("Entered fullscreen mode");
            }
            else {
                m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Windowed);
                m_infoCluster->setLogMessage("Exited fullscreen mode");
            }
        }

        m_gameWindowHeight = m_engine.GetGameWindow()->GetGameHeight();
        m_screenHeight = m_engine.GetGameWindow()->GetScreenHeight();
        m_screenWidth = m_engine.GetGameWindow()->GetScreenWidth();

        updateScreenSizeStability();

        processEngineInput();

        if (m_engine.ProcessKeyDown(ysKey::Code::Insert) &&
            m_engine.GetGameWindow()->IsActive()) {
            if (!isRecording() && readyToRecord()) {
                startRecording();
            }
            else if (isRecording()) {
                stopRecording();
            }
        }

        if (isRecording() && !readyToRecord()) {
            stopRecording();
        }

        if (!m_paused || m_engine.ProcessKeyDown(ysKey::Code::Right)) {
            // calculate ECM
            ecmProcess(m_engine.GetFrameLength());
            
            process(m_engine.GetFrameLength());
            
            // set TSCpp status
            ecmStatus(m_engine.GetFrameLength());
        }

        m_uiManager.update(m_engine.GetFrameLength());

        renderScene();

        m_engine.EndFrame();

        if (isRecording()) {
            recordFrame();
        }
    }

    if (isRecording()) {
        stopRecording();
    }

    m_simulator->endAudioRenderingThread();
    tscpp.StopThreads();
}

void EngineSimApplication::destroy() {
    m_shaderSet.Destroy();

    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryVertexBuffer);
    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryIndexBuffer);

    m_assetManager.Destroy();
    m_engine.Destroy();

    m_simulator->destroy();
    m_audioBuffer.destroy();
}

float runtime;
bool countRuntime;

float MAPlast = 0;

/*byte correctionWUE(float dt) {
    byte WUEValue;
    if (currentStatus.coolant > (table2D_getAxisValue(&WUETable, 9) - CALIBRATION_TEMPERATURE_OFFSET)) {
        //This prevents us doing the 2D lookup if we're already up to temp
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_WARMUP);
        WUEValue = table2D_getRawValue(&WUETable, 9);
    }
    else {
        BIT_SET(currentStatus.engine, BIT_ENGINE_WARMUP);
        WUEValue = table2D_getValue(&WUETable, currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET);
    }

    return WUEValue;
}

float aseTaper;

byte correctionASE(float dt) {
    int16_t ASEValue = currentStatus.ASEValue;
    //Two checks are required:
    //1) Is the engine run time less than the configured ase time
    //2) Make sure we're not still cranking
    if (BIT_CHECK(currentStatus.engine, BIT_ENGINE_CRANK) != true) {
        if ((runtime < (table2D_getValue(&ASECountTable, currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET))) && !(BIT_CHECK(currentStatus.engine, BIT_ENGINE_CRANK)))
        {
            BIT_SET(currentStatus.engine, BIT_ENGINE_ASE); //Mark ASE as active.
            ASEValue = 100 + table2D_getValue(&ASETable, currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET);
            aseTaper = 0;
        }
        else
        {
            if (aseTaper < configPage2.aseTaperTime) //Check if we've reached the end of the taper time
            {
                BIT_SET(currentStatus.engine, BIT_ENGINE_ASE); //Mark ASE as active.
                ASEValue = table2D_getValue(&ASETable, currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET);
                ASEValue *= aseTaper / configPage2.aseTaperTime;
                ASEValue += 100;
                aseTaper += dt;
            }
            else
            {
                BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ASE); //Mark ASE as inactive.
                ASEValue = 100;
            }
        }

        //Safety checks
        if (ASEValue > 255) { ASEValue = 255; }
        if (ASEValue < 0) { ASEValue = 0; }
        ASEValue = (byte)ASEValue;
    }
    else {
        //Engine is cranking, ASE disabled
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ASE); //Mark ASE as inactive.
        ASEValue = 100;
    }
    return ASEValue;
}

float crankingEnrichTaper = 0;

uint16_t correctionCranking(float dt) {
    uint16_t crankingValue = 100;
    //Check if we are actually cranking
    if (BIT_CHECK(currentStatus.engine, BIT_ENGINE_CRANK))
    {
        crankingValue = table2D_getValue(&crankingEnrichTable, currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET);
        crankingValue = (uint16_t)crankingValue * 5; //multiplied by 5 to get range from 0% to 1275%
        crankingEnrichTaper = 0;
    }

    //If we're not cranking, check if if cranking enrichment tapering to ASE should be done
    else if (crankingEnrichTaper < configPage10.crankingEnrichTaper)
    {
        crankingValue = table2D_getValue(&crankingEnrichTable, currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET);
        crankingValue = (uint16_t)crankingValue * 5; //multiplied by 5 to get range from 0% to 1275%
        //Taper start value needs to account for ASE that is now running, so total correction does not increase when taper begins
        unsigned long taperStart = (unsigned long)crankingValue * 100 / currentStatus.ASEValue;
        crankingValue = taperStart; //Taper from start value to 100%
        crankingValue *= crankingEnrichTaper / configPage10.crankingEnrichTaper;
        if (crankingValue < 100) { crankingValue = 100; } //Sanity check
        crankingEnrichTaper += dt;
    }
    return crankingValue;
}

float activateMAPDOT = 0;
float activateTPSDOT = 0;
float MAPlast;

uint16_t correctionAccel(float dt) {
    int16_t accelValue = 100;
    int16_t MAP_change = 0;
    int16_t TPS_change = 0;

    /*if (configPage2.aeMode == AE_MODE_MAP)
    {
        //Get the MAP rate change
        MAP_change = (currentStatus.MAP - MAPlast);
        currentStatus.mapDOT = ldiv(1000000, (MAP_time - MAPlast_time)).quot * MAP_change; //This is the % per second that the MAP has moved
        //currentStatus.mapDOT = 15 * MAP_change; //This is the kpa per second that the MAP has moved
    }
    else if (configPage2.aeMode == AE_MODE_TPS)
    {
        //Get the TPS rate change
        TPS_change = (currentStatus.TPS - currentStatus.TPSlast);
        //currentStatus.tpsDOT = ldiv(1000000, (TPS_time - TPSlast_time)).quot * TPS_change; //This is the % per second that the TPS has moved
        currentStatus.tpsDOT = (TPS_READ_FREQUENCY * TPS_change) / 2; //This is the % per second that the TPS has moved, adjusted for the 0.5% resolution of the TPS
    }


    //First, check whether the accel. enrichment is already running
    if (BIT_CHECK(currentStatus.engine, BIT_ENGINE_ACC) || BIT_CHECK(currentStatus.engine, BIT_ENGINE_DCC))
    {
        //If it is currently running, check whether it should still be running or whether it's reached it's end time
        if (time(nullptr) >= currentStatus.AEEndTime)
        {
            //Time to turn enrichment off
            BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ACC);
            BIT_CLEAR(currentStatus.engine, BIT_ENGINE_DCC);
            currentStatus.AEamount = 0;
            accelValue = 100;

            //Reset the relevant DOT value to 0
            if (configPage2.aeMode == AE_MODE_MAP) { currentStatus.mapDOT = 0; }
            else if (configPage2.aeMode == AE_MODE_TPS) { currentStatus.tpsDOT = 0; }
        }
        else
        {
            //Enrichment still needs to keep running. 
            //Simply return the total TAE amount
            accelValue = currentStatus.AEamount;

            //Need to check whether the accel amount has increased from when AE was turned on
            //If the accel amount HAS increased, we clear the current enrich phase and a new one will be started below
            if ((configPage2.aeMode == AE_MODE_MAP) && (abs(currentStatus.mapDOT) > activateMAPDOT))
            {
                BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ACC);
                BIT_CLEAR(currentStatus.engine, BIT_ENGINE_DCC);
            }
            else if ((configPage2.aeMode == AE_MODE_TPS) && (abs(currentStatus.tpsDOT) > activateTPSDOT))
            {
                BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ACC);
                BIT_CLEAR(currentStatus.engine, BIT_ENGINE_DCC);
            }
        }
    }

    if (!BIT_CHECK(currentStatus.engine, BIT_ENGINE_ACC) && !BIT_CHECK(currentStatus.engine, BIT_ENGINE_DCC)) //Need to check this again as it may have been changed in the above section (Both ACC and DCC are off if this has changed)
    {
        if (configPage2.aeMode == AE_MODE_MAP)
        {
            if (abs(MAP_change) <= configPage2.maeMinChange)
            {
                accelValue = 100;
                currentStatus.mapDOT = 0;
            }
            else
            {
                //If MAE isn't currently turned on, need to check whether it needs to be turned on
                if (abs(currentStatus.mapDOT) > configPage2.maeThresh)
                {
                    activateMAPDOT = abs(currentStatus.mapDOT);
                    currentStatus.AEEndTime = time(nullptr) + ((unsigned long)configPage2.aeTime * 10000); //Set the time in the future where the enrichment will be turned off. taeTime is stored as mS / 10, so multiply it by 100 to get it in uS
                    //Check if the MAP rate of change is negative or positive. Negative means decelarion.
                    if (currentStatus.mapDOT < 0)
                    {
                        BIT_SET(currentStatus.engine, BIT_ENGINE_DCC); //Mark deceleration enleanment as active.
                        accelValue = configPage2.decelAmount; //In decel, use the decel fuel amount as accelValue
                    } //Deceleration
                    //Positive MAP rate of change is acceleration.
                    else
                    {
                        BIT_SET(currentStatus.engine, BIT_ENGINE_ACC); //Mark acceleration enrichment as active.
                        accelValue = table2D_getValue(&maeTable, currentStatus.mapDOT / 10); //The x-axis of mae table is divided by 10 to fit values in byte.

                        //Apply the RPM taper to the above
                        //The RPM settings are stored divided by 100:
                        uint16_t trueTaperMin = configPage2.aeTaperMin * 100;
                        uint16_t trueTaperMax = configPage2.aeTaperMax * 100;
                        if (currentStatus.RPM > trueTaperMin)
                        {
                            if (currentStatus.RPM > trueTaperMax) { accelValue = 0; } //RPM is beyond taper max limit, so accel enrich is turned off
                            else
                            {
                                int16_t taperRange = trueTaperMax - trueTaperMin;
                                int16_t taperPercent = ((currentStatus.RPM - trueTaperMin) * 100UL) / taperRange; //The percentage of the way through the RPM taper range
                                accelValue = (((100 - taperPercent) * accelValue) / 100); //Calculate the above percentage of the calculated accel amount. 
                            }
                        }

                        //Apply AE cold coolant modifier, if CLT is less than taper end temperature
                        if (currentStatus.coolant < (int)(configPage2.aeColdTaperMax - CALIBRATION_TEMPERATURE_OFFSET))
                        {
                            //If CLT is less than taper min temp, apply full modifier on top of accelValue
                            if (currentStatus.coolant <= (int)(configPage2.aeColdTaperMin - CALIBRATION_TEMPERATURE_OFFSET))
                            {
                                uint16_t accelValue_uint = (configPage2.aeColdPct * accelValue) / 100;
                                accelValue = (int16_t)accelValue_uint;
                            }
                            //If CLT is between taper min and max, taper the modifier value and apply it on top of accelValue
                            else
                            {
                                int16_t taperRange = (int16_t)configPage2.aeColdTaperMax - configPage2.aeColdTaperMin;
                                int16_t taperPercent = (int)((currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET - configPage2.aeColdTaperMin) * 100) / taperRange;
                                int16_t coldPct = (int16_t)100 + (((100 - taperPercent) * (configPage2.aeColdPct - 100)) / 100);
                                uint16_t accelValue_uint = (uint16_t)accelValue * coldPct / 100; //Potential overflow (if AE is large) without using uint16_t (percentage() may overflow)
                                accelValue = (int16_t)accelValue_uint;
                            }
                        }
                        accelValue = 100 + accelValue; //In case of AE, add the 100 normalisation to the calculated amount
                    }
                } //MAE Threshold
            } //MAP change threshold
        } //AE Mode
        else if (configPage2.aeMode == AE_MODE_TPS)
        {
            //Check for only very small movement. This not only means we can skip the lookup, but helps reduce false triggering around 0-2% throttle openings
            if (abs(TPS_change) <= configPage2.taeMinChange)
            {
                accelValue = 100;
                currentStatus.tpsDOT = 0;
            }
            else
            {
                //If TAE isn't currently turned on, need to check whether it needs to be turned on
                if (abs(currentStatus.tpsDOT) > configPage2.taeThresh)
                {
                    activateTPSDOT = abs(currentStatus.tpsDOT);
                    currentStatus.AEEndTime = time(nullptr) + ((unsigned long)configPage2.aeTime * 10000); //Set the time in the future where the enrichment will be turned off. taeTime is stored as mS / 10, so multiply it by 100 to get it in uS
                    //Check if the TPS rate of change is negative or positive. Negative means decelarion.
                    if (currentStatus.tpsDOT < 0)
                    {
                        BIT_SET(currentStatus.engine, BIT_ENGINE_DCC); //Mark deceleration enleanment as active.
                        accelValue = configPage2.decelAmount; //In decel, use the decel fuel amount as accelValue
                    } //Deceleration
                    //Positive TPS rate of change is Acceleration.
                    else
                    {
                        BIT_SET(currentStatus.engine, BIT_ENGINE_ACC); //Mark acceleration enrichment as active.
                        accelValue = table2D_getValue(&taeTable, currentStatus.tpsDOT / 10); //The x-axis of tae table is divided by 10 to fit values in byte.
                        //Apply the RPM taper to the above
                        //The RPM settings are stored divided by 100:
                        uint16_t trueTaperMin = configPage2.aeTaperMin * 100;
                        uint16_t trueTaperMax = configPage2.aeTaperMax * 100;
                        if (currentStatus.RPM > trueTaperMin)
                        {
                            if (currentStatus.RPM > trueTaperMax) { accelValue = 0; } //RPM is beyond taper max limit, so accel enrich is turned off
                            else
                            {
                                int16_t taperRange = trueTaperMax - trueTaperMin;
                                int16_t taperPercent = ((currentStatus.RPM - trueTaperMin) * 100UL) / taperRange; //The percentage of the way through the RPM taper range
                                accelValue = ((100 - taperPercent) * accelValue); //Calculate the above percentage of the calculated accel amount. 
                            }
                        }

                        //Apply AE cold coolant modifier, if CLT is less than taper end temperature
                        if (currentStatus.coolant < (int)(configPage2.aeColdTaperMax - CALIBRATION_TEMPERATURE_OFFSET))
                        {
                            //If CLT is less than taper min temp, apply full modifier on top of accelValue
                            if (currentStatus.coolant <= (int)(configPage2.aeColdTaperMin - CALIBRATION_TEMPERATURE_OFFSET))
                            {
                                uint16_t accelValue_uint = (configPage2.aeColdPct * accelValue) / 100;
                                accelValue = (int16_t)accelValue_uint;
                            }
                            //If CLT is between taper min and max, taper the modifier value and apply it on top of accelValue
                            else
                            {
                                int16_t taperRange = (int16_t)configPage2.aeColdTaperMax - configPage2.aeColdTaperMin;
                                int16_t taperPercent = (int)((currentStatus.coolant + CALIBRATION_TEMPERATURE_OFFSET - configPage2.aeColdTaperMin) * 100) / taperRange;
                                int16_t coldPct = (int16_t)100 + (((100 - taperPercent) * (configPage2.aeColdPct - 100)) / 100);
                                uint16_t accelValue_uint = (uint16_t)accelValue * coldPct / 100; //Potential overflow (if AE is large) without using uint16_t
                                accelValue = (int16_t)accelValue_uint;
                            }
                        }
                        accelValue = 100 + accelValue; //In case of AE, add the 100 normalisation to the calculated amount
                    } //Acceleration
                } //TAE Threshold
            } //TPS change threshold
        } //AE Mode
    } //AE active

    return accelValue;
}

byte correctionFloodClear()
{
    byte floodValue = 100;
    if (BIT_CHECK(currentStatus.engine, BIT_ENGINE_CRANK))
    {
        //Engine is currently cranking, check what the TPS is
        if (currentStatus.TPS >= configPage4.floodClear)
        {
            //Engine is cranking and TPS is above threshold. Cut all fuel
            floodValue = 0;
        }
    }
    return floodValue;
}

uint32_t AFRnextCycle;

byte correctionAFRClosedLoop(float dt, Engine* engine)
{
    byte AFRValue = 100;

    if ((configPage6.egoType > 0) || (configPage2.incorporateAFR == true)) //afrTarget value lookup must be done if O2 sensor is enabled, and always if incorporateAFR is enabled
    {
        currentStatus.afrTarget = currentStatus.O2; //Catch all in case the below doesn't run. This prevents the Include AFR option from doing crazy things if the AFR target conditions aren't met. This value is changed again below if all conditions are met.

        //Determine whether the Y axis of the AFR target table tshould be MAP (Speed-Density) or TPS (Alpha-N)
        //Note that this should only run after the sensor warmup delay when using Include AFR option, but on Incorporate AFR option it needs to be done at all times
        if ((currentStatus.runSecs > configPage6.ego_sdelay) || (configPage2.incorporateAFR == true)) { currentStatus.afrTarget = get3DTableValue(&afrTable, currentStatus.fuelLoad, currentStatus.RPM); } //Perform the target lookup
    }

    if ((configPage6.egoType > 0) && (BIT_CHECK(currentStatus.status1, BIT_STATUS1_DFCO) != 1)) //egoType of 0 means no O2 sensor. If DFCO is active do not run the ego controllers to prevent interator wind-up.
    {
        AFRValue = currentStatus.egoCorrection; //Need to record this here, just to make sure the correction stays 'on' even if the nextCycle count isn't ready

        if ((engine->getIgnitionModule()->m_ignitionCount >= AFRnextCycle) || (engine->getIgnitionModule()->m_ignitionCount < (AFRnextCycle - configPage6.egoCount)))
        {
            AFRnextCycle = engine->getIgnitionModule()->m_ignitionCount + configPage6.egoCount; //Set the target ignition event for the next calculation

            //Check all other requirements for closed loop adjustments
            if ((currentStatus.coolant > (int)(configPage6.egoTemp - CALIBRATION_TEMPERATURE_OFFSET)) && (currentStatus.RPM > (unsigned int)(configPage6.egoRPM * 100)) && (currentStatus.TPS <= configPage6.egoTPSMax) && (currentStatus.O2 < configPage6.ego_max) && (currentStatus.O2 > configPage6.ego_min) && (currentStatus.runSecs > configPage6.ego_sdelay) && (BIT_CHECK(currentStatus.status1, BIT_STATUS1_DFCO) == 0))
            {

                //Check which algorithm is used, simple or PID
                if (configPage6.egoAlgorithm == EGO_ALGORITHM_SIMPLE)
                {
                    //*************************************************************************************************************************************
                    //Simple algorithm
                    if (currentStatus.O2 > currentStatus.afrTarget)
                    {
                        //Running lean
                        if (currentStatus.egoCorrection < (100 + configPage6.egoLimit)) //Fuelling adjustment must be at most the egoLimit amount (up or down)
                        {
                            AFRValue = (currentStatus.egoCorrection + 1); //Increase the fuelling by 1%
                        }
                        else { AFRValue = currentStatus.egoCorrection; } //Means we're at the maximum adjustment amount, so simply return that again
                    }
                    else if (currentStatus.O2 < currentStatus.afrTarget)
                    {
                        //Running Rich
                        if (currentStatus.egoCorrection > (100 - configPage6.egoLimit)) //Fuelling adjustment must be at most the egoLimit amount (up or down)
                        {
                            AFRValue = (currentStatus.egoCorrection - 1); //Decrease the fuelling by 1%
                        }
                        else { AFRValue = currentStatus.egoCorrection; } //Means we're at the maximum adjustment amount, so simply return that again
                    }
                    else { AFRValue = currentStatus.egoCorrection; } //Means we're already right on target

                }
                else if (configPage6.egoAlgorithm == EGO_ALGORITHM_PID)
                {
                    //*************************************************************************************************************************************
                    //PID algorithm
                    // hail naw
                    /*egoPID.SetOutputLimits((long)(-configPage6.egoLimit), (long)(configPage6.egoLimit)); //Set the limits again, just in case the user has changed them since the last loop. Note that these are sent to the PID library as (Eg:) -15 and +15
                    egoPID.SetTunings(configPage6.egoKP, configPage6.egoKI, configPage6.egoKD); //Set the PID values again, just in case the user has changed them since the last loop
                    PID_O2 = (long)(currentStatus.O2);
                    PID_AFRTarget = (long)(currentStatus.afrTarget);

                    bool PID_compute = egoPID.Compute();
                    //currentStatus.egoCorrection = 100 + PID_output;
                    if (PID_compute == true) { AFRValue = 100 + PID_output; }
                    
                }
                else { AFRValue = 100; } // Occurs if the egoAlgorithm is set to 0 (No Correction)
            } //Multi variable check 
            else { AFRValue = 100; } // If multivariable check fails disable correction
        } //Ignition count check
    } //egoType

    return AFRValue; //Catch all (Includes when AFR target = current AFR
}

byte correctionBatVoltage()
{
    byte batValue = 100;
    batValue = table2D_getValue(&injectorVCorrectionTable, currentStatus.battery10);
    return batValue;
}

byte correctionIATDensity(void)
{
    byte IATValue = 100;
    IATValue = table2D_getValue(&IATDensityCorrectionTable, currentStatus.IAT + CALIBRATION_TEMPERATURE_OFFSET); //currentStatus.IAT is the actual temperature, values in IATDensityCorrectionTable.axisX are temp+offset

    return IATValue;
}

byte correctionBaro()
{
    byte baroValue = 100;
    baroValue = table2D_getValue(&baroFuelTable, currentStatus.baro);

    return baroValue;
}

byte correctionFlex()
{
    byte flexValue = 100;

    if (configPage2.flexEnabled == 1)
    {
        flexValue = table2D_getValue(&flexFuelTable, currentStatus.ethanolPct);
    }
    return flexValue;
}

byte correctionLaunch()
{
    byte launchValue = 100;
    if (currentStatus.launchingHard || currentStatus.launchingSoft) { launchValue = (100 + configPage6.lnchFuelAdd); }

    return launchValue;
}

float dfcoTaper = 0;

bool correctionDFCO(float dt)
{
    bool DFCOValue = false;
    if (configPage2.dfcoEnabled == 1) {
        if (BIT_CHECK(currentStatus.status1, BIT_STATUS1_DFCO) == 1) {
            DFCOValue = (currentStatus.RPM > (configPage4.dfcoRPM * 10)) && (currentStatus.TPS < configPage4.dfcoTPSThresh);
            if (DFCOValue == false) { dfcoTaper = 0; }
        }
        else {
            if ((currentStatus.TPS < configPage4.dfcoTPSThresh) && (currentStatus.coolant >= (int)(configPage2.dfcoMinCLT - CALIBRATION_TEMPERATURE_OFFSET)) && (currentStatus.RPM > (unsigned int)((configPage4.dfcoRPM * 10) + configPage4.dfcoHyster))) {
                if (dfcoTaper < configPage2.dfcoDelay) {
                    dfcoTaper += dt;
                }
                else { DFCOValue = true; }
            }
            else { dfcoTaper = 0; } //Prevent future activation right away if previous time wasn't activated
        } // DFCO active check
    } // DFCO enabled check
    return DFCOValue;
}

// calculate fuel corrections
uint32_t EngineSimApplication::fuelCorrections(float dt) {
    uint32_t sumCorrections = 100;
    byte numCorrections = 0;

    currentStatus.wueCorrection = correctionWUE(dt); numCorrections++;
    currentStatus.ASEValue = correctionASE(dt); numCorrections++;
    uint16_t correctionCrankingValue = correctionCranking(dt); numCorrections++;
    currentStatus.AEamount = correctionAccel(dt); numCorrections++;
    uint8_t correctionFloodClearValue = correctionFloodClear(); numCorrections++;
    currentStatus.egoCorrection = correctionAFRClosedLoop(dt, m_iceEngine); numCorrections++;

    currentStatus.batCorrection = correctionBatVoltage(); numCorrections++;
    currentStatus.iatCorrection = correctionIATDensity(); numCorrections++;
    currentStatus.baroCorrection = correctionBaro(); numCorrections++;
    currentStatus.flexCorrection = correctionFlex(); numCorrections++;
    currentStatus.launchCorrection = correctionLaunch(); numCorrections++;

    bool dfco = correctionDFCO(dt);
    if (dfco) {
        BIT_SET(currentStatus.status1, BIT_STATUS1_DFCO);
        return 0;
    }
    else {
        BIT_CLEAR(currentStatus.status1, BIT_STATUS1_DFCO);
    }

    sumCorrections = currentStatus.wueCorrection \
        + currentStatus.ASEValue \
        + correctionCrankingValue \
        + currentStatus.AEamount \
        + correctionFloodClearValue \
        + currentStatus.batCorrection \
        + currentStatus.iatCorrection \
        + currentStatus.baroCorrection \
        + currentStatus.flexCorrection \
        + currentStatus.launchCorrection;
    return (sumCorrections);
}*/


/**
 * @brief Looks up and returns the VE value from the secondary fuel table
 *
 * This performs largely the same operations as getVE() however the lookup is of the secondary fuel table and uses the secondary load source
 * @return byte
 */
byte getVE2(void)
{
    byte tempVE = 100;
    if (configPage10.fuel2Algorithm == LOAD_SOURCE_MAP) {
        //Speed Density
        currentStatus.fuelLoad2 = currentStatus.MAP;
    }
    else if (configPage10.fuel2Algorithm == LOAD_SOURCE_TPS) {
        //Alpha-N
        currentStatus.fuelLoad2 = currentStatus.TPS * 2;
    }
    else if (configPage10.fuel2Algorithm == LOAD_SOURCE_IMAPEMAP) {
        //IMAP / EMAP
        currentStatus.fuelLoad2 = (currentStatus.MAP * 100) / currentStatus.EMAP;
    }
    else { currentStatus.fuelLoad2 = currentStatus.MAP; } //Fallback position
    tempVE = get3DTableValue(&fuelTable2, currentStatus.fuelLoad2, currentStatus.RPM); //Perform lookup into fuel map for RPM vs MAP value

    return tempVE;
}

/**
 * @brief Performs a lookup of the second ignition advance table. The values used to look this up will be RPM and whatever load source the user has configured
 *
 * @return byte The current target advance value in degrees
 */
byte getAdvance2(void)
{
    byte tempAdvance = 0;
    if (configPage10.spark2Algorithm == LOAD_SOURCE_MAP) { //Check which fuelling algorithm is being used
        //Speed Density
        currentStatus.ignLoad2 = currentStatus.MAP;
    }
    else if (configPage10.spark2Algorithm == LOAD_SOURCE_TPS) {
        //Alpha-N
        currentStatus.ignLoad2 = currentStatus.TPS * 2;
    }
    else if (configPage10.spark2Algorithm == LOAD_SOURCE_IMAPEMAP) {
        //IMAP / EMAP
        currentStatus.ignLoad2 = (currentStatus.MAP * 100) / currentStatus.EMAP;
    }
    else { currentStatus.ignLoad2 = currentStatus.MAP; }
    tempAdvance = get3DTableValue(&ignitionTable2, currentStatus.ignLoad2, currentStatus.RPM) - OFFSET_IGNITION; //As above, but for ignition advance
    //tempAdvance = correctionsIgn(tempAdvance);

    return tempAdvance;
}

void calculateFuel(void)
{
    //If the secondary fuel table is in use, also get the VE value from there
    BIT_CLEAR(currentStatus.status3, BIT_STATUS3_FUEL2_ACTIVE); //Clear the bit indicating that the 2nd fuel table is in use. 
    
    currentStatus.VE1 = get3DTableValue(&fuelTable, currentStatus.fuelLoad, currentStatus.RPM);
    currentStatus.VE = currentStatus.VE1;

    if (configPage10.fuel2Mode > 0)
    {
        if (configPage10.fuel2Mode == FUEL2_MODE_MULTIPLY)
        {
            currentStatus.VE2 = getVE2();
            //Fuel 2 table is treated as a % value. Table 1 and 2 are multiplied together and divided by 100
            uint16_t combinedVE = ((uint16_t)currentStatus.VE1 * (uint16_t)currentStatus.VE2) / 100;
            if (combinedVE <= 255) { currentStatus.VE = combinedVE; }
            else { currentStatus.VE = 255; }
        }
        else if (configPage10.fuel2Mode == FUEL2_MODE_ADD)
        {
            currentStatus.VE2 = getVE2();
            //Fuel tables are added together, but a check is made to make sure this won't overflow the 8-bit VE value
            uint16_t combinedVE = (uint16_t)currentStatus.VE1 + (uint16_t)currentStatus.VE2;
            if (combinedVE <= 255) { currentStatus.VE = combinedVE; }
            else { currentStatus.VE = 255; }
        }
        else if (configPage10.fuel2Mode == FUEL2_MODE_CONDITIONAL_SWITCH)
        {
            if (configPage10.fuel2SwitchVariable == FUEL2_CONDITION_RPM)
            {
                if (currentStatus.RPM > configPage10.fuel2SwitchValue)
                {
                    BIT_SET(currentStatus.status3, BIT_STATUS3_FUEL2_ACTIVE); //Set the bit indicating that the 2nd fuel table is in use. 
                    currentStatus.VE2 = getVE2();
                    currentStatus.VE = currentStatus.VE2;
                }
            }
            else if (configPage10.fuel2SwitchVariable == FUEL2_CONDITION_MAP)
            {
                if (currentStatus.MAP > configPage10.fuel2SwitchValue)
                {
                    BIT_SET(currentStatus.status3, BIT_STATUS3_FUEL2_ACTIVE); //Set the bit indicating that the 2nd fuel table is in use. 
                    currentStatus.VE2 = getVE2();
                    currentStatus.VE = currentStatus.VE2;
                }
            }
            else if (configPage10.fuel2SwitchVariable == FUEL2_CONDITION_TPS)
            {
                if (currentStatus.TPS > configPage10.fuel2SwitchValue)
                {
                    BIT_SET(currentStatus.status3, BIT_STATUS3_FUEL2_ACTIVE); //Set the bit indicating that the 2nd fuel table is in use. 
                    currentStatus.VE2 = getVE2();
                    currentStatus.VE = currentStatus.VE2;
                }
            }
            else if (configPage10.fuel2SwitchVariable == FUEL2_CONDITION_ETH)
            {
                if (currentStatus.ethanolPct > configPage10.fuel2SwitchValue)
                {
                    BIT_SET(currentStatus.status3, BIT_STATUS3_FUEL2_ACTIVE); //Set the bit indicating that the 2nd fuel table is in use. 
                    currentStatus.VE2 = getVE2();
                    currentStatus.VE = currentStatus.VE2;
                }
            }
        }
        /* Not supported
        else if (configPage10.fuel2Mode == FUEL2_MODE_INPUT_SWITCH)
        {
            if (digitalRead(pinFuel2Input) == configPage10.fuel2InputPolarity)
            {
                BIT_SET(currentStatus.status3, BIT_STATUS3_FUEL2_ACTIVE); //Set the bit indicating that the 2nd fuel table is in use. 
                currentStatus.VE2 = getVE2();
                currentStatus.VE = currentStatus.VE2;
            }
        }
        */
    }
}

void calculateSpark(void)
{
    //Same as above but for the secondary ignition table
    BIT_CLEAR(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE); //Clear the bit indicating that the 2nd spark table is in use. 
    
    currentStatus.advance1 = get3DTableValue(&ignitionTable, currentStatus.ignLoad, currentStatus.RPM) - 40;
    currentStatus.advance = currentStatus.advance1;

    if (configPage10.spark2Mode > 0)
    {
        if (configPage10.spark2Mode == SPARK2_MODE_MULTIPLY)
        {
            BIT_SET(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE);
            currentStatus.advance2 = getAdvance2();
            //make sure we don't have a negative value in the multiplier table (sharing a signed 8 bit table)
            if (currentStatus.advance2 < 0) { currentStatus.advance2 = 0; }
            //Spark 2 table is treated as a % value. Table 1 and 2 are multiplied together and divided by 100
            int16_t combinedAdvance = ((int16_t)currentStatus.advance1 * (int16_t)currentStatus.advance2) / 100;
            //make sure we don't overflow and accidentally set negative timing, currentStatus.advance can only hold a signed 8 bit value
            if (combinedAdvance <= 127) { currentStatus.advance = combinedAdvance; }
            else { currentStatus.advance = 127; }
        }
        else if (configPage10.spark2Mode == SPARK2_MODE_ADD)
        {
            BIT_SET(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE); //Set the bit indicating that the 2nd spark table is in use. 
            currentStatus.advance2 = getAdvance2();
            //Spark tables are added together, but a check is made to make sure this won't overflow the 8-bit VE value
            int16_t combinedAdvance = (int16_t)currentStatus.advance1 + (int16_t)currentStatus.advance2;
            //make sure we don't overflow and accidentally set negative timing, currentStatus.advance can only hold a signed 8 bit value
            if (combinedAdvance <= 127) { currentStatus.advance = combinedAdvance; }
            else { currentStatus.advance = 127; }
        }
        else if (configPage10.spark2Mode == SPARK2_MODE_CONDITIONAL_SWITCH)
        {
            if (configPage10.spark2SwitchVariable == SPARK2_CONDITION_RPM)
            {
                if (currentStatus.RPM > configPage10.spark2SwitchValue)
                {
                    BIT_SET(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE); //Set the bit indicating that the 2nd spark table is in use. 
                    currentStatus.advance2 = getAdvance2();
                    currentStatus.advance = currentStatus.advance2;
                }
            }
            else if (configPage10.spark2SwitchVariable == SPARK2_CONDITION_MAP)
            {
                if (currentStatus.MAP > configPage10.spark2SwitchValue)
                {
                    BIT_SET(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE); //Set the bit indicating that the 2nd spark table is in use. 
                    currentStatus.advance2 = getAdvance2();
                    currentStatus.advance = currentStatus.advance2;
                }
            }
            else if (configPage10.spark2SwitchVariable == SPARK2_CONDITION_TPS)
            {
                if (currentStatus.TPS > configPage10.spark2SwitchValue)
                {
                    BIT_SET(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE); //Set the bit indicating that the 2nd spark table is in use. 
                    currentStatus.advance2 = getAdvance2();
                    currentStatus.advance = currentStatus.advance2;
                }
            }
            else if (configPage10.spark2SwitchVariable == SPARK2_CONDITION_ETH)
            {
                if (currentStatus.ethanolPct > configPage10.spark2SwitchValue)
                {
                    BIT_SET(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE); //Set the bit indicating that the 2nd spark table is in use. 
                    currentStatus.advance2 = getAdvance2();
                    currentStatus.advance = currentStatus.advance2;
                }
            }
        }
        /* Not supported
        else if (configPage10.spark2Mode == SPARK2_MODE_INPUT_SWITCH)
        {
            if (digitalRead(pinSpark2Input) == configPage10.spark2InputPolarity)
            {
                BIT_SET(currentStatus.spark2, BIT_SPARK2_SPARK2_ACTIVE); //Set the bit indicating that the 2nd spark table is in use. 
                currentStatus.advance2 = getAdvance2();
                currentStatus.advance = currentStatus.advance2;
            }
        }
        */

        //Apply the fixed timing correction manually. This has to be done again here if any of the above conditions are met to prevent any of the seconadary calculations applying instead of fixec timing
        // this is applied in the ecmProcess function
        //currentStatus.advance = correctionFixedTiming(currentStatus.advance);
        //currentStatus.advance = correctionCrankingFixedTiming(currentStatus.advance); //This overrides the regular fixed timing, must come last
    }
}

float aseTime = 0;
float aseMaxTime = 0;
float aseTaperTime = 0;

void EngineSimApplication::ecmProcess(float dt) {
    float tps = 1 - m_iceEngine->getThrottle();

    // Count the time from starting the engine
    if (countRuntime)
        runtime += dt;

    #pragma region 2step and 3 step

    if (currentStatus.clutchEngagedRPM < (configPage6.flatSArm * 100) && m_targetClutchPressure <= 0.0 && tps >= (configPage10.lnchCtrlTPS / 255) && configPage6.launchEnabled) {
        m_iceEngine->getIgnitionModule()->m_2stepSoftCutAngle = configPage6.lnchRetard;
        m_iceEngine->getIgnitionModule()->m_2stepSoftCutLimit = configPage6.lnchSoftLim * 100;
        m_iceEngine->getIgnitionModule()->m_2stepHardCutLimit = configPage6.lnchHardLim * 100;
        m_iceEngine->getIgnitionModule()->m_2stepEnabled = true;
    }
    else if (currentStatus.clutchEngagedRPM > (configPage6.flatSArm * 100) && m_targetClutchPressure <= 0.0 && configPage6.flatSEnable) {
        m_iceEngine->getIgnitionModule()->m_3stepSoftCutAngle = configPage6.flatSRetard;
        m_iceEngine->getIgnitionModule()->m_3stepSoftCutLimit = configPage6.flatSSoftWin * 100;
        m_iceEngine->getIgnitionModule()->m_3stepEnabled = true;
    }
    else {
        m_iceEngine->getIgnitionModule()->m_2stepEnabled = false;
        m_iceEngine->getIgnitionModule()->m_3stepEnabled = false;
        m_iceEngine->getIgnitionModule()->m_retard = false;
        m_iceEngine->getIgnitionModule()->m_retardAmount = 0;
    }

    #pragma endregion

    #pragma region Soft/Hard revlimit

    if (units::rpm(m_iceEngine->getRpm()) > configPage4.SoftRevLim * 10) {
        m_iceEngine->getIgnitionModule()->m_retard = true;
        m_iceEngine->getIgnitionModule()->m_retardAmount = -configPage4.SoftLimRetard;
        BIT_SET(currentStatus.spark, BIT_SPARK_SFTLIM);
        BIT_SET(currentStatus.engineProtectStatus, ENGINE_PROTECT_BIT_RPM);
        if (units::rpm(m_iceEngine->getRpm()) > configPage4.HardRevLim * 10) {
            m_iceEngine->getIgnitionModule()->m_revLimitTimer = 1;
            m_iceEngine->getIgnitionModule()->m_revLimit = 99999;
            BIT_SET(currentStatus.spark, BIT_SPARK_HRDLIM);
        }
        else {
            if (!m_iceEngine->getIgnitionModule()->m_2stepEnabled && !m_iceEngine->getIgnitionModule()->m_3stepEnabled) {
                m_iceEngine->getIgnitionModule()->m_retard = false;
                m_iceEngine->getIgnitionModule()->m_retardAmount = 0;
                m_iceEngine->getIgnitionModule()->m_revLimitTimer = 0;
            }
            BIT_CLEAR(currentStatus.spark, BIT_SPARK_HRDLIM);
        }
        m_iceEngine->getIgnitionModule()->m_limiter = true;
    }
    else if (units::rpm(m_iceEngine->getRpm()) > configPage4.HardRevLim * 10) {
        m_iceEngine->getIgnitionModule()->m_retard = true;
        m_iceEngine->getIgnitionModule()->m_retardAmount = -configPage4.SoftLimRetard;
        m_iceEngine->getIgnitionModule()->m_revLimitTimer = 1;
        m_iceEngine->getIgnitionModule()->m_revLimit = 99999;
        m_iceEngine->getIgnitionModule()->m_limiter = true;
        BIT_SET(currentStatus.spark, BIT_SPARK_HRDLIM);
        BIT_SET(currentStatus.engineProtectStatus, ENGINE_PROTECT_BIT_RPM);
    }
    else {
        if (!m_iceEngine->getIgnitionModule()->m_2stepEnabled && !m_iceEngine->getIgnitionModule()->m_3stepEnabled) {
            m_iceEngine->getIgnitionModule()->m_retard = false;
            m_iceEngine->getIgnitionModule()->m_retardAmount = 0;
            m_iceEngine->getIgnitionModule()->m_revLimitTimer = 0;
        }
        BIT_CLEAR(currentStatus.spark, BIT_SPARK_SFTLIM);
        BIT_CLEAR(currentStatus.spark, BIT_SPARK_HRDLIM);
        BIT_CLEAR(currentStatus.engineProtectStatus, ENGINE_PROTECT_BIT_RPM);
        // disable stock rev limiter
        // m_iceEngine->getIgnitionModule()->m_revLimit = 99999;
        m_iceEngine->getIgnitionModule()->m_limiter = false;
    }

    #pragma endregion

    //std::stringstream ss;
    //ss << m_iceEngine->getIgnitionModule()->m_revLimitTimer << ":" << m_iceEngine->getIgnitionModule()->m_limiter << " : " << m_iceEngine->getIgnitionModule()->m_retardAmount << ":" << m_iceEngine->getIgnitionModule()->m_retard << " : " << m_iceEngine->getIgnitionModule()->m_currentTableValue;
    //m_infoCluster->setLogMessage(ss.str());

    #pragma region Crank/Run engine flags

    if (m_simulator->m_starterMotor.m_enabled && currentStatus.RPM < (configPage4.crankRPM * 10)) {
        BIT_SET(currentStatus.engine, BIT_ENGINE_CRANK);

        // ASE Time setup
        aseTaperTime = configPage2.aseTaperTime / 10; // Config is in 100ms and not 1s
        aseMaxTime = table2D_getValue(&ASECountTable, currentStatus.coolant); // Get regular time from table
        aseTime = aseTaperTime + aseMaxTime; // Combine time
        
        // enable runtime count thing
        countRuntime = true;
        runtime = 0.0f;
    }
    else
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_CRANK);

    if (!m_iceEngine->getIgnitionModule()->m_enabled && countRuntime)
        countRuntime = false;

    if (currentStatus.RPM > (configPage4.crankRPM * 10) && !m_simulator->m_starterMotor.m_enabled) {
        BIT_SET(currentStatus.engine, BIT_ENGINE_RUN);
        countRuntime = true;
    }
    else
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_RUN);

    #pragma endregion

    // Put the current VE/fuel value in currentStatus.VE (current VE in TS)
    calculateFuel();
    //float fuel = get3DTableValue(&fuelTable, currentStatus.fuelLoad, currentStatus.RPM);
    float correction = 100;

    #pragma region Fuel Corrections

    // Warmup Enrichment (WUE)

    float wue = 1000;
    if (currentStatus.coolant > (table2D_getAxisValue(&WUETable, 9) - CALIBRATION_TEMPERATURE_OFFSET)) {
        //This prevents us doing the 2D lookup if we're already up to temp
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_WARMUP);
        wue = table2D_getRawValue(&WUETable, 9);
    }
    else {
        BIT_SET(currentStatus.engine, BIT_ENGINE_WARMUP);
        wue = table2D_getValue(&WUETable, currentStatus.coolant);
    }
    currentStatus.wueCorrection = wue;

    // After start enrichment (ASE)

    float ase = 0;
    ase = table2D_getValue(&ASETable, currentStatus.coolant);
    aseTime -= dt;
    if (aseTime <= aseTaperTime && aseTime >= 0) {
        // Taper ASE value
        ase *= aseTime / aseTaperTime;
        BIT_SET(currentStatus.engine, BIT_ENGINE_ASE);
    }
    else if (aseTime <= 0) {
        aseTime = 0;
        ase = 0;
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ASE);
    }
    else {
        BIT_SET(currentStatus.engine, BIT_ENGINE_ASE);
    }
    currentStatus.ASEValue = ase;
    
    correction = wue + ase;

    BIT_CLEAR(currentStatus.status1, BIT_STATUS1_DFCO);
    if (configPage2.dfcoEnabled) {
        if (currentStatus.TPS < configPage4.dfcoTPSThresh) {
            if (currentStatus.RPM > configPage4.dfcoRPM * 10) {
                BIT_SET(currentStatus.status1, BIT_STATUS1_DFCO);
                correction = 0;
            }
        }
    }

    #pragma endregion

    currentStatus.VE *= correction / 100;
    currentStatus.corrections = correction;

    // Put the current spark value in currentStatus.advance (current advance in TS)
    calculateSpark();
    //float ign = get3DTableValue(&ignitionTable, currentStatus.ignLoad, currentStatus.RPM);
    
    #pragma region Ignition Corrections

    //float idleAdvance = table2D_getValue(&idleAdvanceTable, currentStatus.rpmDOT);
    float iatRetard = -table2D_getValue(&IATRetardTable, currentStatus.IAT - CALIBRATION_TEMPERATURE_OFFSET);

    currentStatus.advance += iatRetard;

    float idleTarget = table2D_getValue(&idleTargetTable, currentStatus.coolant - CALIBRATION_TEMPERATURE_OFFSET);
    if (configPage2.fixAngEnable == 1) { currentStatus.advance = configPage4.FixAng; } //Check whether the user has set a fixed timing angle
    if (BIT_CHECK(currentStatus.engine, BIT_ENGINE_CRANK))
    {
        if (configPage2.crkngAddCLTAdv == 0) { currentStatus.advance = configPage4.CrankAng; } //Use the fixed cranking ignition angle
        else { currentStatus.advance = configPage4.CrankAng + (table2D_getValue(&CLTAdvanceTable, currentStatus.coolant - CALIBRATION_TEMPERATURE_OFFSET) - 15); } //Use the CLT compensated cranking ignition angle
    }

    #pragma endregion

    m_iceEngine->getIgnitionModule()->m_currentTableValue = currentStatus.advance;
    
    for (int i = 0; i < m_iceEngine->getIntakeCount(); i++)
        m_iceEngine->getIntake(i)->m_fuelInjectAmount = currentStatus.VE * (9 * 1.25);
    
    float pw = currentStatus.VE * (9 * 1.25);
    pw /= 50;
    pw *= 1000;
    currentStatus.PW1 = pw;
    currentStatus.PW2 = pw;
    currentStatus.PW3 = pw;
    currentStatus.PW4 = pw;
    currentStatus.PW5 = pw;
    currentStatus.PW6 = pw;
    currentStatus.PW7 = pw;
    currentStatus.PW8 = pw;
}

void EngineSimApplication::ecmStatus(float dt) {
    IgnitionModule* ign = m_iceEngine->getIgnitionModule();

    //std::stringstream ss;
    //ss << "BOOST: " << m_simulator->m_turbo.m_boost << " RPM: " << m_simulator->m_turbo.GetRotorRPM();
    //m_infoCluster->setLogMessage(ss.str());

    long prevRPM = currentStatus.longRPM;
    currentStatus.rpmDOT = m_iceEngine->getRpm() - prevRPM;
    currentStatus.longRPM = m_iceEngine->getRpm();
    currentStatus.RPM = currentStatus.longRPM;
    currentStatus.RPMdiv100 = currentStatus.longRPM / 100;

    #pragma region Intake Related Statuses

    float tps = 1 - m_iceEngine->getThrottle();
    currentStatus.TPSlast = currentStatus.TPS;
    currentStatus.TPS = std::round(200.0 * tps);
    currentStatus.tpsDOT = (currentStatus.TPS - currentStatus.TPSlast) / dt;
    currentStatus.tpsADC = std::round(255.0 * tps);

    MAPlast = currentStatus.MAP;
    currentStatus.MAP = (m_iceEngine->getManifoldPressure() * 100) / 102000;
    currentStatus.mapDOT = (currentStatus.MAP - MAPlast) / dt;

    if (configPage2.fuelAlgorithm == LOAD_SOURCE_MAP)
        currentStatus.fuelLoad = currentStatus.MAP;
    else if (configPage2.fuelAlgorithm == LOAD_SOURCE_TPS)
        currentStatus.fuelLoad = currentStatus.TPS * 2;
    else if (configPage2.fuelAlgorithm == LOAD_SOURCE_IMAPEMAP)
        currentStatus.fuelLoad = (currentStatus.MAP * 100) / currentStatus.EMAP;
    else { currentStatus.fuelLoad = currentStatus.MAP; }

    if (configPage2.ignAlgorithm == LOAD_SOURCE_MAP)
        currentStatus.ignLoad = currentStatus.MAP;
    else if (configPage2.ignAlgorithm == LOAD_SOURCE_TPS)
        currentStatus.ignLoad = currentStatus.TPS * 2;
    else if (configPage2.ignAlgorithm == LOAD_SOURCE_IMAPEMAP)
        currentStatus.ignLoad = (currentStatus.MAP * 100) / currentStatus.EMAP;
    else { currentStatus.ignLoad = currentStatus.MAP; }

    //currentStatus.VE = m_iceEngine->getIntake(0)->m_fuelInjectAmount / (9 * 1.25);
    #pragma endregion

    float rpmRed = (units::rpm(m_iceEngine->getRpm()) / m_iceEngine->getRedline());
    //tscpp.currentStatus.battery10 = ((units::rpm(m_iceEngine->getRpm()) / m_iceEngine->getRedline()) * 44) + 124;
    currentStatus.battery10 = (rpmRed * 24) + 120;

    //currentStatus.advance = (ign->getTimingAdvance() / units::deg) * (m_iceEngine->isSpinningCw() ? 1 : -1);
    //currentStatus.advance1 = currentStatus.advance;
    //currentStatus.advance2 = currentStatus.advance;
  
    currentStatus.launchingHard = ign->m_launchingHard;
    currentStatus.launchingSoft = ign->m_launchingSoft;
    if (currentStatus.launchingHard) {
        BIT_SET(currentStatus.spark, BIT_SPARK_HLAUNCH);
    }
    else {
        BIT_CLEAR(currentStatus.spark, BIT_SPARK_HLAUNCH);
    }

    if (currentStatus.launchingSoft) {
        BIT_SET(currentStatus.spark, BIT_SPARK_SLAUNCH);
    }
    else {
        BIT_CLEAR(currentStatus.spark, BIT_SPARK_SLAUNCH);
    }

    currentStatus.flatShiftingHard = ign->m_shiftingHard;

    if (currentStatus.TPS >= 100) {
        BIT_SET(currentStatus.engine, BIT_ENGINE_ACC);
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_DCC);
    }
    else if (currentStatus.RPM > 1000) { // if higher than idle
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ACC);
        BIT_SET(currentStatus.engine, BIT_ENGINE_DCC);
    }
    else {
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_ACC);
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_DCC);
    }

    if (currentStatus.MAP >= 90) {
        BIT_SET(currentStatus.engine, BIT_ENGINE_MAPACC);
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_MAPDCC);
    }
    else if (currentStatus.RPM > 1000) { // if higher than idle
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_MAPACC);
        BIT_SET(currentStatus.engine, BIT_ENGINE_MAPDCC);
    }
    else {
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_MAPACC);
        BIT_CLEAR(currentStatus.engine, BIT_ENGINE_MAPDCC);
    }

    currentStatus.gear = m_transmission->getGear() + 1;
    currentStatus.vss = m_vehicle->getSpeed() / (units::km / units::hour);
    currentStatus.coolant = m_simulator->getCoolantTemperature() + 40; // + 40 to support negatives?
    std::stringstream ss;
    ss << "BKT: " << m_simulator->getTemperature() << " CLT: " << m_simulator->getCoolantTemperature();
    m_infoCluster->setLogMessage(ss.str());

    currentStatus.IAT = 24 + 40; // shows up as 24c (intake temp of ES)
    currentStatus.fuelPumpOn = ign->m_enabled;
    currentStatus.O2 = 255;
    currentStatus.O2ADC = 255;
    currentStatus.nitrous_status = false;

    currentStatus.CTPSActive = 0;
    currentStatus.EMAP = 0;
    currentStatus.EMAPADC = 0;
    currentStatus.baro = 100;
    currentStatus.afrTarget = 147;
    currentStatus.nitrous_status = 0;
    currentStatus.nSquirts = 4;
    currentStatus.nChannels = m_iceEngine->getCylinderCount();
    currentStatus.hasSync = true;
    currentStatus.syncLossCounter = 0;
}

void EngineSimApplication::loadEngine(
    Engine *engine,
    Vehicle *vehicle,
    Transmission *transmission)
{
    destroyObjects();

    if (m_simulator != nullptr) {
        m_simulator->releaseSimulation();
        delete m_simulator;
    }

    if (m_vehicle != nullptr) {
        delete m_vehicle;
        m_vehicle = nullptr;
    }

    if (m_transmission != nullptr) {
        delete m_transmission;
        m_transmission = nullptr;
    }

    if (m_iceEngine != nullptr) {
        m_iceEngine->destroy();
        delete m_iceEngine;
    }

    m_iceEngine = engine;
    m_vehicle = vehicle;
    m_transmission = transmission;

    m_simulator = engine->createSimulator(vehicle, transmission);

    if (engine == nullptr || vehicle == nullptr || transmission == nullptr) {
        m_iceEngine = nullptr;
        m_viewParameters.Layer1 = 0;

        return;
    }

    createObjects(engine);

    m_viewParameters.Layer1 = engine->getMaxDepth();
    engine->calculateDisplacement();

    m_simulator->setSimulationFrequency(engine->getSimulationFrequency());

    Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
    audioParams.inputSampleNoise = static_cast<float>(engine->getInitialJitter());
    audioParams.airNoise = static_cast<float>(engine->getInitialNoise());
    audioParams.dF_F_mix = static_cast<float>(engine->getInitialHighFrequencyGain());
    m_simulator->synthesizer().setAudioParameters(audioParams);

    for (int i = 0; i < engine->getExhaustSystemCount(); ++i) {
        ImpulseResponse *response = engine->getExhaustSystem(i)->getImpulseResponse();

        ysWindowsAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        m_simulator->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t *>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            response->getVolume(),
            i
        );

        waveFile.DestroyInternalBuffer();
    }

    m_simulator->startAudioRenderingThread();
}

void EngineSimApplication::drawGenerated(
    const GeometryGenerator::GeometryIndices &indices,
    int layer)
{
    drawGenerated(indices, layer, m_shaders.GetRegularFlags());
}

void EngineSimApplication::drawGeneratedUi(
    const GeometryGenerator::GeometryIndices &indices,
    int layer)
{
    drawGenerated(indices, layer, m_shaders.GetUiFlags());
}

void EngineSimApplication::drawGenerated(
    const GeometryGenerator::GeometryIndices &indices,
    int layer,
    dbasic::StageEnableFlags flags)
{
    m_engine.DrawGeneric(
        flags,
        m_geometryIndexBuffer,
        m_geometryVertexBuffer,
        sizeof(dbasic::Vertex),
        indices.BaseIndex,
        indices.BaseVertex,
        indices.FaceCount,
        false,
        layer);
}

void EngineSimApplication::configure(const ApplicationSettings &settings) {
    m_applicationSettings = settings;

    if (settings.startFullscreen) {
        m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Fullscreen);
    }

    m_background = ysColor::srgbiToLinear(m_applicationSettings.colorBackground);
    m_foreground = ysColor::srgbiToLinear(m_applicationSettings.colorForeground);
    m_shadow = ysColor::srgbiToLinear(m_applicationSettings.colorShadow);
    m_highlight1 = ysColor::srgbiToLinear(m_applicationSettings.colorHighlight1);
    m_highlight2 = ysColor::srgbiToLinear(m_applicationSettings.colorHighlight2);
    m_pink = ysColor::srgbiToLinear(m_applicationSettings.colorPink);
    m_red = ysColor::srgbiToLinear(m_applicationSettings.colorRed);
    m_orange = ysColor::srgbiToLinear(m_applicationSettings.colorOrange);
    m_yellow = ysColor::srgbiToLinear(m_applicationSettings.colorYellow);
    m_blue = ysColor::srgbiToLinear(m_applicationSettings.colorBlue);
    m_green = ysColor::srgbiToLinear(m_applicationSettings.colorGreen);
}

void EngineSimApplication::createObjects(Engine *engine) {
    for (int i = 0; i < engine->getCylinderCount(); ++i) {
        ConnectingRodObject *rodObject = new ConnectingRodObject;
        rodObject->initialize(this);
        rodObject->m_connectingRod = engine->getConnectingRod(i);
        m_objects.push_back(rodObject);

        PistonObject *pistonObject = new PistonObject;
        pistonObject->initialize(this);
        pistonObject->m_piston = engine->getPiston(i);
        m_objects.push_back(pistonObject);

        CombustionChamberObject *ccObject = new CombustionChamberObject;
        ccObject->initialize(this);
        ccObject->m_chamber = m_iceEngine->getChamber(i);
        m_objects.push_back(ccObject);
    }

    for (int i = 0; i < engine->getCrankshaftCount(); ++i) {
        CrankshaftObject *crankshaftObject = new CrankshaftObject;
        crankshaftObject->initialize(this);
        crankshaftObject->m_crankshaft = engine->getCrankshaft(i);
        m_objects.push_back(crankshaftObject);
    }

    for (int i = 0; i < engine->getCylinderBankCount(); ++i) {
        CylinderBankObject *cbObject = new CylinderBankObject;
        cbObject->initialize(this);
        cbObject->m_bank = engine->getCylinderBank(i);
        cbObject->m_head = engine->getHead(i);
        m_objects.push_back(cbObject);

        CylinderHeadObject *chObject = new CylinderHeadObject;
        chObject->initialize(this);
        chObject->m_head = engine->getHead(i);
        chObject->m_engine = engine;
        m_objects.push_back(chObject);
    }
}

void EngineSimApplication::destroyObjects() {
    for (SimulationObject *object : m_objects) {
        object->destroy();
        delete object;
    }

    m_objects.clear();
}

const SimulationObject::ViewParameters &
    EngineSimApplication::getViewParameters() const
{
    return m_viewParameters;
}

void EngineSimApplication::loadScript() {
    Engine *engine = nullptr;
    Vehicle *vehicle = nullptr;
    Transmission *transmission = nullptr;

#ifdef ATG_ENGINE_SIM_PIRANHA_ENABLED
    es_script::Compiler compiler;
    compiler.initialize();
    const bool compiled = compiler.compile("../assets/main.mr");
    if (compiled) {
        const es_script::Compiler::Output output = compiler.execute();
        configure(output.applicationSettings);

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
#endif /* ATG_ENGINE_SIM_PIRANHA_ENABLED */

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

    loadEngine(engine, vehicle, transmission);
    refreshUserInterface();
}

void EngineSimApplication::processEngineInput() {
    if (m_iceEngine == nullptr) {
        return;
    }

    const float dt = m_engine.GetFrameLength();
    const bool fineControlMode = m_engine.IsKeyDown(ysKey::Code::Space);

    const int mouseWheel = m_engine.GetMouseWheel();
    const int mouseWheelDelta = mouseWheel - m_lastMouseWheel;
    m_lastMouseWheel = mouseWheel;

    bool fineControlInUse = false;
    if (m_engine.IsKeyDown(ysKey::Code::Z)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.volume = clamp(audioParams.volume + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[Z] - Set volume to " + std::to_string(audioParams.volume));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::X)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.convolution = clamp(audioParams.convolution + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[X] - Set convolution level to " + std::to_string(audioParams.convolution));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::C)) {
        const double rate = fineControlMode
            ? 0.00001
            : 0.001;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.dF_F_mix = clamp(audioParams.dF_F_mix + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[C] - Set high freq. gain to " + std::to_string(audioParams.dF_F_mix));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::V)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.airNoise = clamp(audioParams.airNoise + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[V] - Set low freq. noise to " + std::to_string(audioParams.airNoise));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::B)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.inputSampleNoise = clamp(audioParams.inputSampleNoise + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[B] - Set high freq. noise to " + std::to_string(audioParams.inputSampleNoise));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N)) {
        const double rate = fineControlMode
            ? 10.0
            : 100.0;

        const double newSimulationFrequency = clamp(
            m_simulator->getSimulationFrequency() + mouseWheelDelta * rate * dt,
            400.0, 400000.0);

        m_simulator->setSimulationFrequency(newSimulationFrequency);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[N] - Set simulation freq to " + std::to_string(m_simulator->getSimulationFrequency()));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::G) && m_simulator->m_dyno.m_hold) {
        if (mouseWheelDelta > 0) {
            m_dynoSpeed += m_iceEngine->getDynoHoldStep();
        }
        else if (mouseWheelDelta < 0) {
            m_dynoSpeed -= m_iceEngine->getDynoHoldStep();
        }

        m_dynoSpeed = clamp(m_dynoSpeed, m_iceEngine->getDynoMinSpeed(), m_iceEngine->getDynoMaxSpeed());

        m_infoCluster->setLogMessage("[G] - Set dyno speed to " + std::to_string(units::toRpm(m_dynoSpeed)));
        fineControlInUse = true;
    }

    const double prevTargetThrottle = m_targetSpeedSetting;
    m_targetSpeedSetting = fineControlMode ? m_targetSpeedSetting : 0.0;
    if (m_engine.IsKeyDown(ysKey::Code::Q)) {
        m_targetSpeedSetting = 0.01;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::W)) {
        m_targetSpeedSetting = 0.1;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::E)) {
        m_targetSpeedSetting = 0.2;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::R)) {
        m_targetSpeedSetting = 1.0;
    }
    else if (fineControlMode && !fineControlInUse) {
        m_targetSpeedSetting = clamp(m_targetSpeedSetting + mouseWheelDelta * 0.0001);
    }

    if (prevTargetThrottle != m_targetSpeedSetting) {
        m_infoCluster->setLogMessage("Speed control set to " + std::to_string(m_targetSpeedSetting));
    }

    m_speedSetting = m_targetSpeedSetting * 0.5 + 0.5 * m_speedSetting;

    m_iceEngine->setSpeedControl(m_speedSetting);
    if (m_engine.ProcessKeyDown(ysKey::Code::M)) {
        const int currentLayer = getViewParameters().Layer0;
        if (currentLayer + 1 < m_iceEngine->getMaxDepth()) {
            setViewLayer(currentLayer + 1);
        }

        m_infoCluster->setLogMessage("[M] - Set render layer to " + std::to_string(getViewParameters().Layer0));
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::OEM_Comma)) {
        if (getViewParameters().Layer0 - 1 >= 0)
            setViewLayer(getViewParameters().Layer0 - 1);

        m_infoCluster->setLogMessage("[,] - Set render layer to " + std::to_string(getViewParameters().Layer0));
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::D)) {
        m_simulator->m_dyno.m_enabled = !m_simulator->m_dyno.m_enabled;

        const std::string msg = m_simulator->m_dyno.m_enabled
            ? "DYNOMOMETER ENABLED"
            : "DYNOMOMETER DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::H)) {
        m_simulator->m_dyno.m_hold = !m_simulator->m_dyno.m_hold;

        const std::string msg = m_simulator->m_dyno.m_hold
            ? m_simulator->m_dyno.m_enabled ? "HOLD ENABLED" : "HOLD ON STANDBY [ENABLE DYNO. FOR HOLD]"
            : "HOLD DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_simulator->m_dyno.m_enabled) {
        if (!m_simulator->m_dyno.m_hold) {
            if (m_simulator->getFilteredDynoTorque() > units::torque(1.0, units::ft_lb)) {
                m_dynoSpeed += units::rpm(500) * dt;
            }
            else {
                m_dynoSpeed *= (1 / (1 + dt));
            }

            if (m_dynoSpeed > m_iceEngine->getRedline()) {
                m_simulator->m_dyno.m_enabled = false;
                m_dynoSpeed = units::rpm(0);
            }
        }
    }
    else {
        if (!m_simulator->m_dyno.m_hold) {
            m_dynoSpeed = units::rpm(0);
        }
    }

    m_dynoSpeed = clamp(m_dynoSpeed, m_iceEngine->getDynoMinSpeed(), m_iceEngine->getDynoMaxSpeed());
    m_simulator->m_dyno.m_rotationSpeed = m_dynoSpeed;

    const bool prevStarterEnabled = m_simulator->m_starterMotor.m_enabled;
    if (m_engine.IsKeyDown(ysKey::Code::S)) {
        m_simulator->m_starterMotor.m_enabled = true;
    }
    else {
        m_simulator->m_starterMotor.m_enabled = false;
    }

    if (prevStarterEnabled != m_simulator->m_starterMotor.m_enabled) {
        const std::string msg = m_simulator->m_starterMotor.m_enabled
            ? "STARTER ENABLED"
            : "STARTER DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::A)) {
        m_simulator->getEngine()->getIgnitionModule()->m_enabled =
            !m_simulator->getEngine()->getIgnitionModule()->m_enabled;

        const std::string msg = m_simulator->getEngine()->getIgnitionModule()->m_enabled
            ? "IGNITION ENABLED"
            : "IGNITION DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::Up)) {
        m_simulator->getTransmission()->changeGear(m_simulator->getTransmission()->getGear() + 1);

        m_infoCluster->setLogMessage(
            "UPSHIFTED TO " + std::to_string(m_simulator->getTransmission()->getGear() + 1));
    }
    else if (m_engine.ProcessKeyDown(ysKey::Code::Down)) {
        m_simulator->getTransmission()->changeGear(m_simulator->getTransmission()->getGear() - 1);

        if (m_simulator->getTransmission()->getGear() != -1) {
            m_infoCluster->setLogMessage(
                "DOWNSHIFTED TO " + std::to_string(m_simulator->getTransmission()->getGear() + 1));
        }
        else {
            m_infoCluster->setLogMessage("SHIFTED TO NEUTRAL");
        }
    }

    if (m_engine.IsKeyDown(ysKey::Code::T)) {
        m_targetClutchPressure -= 0.2 * dt;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::U)) {
        m_targetClutchPressure += 0.2 * dt;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::Shift)) {
        if(m_targetClutchPressure != 0.0)
            currentStatus.clutchEngagedRPM = currentStatus.RPM;

        m_targetClutchPressure = 0.0;
        m_infoCluster->setLogMessage("CLUTCH DEPRESSED");
    }
    else if (!m_engine.IsKeyDown(ysKey::Code::Y)) {
        m_targetClutchPressure = 1.0;
    }

    m_targetClutchPressure = clamp(m_targetClutchPressure);

    double clutchRC = 0.001;
    if (m_engine.IsKeyDown(ysKey::Code::Space)) {
        clutchRC = 1.0;
    }

    const double clutch_s = dt / (dt + clutchRC);
    m_clutchPressure = m_clutchPressure * (1 - clutch_s) + m_targetClutchPressure * clutch_s;
    m_simulator->getTransmission()->setClutchPressure(m_clutchPressure);
}

void EngineSimApplication::renderScene() {
    getShaders()->ResetBaseColor();
    getShaders()->SetObjectTransform(ysMath::LoadIdentity());

    m_textRenderer.SetColor(ysColor::linearToSrgb(m_foreground));
    m_shaders.SetClearColor(ysColor::linearToSrgb(m_shadow));

    const int screenWidth = m_engine.GetGameWindow()->GetGameWidth();
    const int screenHeight = m_engine.GetGameWindow()->GetGameHeight();
    const float aspectRatio = screenWidth / (float)screenHeight;

    const Point cameraPos = m_engineView->getCameraPosition();
    m_shaders.m_cameraPosition = ysMath::LoadVector(cameraPos.x, cameraPos.y);

    m_shaders.CalculateUiCamera(screenWidth, screenHeight);

    if (m_screen == 0) {
        Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });
        Grid grid;
        grid.v_cells = 2;
        grid.h_cells = 3;
        Grid grid3x3;
        grid3x3.v_cells = 3;
        grid3x3.h_cells = 3;
        m_engineView->setDrawFrame(true);
        m_engineView->setBounds(grid.get(windowBounds, 1, 0, 1, 1));
        m_engineView->setLocalPosition({ 0, 0 });

        m_rightGaugeCluster->m_bounds = grid.get(windowBounds, 2, 0, 1, 2);
        m_oscCluster->m_bounds = grid.get(windowBounds, 1, 1);
        m_performanceCluster->m_bounds = grid3x3.get(windowBounds, 0, 1);
        m_loadSimulationCluster->m_bounds = grid3x3.get(windowBounds, 0, 2);

        Grid grid1x3;
        grid1x3.v_cells = 3;
        grid1x3.h_cells = 1;
        m_mixerCluster->m_bounds = grid1x3.get(grid3x3.get(windowBounds, 0, 0), 0, 2);
        m_infoCluster->m_bounds = grid1x3.get(grid3x3.get(windowBounds, 0, 0), 0, 0, 1, 2);

        m_loggingCluster->setVisible(false);
        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(true);
        m_oscCluster->setVisible(true);
        m_performanceCluster->setVisible(true);
        m_loadSimulationCluster->setVisible(true);
        m_mixerCluster->setVisible(true);
        m_infoCluster->setVisible(true);

        m_oscCluster->activate();
    }
    else if (m_screen == 1) {
        Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });
        m_engineView->setDrawFrame(false);
        m_engineView->setBounds(windowBounds);
        m_engineView->setLocalPosition({ 0, 0 });
        m_engineView->activate();

        m_loggingCluster->setVisible(false);
        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(false);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(false);
    }
    else if (m_screen == 2) {
        Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });
        Grid grid;
        grid.v_cells = 1;
        grid.h_cells = 3;
        m_engineView->setDrawFrame(true);
        m_engineView->setBounds(grid.get(windowBounds, 0, 0, 2, 1));
        m_engineView->setLocalPosition({ 0, 0 });
        m_engineView->activate();

        m_rightGaugeCluster->m_bounds = grid.get(windowBounds, 2, 0, 1, 1);

        m_loggingCluster->setVisible(false);
        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(true);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(false);
    }
    else if (m_screen == 3) { // TUNING PARAMETERS
        Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });
        const Bounds top = windowBounds.verticalSplit(0.75f, 1.0f);
        const Bounds bottom = windowBounds.verticalSplit(0.0f, 0.75f);
        m_engineView->setDrawFrame(false);
        //m_engineView->setBounds(grid.get(windowBounds, 0, 0, 2, 1));
        //m_engineView->setLocalPosition({ 0, 0 });
        m_engineView->activate();

        m_infoCluster->m_bounds = top;
        m_rightGaugeCluster->m_bounds = bottom;

        m_loggingCluster->setVisible(false);
        m_engineView->setVisible(false);
        m_rightGaugeCluster->setVisible(true);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(true);
    }
    else if (m_screen == 4) { // TUNING DYNO
        Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });
        const Bounds top = windowBounds.verticalSplit(0.25f, 1.0f);
        const Bounds bottom = windowBounds.verticalSplit(0.0f, 0.25f);
        //const Bounds bottomtop = bottom.verticalSplit(0.0f, 0.5f);
        //const Bounds bottombottom = bottom.verticalSplit(0.5f, 1.0f);
        m_engineView->setDrawFrame(false);
        //m_engineView->setBounds(grid.get(windowBounds, 0, 0, 2, 1));
        //m_engineView->setLocalPosition({ 0, 0 });
        m_engineView->activate();

        //m_infoCluster->m_bounds = top;
        m_rightGaugeCluster->m_bounds = top;
        m_loadSimulationCluster->m_bounds = bottom;

        m_loggingCluster->setVisible(false);
        m_engineView->setVisible(false);
        m_rightGaugeCluster->setVisible(true);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(true);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(false);
    }
    else if (m_screen == 5) { // LOGGING SHIT
        Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });
        const Bounds top = windowBounds.verticalSplit(0.75f, 1.0f);
        const Bounds bottom = windowBounds.verticalSplit(0.0f, 0.75f);
        //const Bounds bottomtop = bottom.verticalSplit(0.0f, 0.5f);
        //const Bounds bottombottom = bottom.verticalSplit(0.5f, 1.0f);
        m_engineView->setDrawFrame(false);
        //m_engineView->setBounds(grid.get(windowBounds, 0, 0, 2, 1));
        //m_engineView->setLocalPosition({ 0, 0 });
        m_engineView->activate();

        m_infoCluster->m_bounds = top;
        //m_rightGaugeCluster->m_bounds = top;
        m_loggingCluster->m_bounds = bottom;

        m_loggingCluster->setVisible(true);
        m_engineView->setVisible(false);
        m_rightGaugeCluster->setVisible(false);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(true);
    }

    const float cameraAspectRatio =
        m_engineView->m_bounds.width() / m_engineView->m_bounds.height();
    m_engine.GetDevice()->ResizeRenderTarget(
        m_mainRenderTarget,
        m_engineView->m_bounds.width(),
        m_engineView->m_bounds.height(),
        m_engineView->m_bounds.width(),
        m_engineView->m_bounds.height()
    );
    m_engine.GetDevice()->RepositionRenderTarget(
        m_mainRenderTarget,
        m_engineView->m_bounds.getPosition(Bounds::tl).x,
        screenHeight - m_engineView->m_bounds.getPosition(Bounds::tl).y
    );
    m_shaders.CalculateCamera(
        cameraAspectRatio * m_displayHeight / m_engineView->m_zoom,
        m_displayHeight / m_engineView->m_zoom,
        m_engineView->m_bounds,
        m_screenWidth,
        m_screenHeight,
        m_displayAngle);

    m_geometryGenerator.reset();

    render();

    m_engine.GetDevice()->EditBufferDataRange(
        m_geometryVertexBuffer,
        (char *)m_geometryGenerator.getVertexData(),
        sizeof(dbasic::Vertex) * m_geometryGenerator.getCurrentVertexCount(),
        0);

    m_engine.GetDevice()->EditBufferDataRange(
        m_geometryIndexBuffer,
        (char *)m_geometryGenerator.getIndexData(),
        sizeof(unsigned short) * m_geometryGenerator.getCurrentIndexCount(),
        0);
}

void EngineSimApplication::refreshUserInterface() {
    m_uiManager.destroy();
    m_uiManager.initialize(this);

    m_loggingCluster = m_uiManager.getRoot()->addElement<LoggingCluster>();
    m_engineView = m_uiManager.getRoot()->addElement<EngineView>();
    m_rightGaugeCluster = m_uiManager.getRoot()->addElement<RightGaugeCluster>();
    m_oscCluster = m_uiManager.getRoot()->addElement<OscilloscopeCluster>();
    m_performanceCluster = m_uiManager.getRoot()->addElement<PerformanceCluster>();
    m_loadSimulationCluster = m_uiManager.getRoot()->addElement<LoadSimulationCluster>();
    m_mixerCluster = m_uiManager.getRoot()->addElement<MixerCluster>();
    m_infoCluster = m_uiManager.getRoot()->addElement<InfoCluster>();

    //m_loggingCluster->setApp(this);
    m_infoCluster->setEngine(m_iceEngine);
    m_rightGaugeCluster->m_simulator = m_simulator;
    m_rightGaugeCluster->setEngine(m_iceEngine);
    m_oscCluster->setSimulator(m_simulator);
    if (m_iceEngine != nullptr) {
        m_oscCluster->setDynoMaxRange(units::toRpm(m_iceEngine->getRedline()));
    }
    m_performanceCluster->setSimulator(m_simulator);
    m_loadSimulationCluster->setSimulator(m_simulator);
    m_mixerCluster->setSimulator(m_simulator);
}

void EngineSimApplication::startRecording() {
    m_recording = true;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    atg_dtv::Encoder::VideoSettings settings{};

    // Output filename
    settings.fname = "../workspace/video_capture/engine_sim_video_capture.mp4";
    settings.inputWidth = m_engine.GetScreenWidth();
    settings.inputHeight = m_engine.GetScreenHeight();
    settings.width = settings.inputWidth;
    settings.height = settings.inputHeight;
    settings.hardwareEncoding = true;
    settings.inputAlpha = true;
    settings.bitRate = 40000000;

    m_encoder.run(settings, 2);
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

void EngineSimApplication::updateScreenSizeStability() {
    m_screenResolution[m_screenResolutionIndex][0] = m_engine.GetScreenWidth();
    m_screenResolution[m_screenResolutionIndex][1] = m_engine.GetScreenHeight();

    m_screenResolutionIndex = (m_screenResolutionIndex + 1) % ScreenResolutionHistoryLength;
}

bool EngineSimApplication::readyToRecord() {
    const int w = m_screenResolution[0][0];
    const int h = m_screenResolution[0][1];

    if (w <= 0 && h <= 0) return false;
    if ((w % 2) != 0 || (h % 2) != 0) return false;

    for (int i = 1; i < ScreenResolutionHistoryLength; ++i) {
        if (m_screenResolution[i][0] != w) return false;
        if (m_screenResolution[i][1] != h) return false;
    }

    return true;
}

void EngineSimApplication::stopRecording() {
    m_recording = false;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    m_encoder.commit();
    m_encoder.stop();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

void EngineSimApplication::recordFrame() {
#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    atg_dtv::Frame *frame = m_encoder.newFrame(false);
    if (frame != nullptr && m_encoder.getError() == atg_dtv::Encoder::Error::None) {
        m_engine.GetDevice()->ReadRenderTarget(m_engine.GetScreenRenderTarget(), frame->m_rgb);
    }

    m_encoder.submitFrame();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

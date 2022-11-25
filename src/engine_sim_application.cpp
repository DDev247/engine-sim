#include "../include/engine_sim_application.h"

#include "../include/piston_object.h"
#include "../include/connecting_rod_object.h"
#include "../include/constants.h"
#include "../include/units.h"
#include "../include/crankshaft_object.h"
#include "../include/ground_object.h"
#include "../include/tire_object.h"
#include "../include/vehicle_object.h"
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

std::string EngineSimApplication::s_buildVersion = "0.1.10a";

EngineSimApplication::EngineSimApplication()
{
    m_debug = false;
    m_world = nullptr;

    m_selected_car = 1;

    m_assetPath = "./";

    m_cameraRotation = ysVector2(0, 0);

    m_geometryVertexBuffer = nullptr;
    m_geometryIndexBuffer = nullptr;

    m_paused = false;
    m_recording = false;
    m_screenResolutionIndex = 0;
    for (int i = 0; i < ScreenResolutionHistoryLength; ++i)
    {
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

    m_engineView = nullptr;
    m_rightGaugeCluster = nullptr;
    m_customGaugeCluster = nullptr;
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
    m_cameraRotation = ysVector2(0.0f, 0.5f * ysMath::Constants::PI);
    m_dragStartMousePosition = ysVector2(0, 0);
}

EngineSimApplication::~EngineSimApplication()
{
    printf("\nQuit\n");

    if (gGameController != NULL)
    {
        SDL_GameControllerClose(gGameController);
        gGameController = NULL;
    }
    else if (gJoystick != NULL)
    {
        SDL_JoystickClose(gJoystick);
        gJoystick = NULL;
    }

    m_world->~b2World();
    m_world = nullptr;

    SDL_Quit();
}

std::string EngineSimApplication::intToString(int number)
{
    std::string string = std::to_string(number);

    if (number < 10)
        return '0' + string;
    else
        return string;
}

void EngineSimApplication::initialize(void *instance, ysContextObject::DeviceAPI api)
{
    dbasic::Path modulePath = dbasic::GetModulePath();
    dbasic::Path confPath = modulePath.Append("delta.conf");

    std::string enginePath = "../dependencies/submodules/delta-studio/engines/basic";
    m_assetPath = "../assets";
    if (confPath.Exists())
    {
        std::fstream confFile(confPath.ToString(), std::ios::in);

        std::getline(confFile, enginePath);
        std::getline(confFile, m_assetPath);
        enginePath = modulePath.Append(enginePath).ToString();
        m_assetPath = modulePath.Append(m_assetPath).ToString();

        confFile.close();
    }

    m_engine.GetConsole()->SetDefaultFontDirectory(enginePath + "/fonts/");

    const std::string shaderPath = enginePath + "/shaders/";
    std::string winTitle = "Engine Sim | AngeTheGreat | v" + s_buildVersion;
    dbasic::DeltaEngine::GameEngineSettings settings;
    settings.API = api;
    settings.DepthBuffer = true;
    settings.Instance = instance;
    settings.ShaderDirectory = shaderPath.c_str();
    settings.WindowTitle = winTitle.c_str();
    settings.WindowPositionX = 0;
    settings.WindowPositionY = 0;
    settings.WindowStyle = ysWindow::WindowStyle::Windowed;
    settings.WindowWidth = 1920;
    settings.WindowHeight = 1010;

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

    m_shaders.SetClearColor(ysColor::srgbiToLinear(0xFF, 0xFF, 0xFF));

    m_assetManager.SetEngine(&m_engine);

    m_engine.GetDevice()->CreateIndexBuffer(
        &m_geometryIndexBuffer, sizeof(unsigned short) * 200000, nullptr);
    m_engine.GetDevice()->CreateVertexBuffer(
        &m_geometryVertexBuffer, sizeof(dbasic::Vertex) * 100000, nullptr);

    m_geometryGenerator.initialize(100000, 200000);

    initialize();
}

void EngineSimApplication::loadMaterial(std::string filename, std::string name)
{
    dbasic::TextureAsset *textureAsset;
    dbasic::Material *material;

    printf(("\nLoading Material: " + name).c_str());

    const char *textureName = ("Texture" + name).c_str();
    const char *materialName = ("Material" + name).c_str();

    m_assetManager.LoadTexture(filename.c_str(), textureName);
    textureAsset = m_assetManager.GetTexture(textureName);
    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0x777777));
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName(materialName);
}

void EngineSimApplication::initialize()
{
    m_iniReader = inih::INIReader{"../settings.ini"};

    m_selected_track = m_iniReader.Get<int>("Settings", "SelectedTrack");
    printf("\nSelected Track: %i", m_selected_track);

    m_show_engine = m_iniReader.Get<bool>("Settings", "ShowEngine");
    printf("\nShowing engine: %i", m_show_engine);

    m_fovY = m_iniReader.Get<float>("Camera", "FovY");
    printf("\nFovY: %f", m_fovY);
    m_fovY *= (ysMath::Constants::PI / 180.0f);

    m_zoom = m_iniReader.Get<float>("Camera", "Zoom");
    printf("\nZoom: %f", m_zoom);

    // Define the gravity vector.
    b2Vec2 gravity(0.0f, 0.0f);

    // Construct a world object, which will hold and simulate the rigid bodies.
    m_world = new b2World(gravity);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    if (m_selected_car == 1)
    {
        // new
        loadMaterial("../assets/textures/bluebird/wheel_sj30_dff.png", "Wheel");
        loadMaterial("../assets/textures/bluebird/wp_tire_01_diff.png", "Tire");
        loadMaterial("../assets/textures/bluebird/ucb_interior_2800x2800.png", "Interior");
        loadMaterial("../assets/textures/bluebird/headlights_ucb.png", "Headlights");
        loadMaterial("../assets/textures/bluebird/UCB_Bottom_2048x2048.png", "Bottom");
        loadMaterial("../assets/textures/bluebird/UCB_Bottom_2048x2048.png", "Bottom2");
        loadMaterial("../assets/textures/bluebird/ucb_coil.png", "Coil");
    }

    m_assetManager.LoadTexture("../assets/textures/wheel_sj30_nrm.png", "NormalMap");
    dbasic::TextureAsset *textureAsset_N = m_assetManager.GetTexture("NormalMap");

    dbasic::Material *material_01;
    material_01 = m_assetManager.NewMaterial();
    material_01->SetDiffuseColor(ysColor::srgbiToLinear(0xFFFFFF));
    material_01->SetMetallic(2.0f);
    material_01->SetNormalMap(textureAsset_N->GetTexture());
    material_01->SetName("MaterialWhite");

    m_assetManager.LoadTexture("../assets/textures/mr2.png", "Texture_02");
    dbasic::TextureAsset *textureAsset_02 = m_assetManager.GetTexture("Texture_02");
    dbasic::Material *material_02;
    material_02 = m_assetManager.NewMaterial();
    material_02->SetDiffuseMap(textureAsset_02->GetTexture());
    material_02->SetLit(true);
    material_02->SetName("MaterialLights");

    // m_assetManager.LoadTexture("../assets/textures/Purple/texture_08.png", "Texture_03");
    // dbasic::TextureAsset* textureAsset_03 =  m_assetManager.GetTexture("Texture_03");
    dbasic::Material *material_03;
    material_03 = m_assetManager.NewMaterial();
    material_03->SetDiffuseColor(ysColor::srgbiToLinear(0xFF0000));
    // material_03->SetDiffuseMap(textureAsset_03->GetTexture());
    material_03->SetLit(true);
    material_03->SetName("Material_03");

    dbasic::TextureAsset *textureAsset;
    dbasic::Material *material;

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0x00FF00));
    material->SetName("MaterialGreen");

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0x000000));
    material->SetName("MaterialBlack");

    m_assetManager.LoadTexture("../assets/textures/tunnel_road.jpg", "Texture_04");
    dbasic::TextureAsset *textureAsset_04 = m_assetManager.GetTexture("Texture_04");
    dbasic::Material *material_04;
    material_04 = m_assetManager.NewMaterial();
    material_04->SetDiffuseMap(textureAsset_04->GetTexture());
    material_04->SetDiffuseColor(ysColor::srgbiToLinear(0xFFFFFF));
    material_04->SetLit(true);
    material_04->SetName("Material_04");

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0x96A7AA));
    material->SetDiffuseMix(0.27f);
    material->SetName("MaterialGlass");

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0x777777));
    material->SetName("MaterialFrame");

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0x00FFFF));
    material->SetName("MaterialMark");

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0xDDDDDD));
    material->SetLit(true);
    // material->SetDiffuseMix(0.0f);
    material->SetName("MaterialGray");

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0xDDDDDD));
    material->SetLit(true);
    material->SetDiffuseMix(1.0f);
    material->SetName("MaterialEngine");

    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(0.8f * ysColor::srgbiToLinear(0x22CCFF));
    // material->SetDiffuseMix(0.0f);
    material->SetName("MaterialBody");

    m_assetManager.LoadTexture("../assets/textures/mr2_tyre.png", "TextureTires");
    textureAsset = m_assetManager.GetTexture("TextureTires");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetDiffuseColor(ysColor::srgbiToLinear(0xFFFFFF));
    material->SetName("MaterialTires");

    m_assetManager.LoadTexture("../assets/textures/track.png", "TextureTrack");
    textureAsset = m_assetManager.GetTexture("TextureTrack");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialTrack");

    m_assetManager.LoadTexture("../assets/textures/asphalt.png", "TextureAsphalt");
    textureAsset = m_assetManager.GetTexture("TextureAsphalt");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialAsphalt");

    m_assetManager.LoadTexture("../assets/textures/Track05.png", "TextureTrack05");
    textureAsset = m_assetManager.GetTexture("TextureTrack05");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialTrack05");

    m_assetManager.LoadTexture("../assets/textures/tunnel_road.jpg", "TextureTunnel");
    textureAsset = m_assetManager.GetTexture("TextureTunnel");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialTunnel");

    // m_assetManager.LoadTexture("../assets/textures/color_palette.png", "TextureLittleCity");
    m_assetManager.LoadTexture("../assets/textures/tunnel_road.jpg", "TextureLittleCity");
    textureAsset = m_assetManager.GetTexture("TextureLittleCity");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseColor(ysColor::srgbiToLinear(0xFFFFFF));
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetLit(true);
    material->SetName("MaterialLittleCity");

    /// GARDA
    m_assetManager.LoadTexture("../assets/textures/garda/fence.png", "TextureFence");
    textureAsset = m_assetManager.GetTexture("TextureFence");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialFence");

    m_assetManager.LoadTexture("../assets/textures/garda/standard_material.png", "TextureStandard");
    textureAsset = m_assetManager.GetTexture("TextureStandard");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialStandard");

    m_assetManager.LoadTexture("../assets/textures/garda/material.jpg", "TextureMat");
    textureAsset = m_assetManager.GetTexture("TextureMat");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialMat");

    m_assetManager.LoadTexture("../assets/textures/garda/italian.jpg", "TextureItalian");
    textureAsset = m_assetManager.GetTexture("TextureItalian");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialItalian");

    m_assetManager.LoadTexture("../assets/textures/garda/parking.jpg", "TextureParking");
    textureAsset = m_assetManager.GetTexture("TextureParking");
    material = m_assetManager.NewMaterial();
    material->SetDiffuseMap(textureAsset->GetTexture());
    material->SetName("MaterialParking");

    std::vector<std::string> textureNames = getIniReader().GetVector<std::string>("Vehicle", "Textures");
    std::string textureFolder = "trueno";

    for (std::vector<std::string>::size_type i = 0; i != textureNames.size(); i++)
    {
        std::string textureName = textureNames[i];
        m_assetManager.LoadTexture(("../assets/textures/" + textureFolder + "/" + textureName).c_str(), ("Texture_" + textureName).c_str());
        textureAsset = m_assetManager.GetTexture(("Texture_" + textureName).c_str());
        material = m_assetManager.NewMaterial();
        material->SetDiffuseMap(textureAsset->GetTexture());
        std::string materialName = "Material__" + intToString(i);
        material->SetName(materialName.c_str());
        printf("\n%s", materialName.c_str());
    }

    //////////////////

    if (true)
    {
        loadMaterial("../assets/textures/city/Roads_Grounds.png", "CityRoads");
        loadMaterial("../assets/textures/city/Filler_Buildings_1.png", "Filler");
        loadMaterial("../assets/textures/city/LM_Basketball.png", "Basketball");
        loadMaterial("../assets/textures/city/City_Props.png", "Props");
        loadMaterial("../assets/textures/city/LM_Clinic.png", "Clinic");
        loadMaterial("../assets/textures/city/LM_FishFactory.png", "Fish");
        loadMaterial("../assets/textures/city/LM_Laundrymat.png", "Laundry");
        loadMaterial("../assets/textures/city/LM_Pawnshop.png", "Pawn");
        loadMaterial("../assets/textures/city/LM_Projects.png", "Projects");
        loadMaterial("../assets/textures/city/Paramount.png", "Paramount");
    }

    dbasic::Material *material_UI;
    material_UI = m_assetManager.NewMaterial();
    material_UI->SetLit(false);
    material_UI->SetName("Material_UI");

    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xFF));

    std::chrono::steady_clock::time_point material_end = std::chrono::steady_clock::now();
    printf("Model loading took: ");
    printf((std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(material_end - begin).count())).c_str());
    printf(" ms.\n");

    std::chrono::steady_clock::time_point model_begin = std::chrono::steady_clock::now();

    float trackScale = getIniReader().Get<float>("Track", "Scale");
    std::string vehicleModelFileName = getIniReader().Get<std::string>("Vehicle", "ModelFileName");
    printf("\n%s", vehicleModelFileName.c_str());

    m_assetManager.CompileInterchangeFile((m_assetPath + "/assets").c_str(), 1.0f, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/ConnectingRodLong").c_str(), 1.0f, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/Piston_2JZ_GE").c_str(), 1.0f, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/Rod_2JZ_GE").c_str(), 1.0f, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/Crankshaft_2JZ_GE_Part_1").c_str(), 1.0f, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/Crankshaft_2JZ_GE_Part_2").c_str(), 1.0f, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/Head_2JZ_GE_Split").c_str(), 1.0f, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/Camshaft_2JZ_GE").c_str(), 1.0f, true);

    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/trackGarda").c_str(), trackScale, true);
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/cube").c_str(), 1.0f, true);

    float vehicleScale = getIniReader().Get<float>("Vehicle", "Scale");
    m_assetManager.CompileInterchangeFile((m_assetPath + "/models/" + vehicleModelFileName).c_str(), vehicleScale, true);

    m_assetManager.LoadSceneFile((m_assetPath + "/assets").c_str(), true);

    m_assetManager.LoadSceneFile((m_assetPath + "/models/ConnectingRodLong").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/Piston_2JZ_GE").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/Rod_2JZ_GE").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/Crankshaft_2JZ_GE_Part_1").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/Crankshaft_2JZ_GE_Part_2").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/Head_2JZ_GE_Split").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/Camshaft_2JZ_GE").c_str(), true);

    m_assetManager.LoadSceneFile((m_assetPath + "/models/trackGarda").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/cube").c_str(), true);
    m_assetManager.LoadSceneFile((m_assetPath + "/models/" + vehicleModelFileName).c_str(), true);

    if (m_selected_track == 4)
    {
        m_assetManager.LoadTexture("../assets/tracks/city2/textures/low_modular_kit_BaseColor.png", "TextureCity2");
        textureAsset = m_assetManager.GetTexture("TextureCity2");
        material = m_assetManager.NewMaterial();
        material->SetDiffuseMap(textureAsset->GetTexture());
        material->SetName("MaterialCity2");

        std::string path = m_assetPath + "/tracks/city2/city_02";
        m_assetManager.CompileInterchangeFile((path).c_str(), 1.0f, true);
        m_assetManager.LoadSceneFile((path).c_str(), true);
    }

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
    m_audioSource->SetMode((m_simulator.getEngine() != nullptr)
                               ? ysAudioSource::Mode::Loop
                               : ysAudioSource::Mode::Stop);
    m_audioSource->SetPan(0.0f);
    m_audioSource->SetVolume(1.0f);

    std::chrono::steady_clock::time_point model_end = std::chrono::steady_clock::now();
    printf("Model/ES loading took: ");
    printf((std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(model_end - model_begin).count())).c_str());
    printf(" ms.\n");

    std::chrono::steady_clock::time_point joy_begin = std::chrono::steady_clock::now();

    int selectedController = 0;
    std::string joyOrGC;

    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
    {
        m_infoCluster->setLogMessage("SDL could not initialize joystick!");
    }
    else
    {
        std::string mappingFilePath = "../gamecontrollerdb.txt";
        int i = SDL_GameControllerAddMappingsFromFile(mappingFilePath.c_str());
        if (i == -1)
        {
            printf("\n[%s] NOT LOADED!!! %i\n\n", mappingFilePath.c_str(), i);
        }
        else
        {
            printf("\nLoaded %i mappings from file: [ %s ]", i, mappingFilePath.c_str());
        }

        // Check for joysticks
        if (SDL_NumJoysticks() < 1)
        {
            m_infoCluster->setLogMessage("No joysticks connected");
        }
        else if (selectedController >= SDL_NumJoysticks())
        {
            m_infoCluster->setLogMessage("Selected controller not exist!");
        }
        else if (SDL_IsGameController(selectedController))
        {
            joyOrGC = "Game controller";
            std::string selectedControllerName = SDL_JoystickNameForIndex(selectedController);
            m_infoCluster->setLogMessage(joyOrGC + ": " + selectedControllerName);
            printf("\n%s: %s\n", joyOrGC.c_str(), selectedControllerName.c_str());

            gGameController = SDL_GameControllerOpen(selectedController);
            if (!gGameController)
                m_infoCluster->setLogMessage("GameController null");
        }
        else
        {
            // Load joystick
            joyOrGC = "Joystick";
            std::string selectedControllerName = SDL_JoystickNameForIndex(selectedController);
            m_infoCluster->setLogMessage(joyOrGC + ": " + selectedControllerName);
            printf("\n%s: %s\n\n", joyOrGC.c_str(), selectedControllerName.c_str());
            
            gJoystick = SDL_JoystickOpen(selectedController);

            if (gJoystick == NULL)
            {
                m_infoCluster->setLogMessage("Warning: Unable to open joystick!");
            }
        }
    }

    std::chrono::steady_clock::time_point joy_end = std::chrono::steady_clock::now();
    printf("Joystick Loading took: ");
    printf((std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(joy_end - joy_begin).count())).c_str());
    printf(" ms.\n");

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    printf("Loading took: ");
    printf((std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count())).c_str());
    printf(" ms.\n");

#ifdef ATG_ENGINE_DISCORD_ENABLED
    // Create a global instance of discord-rpc
    CDiscord::CreateInstance();

    // Enable it, this needs to be set via a config file of some sort.
    GetDiscordManager()->SetUseDiscord(true);
    DiscordRichPresence passMe = {0};
    GetDiscordManager()->SetStatus(passMe, m_iceEngine->getName(), s_buildVersion);
#endif /* ATG_ENGINE_DISCORD_ENABLED */

    // m_assetManager.LoadTexture("./tire.png", "TestTexture4");
    // dbasic::TextureAsset *textureAsset = m_assetManager.GetTexture("TestTexture2");
}

void EngineSimApplication::process(float frame_dt)
{

    // Prepare for simulation. Typically we use a time step of 1/60 of a
    // second (60Hz) and 10 iterations. This provides a high quality simulation
    // in most game scenarios.
    float timeStep = 1.0f / 60.0f;
    int32 velocityIterations = 1;
    int32 positionIterations = 1;

    // Instruct the world to perform a single step of simulation.
    // It is generally best to keep the time step and iterations fixed.

    frame_dt = clamp(frame_dt, 1 / 200.0f, 1 / 30.0f);

    double speed = 1.0 / 1.0;
    if (m_engine.IsKeyDown(ysKey::Code::N1))
    {
        speed = 1 / 10.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N2))
    {
        speed = 1 / 100.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N3))
    {
        speed = 1 / 200.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N4))
    {
        speed = 1 / 500.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::G))
    {
        speed = 1 / 1000.0;
    }

    m_simulator.setSimulationSpeed(speed);

    m_world->Step(speed * frame_dt, velocityIterations, positionIterations);

    m_vehicle_object->process(speed * frame_dt);

    const double avgFramerate = clamp(m_engine.GetAverageFramerate(), 30.0f, 1000.0f);
    m_simulator.startFrame(1 / avgFramerate);

    auto proc_t0 = std::chrono::steady_clock::now();
    const int iterationCount = m_simulator.getFrameIterationCount();
    while (m_simulator.simulateStep())
    {
        m_oscCluster->sample();
    }

    auto proc_t1 = std::chrono::steady_clock::now();

    m_simulator.endFrame();

    auto duration = proc_t1 - proc_t0;
    if (iterationCount > 0)
    {
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

    if (currentLead > newLead)
    {
        maxWrite = 0;
    }

    int16_t *samples = new int16_t[maxWrite];
    const int readSamples = m_simulator.readAudioOutput(maxWrite, samples);

    for (SampleOffset i = 0; i < (SampleOffset)readSamples && i < maxWrite; ++i)
    {
        const int16_t sample = samples[i];
        if (m_oscillatorSampleOffset % 4 == 0)
        {
            m_oscCluster->getAudioWaveformOscilloscope()->addDataPoint(
                m_oscillatorSampleOffset,
                sample / (float)(INT16_MAX));
        }

        m_audioBuffer.writeSample(sample, m_audioBuffer.m_writePointer, (int)i);

        m_oscillatorSampleOffset = (m_oscillatorSampleOffset + 1) % (44100 / 10);
    }

    delete[] samples;

    if (readSamples > 0)
    {
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

    m_performanceCluster->addAudioLatencySample(
        m_audioBuffer.offsetDelta(m_audioSource->GetCurrentWritePosition(), m_audioBuffer.m_writePointer) / (44100 * 0.1));
    m_performanceCluster->addInputBufferUsageSample(
        (double)m_simulator.getSynthesizerInputLatency() / m_simulator.getSynthesizerInputLatencyTarget());

    if (m_simulator.getEngine() != nullptr)
    {
        if (m_simulator.getSynthesizerInputLatency() > m_simulator.getSynthesizerInputLatencyTarget() * 2)
        {
            m_audioSource->SetMode(ysAudioSource::Mode::Stop);
        }
        else
        {
            m_audioSource->SetMode(ysAudioSource::Mode::Loop);
        }
    }
}

void EngineSimApplication::render()
{

    for (SimulationObject *object : m_objects)
    {
        object->generateGeometry();
    }

    for (SimulationObject *object : m_objects)
    {
        object->render(&getViewParameters());
    }

    m_shaders.UseMaterial(m_assetManager.FindMaterial("Material_UI"));
    m_uiManager.render();
}

float EngineSimApplication::pixelsToUnits(float pixels) const
{
    const float f = m_displayHeight / m_engineView->m_bounds.height();
    return pixels * f;
}

float EngineSimApplication::unitsToPixels(float units) const
{
    const float f = m_engineView->m_bounds.height() / m_displayHeight;
    return units * f;
}

void EngineSimApplication::run()
{

    while (true)
    {
        m_engine.StartFrame();

        if (!m_engine.IsOpen())
            break;
        if (m_engine.ProcessKeyDown(ysKey::Code::Escape))
        {
            break;
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::Return))
        {
            m_audioSource->SetMode(ysAudioSource::Mode::Stop);
            loadScript();
            if (m_simulator.getEngine() != nullptr)
            {
                m_audioSource->SetMode(ysAudioSource::Mode::Loop);
            }
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::Tab))
        {
            m_screen++;
            if (m_screen > 3)
                m_screen = 0;
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::F))
        {
            if (m_engine.GetGameWindow()->GetWindowStyle() != ysWindow::WindowStyle::Fullscreen)
            {
                m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Fullscreen);
                m_engine.GetGameWindow()->SetState(ysWindow::WindowState::Fullscreen);

                m_infoCluster->setLogMessage("Entered fullscreen mode");
            }
            else if (m_engine.GetGameWindow()->GetWindowStyle() == ysWindow::WindowStyle::Fullscreen)
            {
                m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Windowed);
                m_engine.GetGameWindow()->SetState(ysWindow::WindowState::Unknown);
                m_infoCluster->setLogMessage("Exited fullscreen mode");
            }
        }

        m_gameWindowHeight = m_engine.GetGameWindow()->GetGameHeight();
        m_screenHeight = m_engine.GetGameWindow()->GetScreenHeight();
        m_screenWidth = m_engine.GetGameWindow()->GetScreenWidth();

        updateScreenSizeStability();

        processEngineInput();

        if (m_engine.ProcessKeyDown(ysKey::Code::Insert) &&
            m_engine.GetGameWindow()->IsActive())
        {
            if (!isRecording() && readyToRecord())
            {
                startRecording();
            }
            else if (isRecording())
            {
                stopRecording();
            }
        }

        if (isRecording() && !readyToRecord())
        {
            stopRecording();
        }

        if (!m_paused || m_engine.ProcessKeyDown(ysKey::Code::Right))
        {
            process(m_engine.GetFrameLength());
        }

        m_uiManager.update(m_engine.GetFrameLength());

        renderScene();

        m_engine.EndFrame();

        if (isRecording())
        {
            recordFrame();
        }
    }

    if (isRecording())
    {
        stopRecording();
    }

    m_simulator.endAudioRenderingThread();
}

void EngineSimApplication::destroy()
{
    m_shaderSet.Destroy();

    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryVertexBuffer);
    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryIndexBuffer);

    m_assetManager.Destroy();
    m_engine.Destroy();

    m_simulator.destroy();
    m_audioBuffer.destroy();
}

void EngineSimApplication::loadEngine(
    Engine *engine,
    Vehicle *vehicle,
    Transmission *transmission)
{
    if (m_vehicle != nullptr)
    {
        delete m_vehicle;
        m_vehicle = nullptr;
    }
    if (m_transmission != nullptr)
    {
        delete m_transmission;
        m_transmission = nullptr;
    }
    if (m_iceEngine != nullptr)
    {
        m_iceEngine->destroy();
        delete m_iceEngine;
    }

    m_iceEngine = engine;
    m_vehicle = vehicle;
    m_transmission = transmission;

    destroyObjects();
    m_simulator.releaseSimulation();

    if (engine == nullptr || vehicle == nullptr || transmission == nullptr)
    {
        m_iceEngine = nullptr;
        m_viewParameters.Layer1 = 0;

        return;
    }
    createObjects(engine);
    m_viewParameters.Layer1 = engine->getMaxDepth();
    engine->calculateDisplacement();

    m_simulator.setFluidSimulationSteps(8);
    m_simulator.setSimulationFrequency(engine->getSimulationFrequency());

    Simulator::Parameters simulatorParams;
    simulatorParams.systemType = Simulator::SystemType::NsvOptimized;
    m_simulator.initialize(simulatorParams);
    m_simulator.loadSimulation(engine, vehicle, transmission);

    Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
    audioParams.InputSampleNoise = static_cast<float>(engine->getInitialJitter());
    audioParams.AirNoise = static_cast<float>(engine->getInitialNoise());
    audioParams.dF_F_mix = static_cast<float>(engine->getInitialHighFrequencyGain());
    m_simulator.getSynthesizer()->setAudioParameters(audioParams);

    for (int i = 0; i < engine->getExhaustSystemCount(); ++i)
    {
        ImpulseResponse *response = engine->getExhaustSystem(i)->getImpulseResponse();

        ysAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        m_simulator.getSynthesizer()->initializeImpulseResponse(
            reinterpret_cast<const int16_t *>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            response->getVolume(),
            i);

        waveFile.DestroyInternalBuffer();
    }

    m_simulator.startAudioRenderingThread();
}

void EngineSimApplication::drawGenerated(
    const GeometryGenerator::GeometryIndices &indices,
    int layer)
{
    drawGenerated(indices, layer, m_shaders.GetRegularFlags());
    // drawGenerated(indices, layer, m_shaders.GetUiFlags());
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

void EngineSimApplication::configure(const ApplicationSettings &settings)
{
    m_applicationSettings = settings;

    if (settings.startFullscreen)
    {
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

void EngineSimApplication::createObjects(Engine *engine)
{
    GroundObject *groundObject = new GroundObject(this);
    groundObject->m_ground = engine->getGround();
    groundObject->m_vehicle = m_vehicle;
    m_objects.push_back(groundObject);

    VehicleObject *vehicleObject = new VehicleObject(this, m_world, m_vehicle);

    m_vehicle->m_transform = &vehicleObject->m_transform;
    m_vehicle->m_transform_engine = &vehicleObject->m_transform_engine;
    m_vehicle->m_transform_camera = &vehicleObject->m_transform_camera;

    m_objects.push_back(vehicleObject);
    m_vehicle_object = vehicleObject;

    int cylPerBank = engine->getCylinderCount() / engine->getCylinderBankCount();

    int cylinderIndex = 0;

    for (int i = 0; i < engine->getCylinderBankCount(); i++)
    {
        for (int j = 0; j < cylPerBank; ++j)
        {
            ConnectingRodObject *rodObject = new ConnectingRodObject;
            rodObject->initialize(this);
            rodObject->m_connectingRod = engine->getConnectingRod(cylinderIndex);
            rodObject->z = j;
            m_objects.push_back(rodObject);

            PistonObject *pistonObject = new PistonObject;
            pistonObject->initialize(this);
            pistonObject->m_piston = engine->getPiston(cylinderIndex);
            pistonObject->z = j;
            m_objects.push_back(pistonObject);

            CombustionChamberObject *ccObject = new CombustionChamberObject;
            ccObject->initialize(this);
            ccObject->m_chamber = m_iceEngine->getChamber(cylinderIndex);
            ccObject->z = j;
            m_objects.push_back(ccObject);

            cylinderIndex++;
        }
    }

    for (int i = 0; i < engine->getCrankshaftCount(); ++i)
    {
        CrankshaftObject *crankshaftObject = new CrankshaftObject;
        crankshaftObject->initialize(this);
        crankshaftObject->m_crankshaft = engine->getCrankshaft(i);
        crankshaftObject->m_vehicle = m_vehicle;
        crankshaftObject->z = i;
        m_objects.push_back(crankshaftObject);
    }

    cylinderIndex = 0;

    for (int i = 0; i < engine->getCylinderBankCount(); ++i)
    {
        CylinderBankObject *cbObject = new CylinderBankObject;
        cbObject->initialize(this);
        cbObject->m_bank = engine->getCylinderBank(i);
        cbObject->m_first_head = engine->getHead(i);
        m_objects.push_back(cbObject);

        for (int j = 0; j < cylPerBank; ++j)
        {
            CylinderHead *ch = new CylinderHead;
            ch->m_cylinderIndex = j;

            CylinderHeadObject *chObject = new CylinderHeadObject;
            chObject->initialize(this);
            chObject->m_head = engine->getHead(i);
            chObject->m_engine = engine;
            chObject->m_cylinderIndex = j;
            chObject->z = j;
            m_objects.push_back(chObject);
        }
    }

    engine->scaleZ = engine->getHead(0)->getCylinderBank()->getBore() * 1.042f;

    printf("\nObjects created\n");
}

void EngineSimApplication::destroyObjects()
{
    for (SimulationObject *object : m_objects)
    {
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

void EngineSimApplication::loadScript()
{
    Engine *engine = nullptr;
    Vehicle *vehicle = nullptr;
    Transmission *transmission = nullptr;

#ifdef ATG_ENGINE_SIM_PIRANHA_ENABLED
    es_script::Compiler compiler;
    compiler.initialize();
    const bool compiled = compiler.compile("../assets/main.mr");
    if (compiled)
    {
        const es_script::Compiler::Output output = compiler.execute();
        configure(output.applicationSettings);

        engine = output.engine;
        vehicle = output.vehicle;
        transmission = output.transmission;
    }
    else
    {
        engine = nullptr;
        vehicle = nullptr;
        transmission = nullptr;
    }

    compiler.destroy();
#endif /* ATG_ENGINE_SIM_PIRANHA_ENABLED */

    if (vehicle == nullptr)
    {
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

    if (transmission == nullptr)
    {
        const double gearRatios[] = {2.97, 2.07, 1.43, 1.00, 0.84, 0.56};
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

void EngineSimApplication::processEngineInput()
{
    if (m_iceEngine == nullptr)
    {
        return;
    }

    const float dt = m_engine.GetFrameLength();
    bool fineControlMode = m_engine.IsKeyDown(ysKey::Code::Space);

    int mx, my, mz;
    m_engine.GetMousePos(&mx, &my);

    if (m_engine.ProcessMouseButtonDown(ysMouse::Button::Left))
    {
        m_dragStartMousePosition = ysVector2(mx, my);
    }

    const int mouseWheel = m_engine.GetMouseWheel();
    const int mouseWheelDelta = mouseWheel - m_lastMouseWheel;
    m_lastMouseWheel = mouseWheel;

    m_zoom = clamp(m_zoom - 0.01f * mouseWheelDelta, 0.9f, 90.0f);

    if (m_engine.ProcessJoystickButtonDown(ysJoystick::Button::Button_Back))
    {
        fineControlMode = !fineControlMode;
    }

    bool fineControlInUse = false;
    if (m_engine.IsKeyDown(ysKey::Code::Z))
    {
        const double rate = fineControlMode
                                ? 0.001
                                : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
        audioParams.Volume = clamp(audioParams.Volume + mouseWheelDelta * rate * dt);

        m_simulator.getSynthesizer()->setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[Z] - Set volume to " + std::to_string(audioParams.Volume));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::X))
    {
        const double rate = fineControlMode
                                ? 0.001
                                : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
        audioParams.Convolution = clamp(audioParams.Convolution + mouseWheelDelta * rate * dt);

        m_simulator.getSynthesizer()->setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[X] - Set convolution level to " + std::to_string(audioParams.Convolution));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::C))
    {
        const double rate = fineControlMode
                                ? 0.00001
                                : 0.001;

        Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
        audioParams.dF_F_mix = clamp(audioParams.dF_F_mix + mouseWheelDelta * rate * dt);

        m_simulator.getSynthesizer()->setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[C] - Set high freq. gain to " + std::to_string(audioParams.dF_F_mix));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::V))
    {
        const double rate = fineControlMode
                                ? 0.001
                                : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
        audioParams.AirNoise = clamp(audioParams.AirNoise + mouseWheelDelta * rate * dt);

        m_simulator.getSynthesizer()->setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[V] - Set low freq. noise to " + std::to_string(audioParams.AirNoise));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::B))
    {
        const double rate = fineControlMode
                                ? 0.001
                                : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
        audioParams.InputSampleNoise = clamp(audioParams.InputSampleNoise + mouseWheelDelta * rate * dt);

        m_simulator.getSynthesizer()->setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[B] - Set high freq. noise to " + std::to_string(audioParams.InputSampleNoise));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N))
    {
        const double rate = fineControlMode
                                ? 10.0
                                : 100.0;

        const double newSimulationFrequency = clamp(
            m_simulator.getSimulationFrequency() + mouseWheelDelta * rate * dt,
            400.0, 400000.0);

        m_simulator.setSimulationFrequency(newSimulationFrequency);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[N] - Set simulation freq to " + std::to_string(m_simulator.getSimulationFrequency()));
    }
    else if (m_engine.IsKeyDown(ysKey::Code::G) && m_simulator.m_dyno.m_hold)
    {
        if (mouseWheelDelta > 0)
        {
            m_dynoSpeed += units::rpm(100.0);
        }
        else if (mouseWheelDelta < 0)
        {
            m_dynoSpeed -= units::rpm(100.0);
        }

        m_dynoSpeed = clamp(m_dynoSpeed, units::rpm(0), DBL_MAX);

        m_infoCluster->setLogMessage("[G] - Set dyno speed to " + std::to_string(m_dynoSpeed));
        fineControlInUse = true;
    }

    const double prevTargetThrottle = m_targetSpeedSetting;
    m_targetSpeedSetting = fineControlMode ? m_targetSpeedSetting : 0.0;

    if (m_engine.IsKeyDown(ysKey::Code::Q))
    {
        m_targetSpeedSetting = 0.01;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::W))
    {
        m_targetSpeedSetting = 0.1;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::E))
    {
        m_targetSpeedSetting = 0.2;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::R))
    {
        m_targetSpeedSetting = 1.0;
    }
    else if (fineControlMode && !fineControlInUse)
    {
        m_targetSpeedSetting = clamp(m_targetSpeedSetting + mouseWheelDelta * 0.0001);
    }
    if (prevTargetThrottle != m_targetSpeedSetting)
    {
        m_infoCluster->setLogMessage("Speed control set to " + std::to_string(m_targetSpeedSetting));
    }

    m_speedSetting = m_targetSpeedSetting * 0.5 + 0.5 * m_speedSetting;

    m_iceEngine->setSpeedControl(m_speedSetting);

    if (m_engine.ProcessKeyDown(ysKey::Code::M))
    {
        m_show_engine = !m_show_engine;
        m_infoCluster->setLogMessage("Show engine: " + std::to_string(m_show_engine));
    }
    if (m_engine.ProcessKeyDown(ysKey::Code::N))
    {
        m_debug = !m_debug;
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::OEM_Comma))
    {
        if (getViewParameters().Layer0 - 1 >= 0)
            setViewLayer(getViewParameters().Layer0 - 1);

        m_infoCluster->setLogMessage("[,] - Set render layer to " + std::to_string(getViewParameters().Layer0));
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::D) or m_engine.IsJoystickButtonDown(ysJoystick::Button::Button_L1))
    {
        //"Brakes"
        m_vehicle->m_rotatingMass->v_theta *= 0.98f;
        m_vehicle->m_brakes = 0.6;
    }
    else
        m_vehicle->m_brakes = 0.0;

    if (m_engine.ProcessKeyDown(ysKey::Code::H) or m_engine.ProcessJoystickButtonDown(ysJoystick::Button::Button_Left))
    {
        m_simulator.m_dyno.m_hold = !m_simulator.m_dyno.m_hold;

        const std::string msg = m_simulator.m_dyno.m_hold
                                    ? m_simulator.m_dyno.m_enabled ? "HOLD ENABLED" : "HOLD ON STANDBY [ENABLE DYNO. FOR HOLD]"
                                    : "HOLD DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_simulator.m_dyno.m_enabled)
    {
        if (!m_simulator.m_dyno.m_hold)
        {
            if (m_simulator.getFilteredDynoTorque() > units::torque(1.0, units::ft_lb))
            {
                m_dynoSpeed += units::rpm(500) * dt;
            }
            else
            {
                m_dynoSpeed *= (1 / (1 + dt));
            }

            if ((m_dynoSpeed + units::rpm(1000)) > m_iceEngine->getRedline())
            {
                m_simulator.m_dyno.m_enabled = false;
                m_dynoSpeed = units::rpm(0);
            }
        }
    }
    else
    {
        if (!m_simulator.m_dyno.m_hold)
        {
            m_dynoSpeed = units::rpm(0);
        }
    }

    m_simulator.m_dyno.m_rotationSpeed = m_dynoSpeed + units::rpm(1000);

    const bool prevStarterEnabled = m_simulator.m_starterMotor.m_enabled;
    if (m_engine.IsKeyDown(ysKey::Code::S) or m_engine.IsJoystickButtonDown(ysJoystick::Button::Button_B))
    {
        m_simulator.m_starterMotor.m_enabled = true;
    }
    else
    {
        m_simulator.m_starterMotor.m_enabled = false;
    }

    if (prevStarterEnabled != m_simulator.m_starterMotor.m_enabled)
    {
        const std::string msg = m_simulator.m_starterMotor.m_enabled
                                    ? "STARTER ENABLED"
                                    : "STARTER DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::A) or m_engine.ProcessJoystickButtonDown(ysJoystick::Button::Button_Y))
    {
        m_simulator.getEngine()->getIgnitionModule()->m_enabled =
            !m_simulator.getEngine()->getIgnitionModule()->m_enabled;

        const std::string msg = m_simulator.getEngine()->getIgnitionModule()->m_enabled
                                    ? "IGNITION ENABLED"
                                    : "IGNITION DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_engine.ProcessKeyDown(ysKey::Code::Up) or m_engine.ProcessJoystickButtonDown(ysJoystick::Button::Button_A))
    {
        m_simulator.getTransmission()->changeGear(m_simulator.getTransmission()->getGear() + 1);

        m_infoCluster->setLogMessage(
            "UPSHIFTED TO " + std::to_string(m_simulator.getTransmission()->getGear() + 1));
    }
    else if (m_engine.ProcessKeyDown(ysKey::Code::Down) or m_engine.ProcessJoystickButtonDown(ysJoystick::Button::Button_X))
    {
        m_simulator.getTransmission()->changeGear(m_simulator.getTransmission()->getGear() - 1);

        if (m_simulator.getTransmission()->getGear() != -1)
        {
            m_infoCluster->setLogMessage(
                "DOWNSHIFTED TO " + std::to_string(m_simulator.getTransmission()->getGear() + 1));
        }
        else
        {
            m_infoCluster->setLogMessage("SHIFTED TO NEUTRAL");
        }
    }

    if (m_engine.IsKeyDown(ysKey::Code::T))
    {
        m_targetClutchPressure -= 0.2 * dt;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::U))
    {
        m_targetClutchPressure += 0.2 * dt;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::Shift) or m_engine.IsJoystickButtonDown(ysJoystick::Button::Button_Down))
    {
        m_targetClutchPressure = 0.0;
        m_infoCluster->setLogMessage("CLUTCH DEPRESSED");
    }
    else if (!m_engine.IsKeyDown(ysKey::Code::Y))
    {
        m_targetClutchPressure = 1.0;
    }

    m_targetClutchPressure = clamp(m_targetClutchPressure);

    double clutchRC = 0.001;
    if (m_engine.IsKeyDown(ysKey::Code::Space))
    {
        clutchRC = 1.0;
    }

    const double clutch_s = dt / (dt + clutchRC);
    m_clutchPressure = m_clutchPressure * (1 - clutch_s) + m_targetClutchPressure * clutch_s;

    int x, y, z;
    m_engine.GetJoystickAxes(&x, &y, &z);

    m_speedSetting = x / 32800.0f;
    m_iceEngine->setSpeedControl(m_speedSetting);

    m_clutchPressure = 1.0f - y / 32800.0f;

    if (!m_engine.IsJoystickButtonDown(ysJoystick::Button::Button_L1))
        m_simulator.getTransmission()->setClutchPressure(m_clutchPressure);

    float steeringInput = m_simulator.getVehicle()->mapToRange(z, -32000.0f, 32000.0f, -1.0f, 1.0f);
    m_simulator.getVehicle()->steeringAngle = clamp(steeringInput * abs(steeringInput) / clamp((0.5f * abs(0.3f * m_simulator.getVehicle()->getSpeed())), 1.0f, 9.0f), -0.7f, 0.7f);

    ysVector t = ysMath::LoadVector(0, 0, 0, 0);

    float distance = sqrt(
        pow((float)(t[0]), 2) +
        pow((float)(t[1]), 2) * 1.0);

    m_engineView->m_zoom = 1.0f;
    m_engineView->setLocalPosition(
        {-100.0f * (float)(t[0]),
         -1.0f * (float)(t[1])});
}

void EngineSimApplication::renderScene()
{

    getShaders()->ResetBaseColor();
    getShaders()->SetObjectTransform(ysMath::LoadIdentity());

    m_shaders.SetClearColor(ysColor::linearToSrgb(ysColor::srgbiToLinear(0x225599)));

    const int screenWidth = m_engine.GetGameWindow()->GetGameWidth();
    const int screenHeight = m_engine.GetGameWindow()->GetGameHeight();
    const float aspectRatio = screenWidth / (float)screenHeight;

    const Point cameraPos = m_engineView->getCameraPosition();
    m_shaders.m_cameraPosition = ysMath::LoadVector(cameraPos.x, cameraPos.y);

    m_shaders.CalculateUiCamera(screenWidth, screenHeight);

    m_screen = m_screen % 4;

    switch (m_screen)
    {
    case 3:
    {
        Bounds windowBounds((float)screenWidth, (float)screenHeight, {0, (float)screenHeight});
        Grid grid;
        grid.v_cells = 2;
        grid.h_cells = 3;
        Grid grid3x3;
        grid3x3.v_cells = 3;
        grid3x3.h_cells = 3;
        m_engineView->setDrawFrame(true);
        m_engineView->setBounds(grid.get(windowBounds, 1, 0, 1, 1));
        m_engineView->setLocalPosition({0, 0});

        m_rightGaugeCluster->m_bounds = grid.get(windowBounds, 2, 0, 1, 2);
        m_oscCluster->m_bounds = grid.get(windowBounds, 1, 1);
        m_performanceCluster->m_bounds = grid3x3.get(windowBounds, 0, 1);
        m_loadSimulationCluster->m_bounds = grid3x3.get(windowBounds, 0, 2);

        Grid grid1x3;
        grid1x3.v_cells = 3;
        grid1x3.h_cells = 1;
        m_mixerCluster->m_bounds = grid1x3.get(grid3x3.get(windowBounds, 0, 0), 0, 2);
        m_infoCluster->m_bounds = grid1x3.get(grid3x3.get(windowBounds, 0, 0), 0, 0, 1, 2);

        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(true);
        m_customGaugeCluster->setVisible(false);
        m_oscCluster->setVisible(true);
        m_performanceCluster->setVisible(true);
        m_loadSimulationCluster->setVisible(true);
        m_mixerCluster->setVisible(true);
        m_infoCluster->setVisible(true);

        m_oscCluster->activate();
        break;
    }
    case 1:
    {
        Bounds windowBounds((float)screenWidth, (float)screenHeight, {0, (float)screenHeight});
        m_engineView->setDrawFrame(false);
        m_engineView->setBounds(windowBounds);
        m_engineView->setLocalPosition({0, 0});
        m_engineView->activate();

        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(false);
        m_customGaugeCluster->setVisible(false);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(false);
        break;
    }
    case 2:
    {
        Bounds windowBounds((float)screenWidth, (float)screenHeight, {0, (float)screenHeight});
        Grid grid;
        grid.v_cells = 1;
        grid.h_cells = 3;
        m_engineView->setDrawFrame(true);
        m_engineView->setBounds(grid.get(windowBounds, 0, 0, 2, 1));
        m_engineView->setLocalPosition({0, 0});
        m_engineView->activate();

        m_rightGaugeCluster->m_bounds = grid.get(windowBounds, 2, 0, 1, 1);

        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(true);
        m_customGaugeCluster->setVisible(false);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(false);
        break;
    }
    case 0:
    {
        Bounds windowBounds((float)screenWidth, (float)screenHeight, {0, (float)screenHeight});
        Grid grid;
        grid.v_cells = 9;
        grid.h_cells = 16;

        Bounds engineViewBounds = grid.get(windowBounds, 0, 0, 16, 9);
        m_engineView->setDrawFrame(false);
        m_engineView->setBounds(engineViewBounds);
        m_engineView->setLocalPosition({0, 0});
        m_engineView->setVisible(true);

        m_infoCluster->m_bounds = grid.get(windowBounds, 0, 0, 16, 2);
        m_rightGaugeCluster->m_bounds = grid.get(windowBounds, 11, 0, 5, 9);
        m_loadSimulationCluster->m_bounds = grid.get(windowBounds, 0, 2, 5, 2);

        m_customGaugeCluster->m_bounds = grid.get(windowBounds, 0, 0, 16, 9);

        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_mixerCluster->setVisible(false);

        m_infoCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_rightGaugeCluster->setVisible(false);

        m_customGaugeCluster->setVisible(true);

        SDL_ShowCursor(SDL_DISABLE);

        break;
    }
    }

    const float cameraAspectRatio =
        m_engineView->m_bounds.width() / m_engineView->m_bounds.height();
    m_engine.GetDevice()->ResizeRenderTarget(
        m_mainRenderTarget,
        m_engineView->m_bounds.width(),
        m_engineView->m_bounds.height(),
        m_engineView->m_bounds.width(),
        m_engineView->m_bounds.height());
    m_engine.GetDevice()->RepositionRenderTarget(
        m_mainRenderTarget,
        m_engineView->m_bounds.getPosition(Bounds::tl).x,
        screenHeight - m_engineView->m_bounds.getPosition(Bounds::tl).y);

    bool isTargetEngine;

    if (m_screen == 0)
        isTargetEngine = false;
    else
        isTargetEngine = true;

    int mx, my, mz;
    m_engine.GetMousePos(&mx, &my);

    if (m_engine.IsMouseButtonDown(ysMouse::Button::Left))
    {
        float deltaX = +0.0004f * (mx - m_dragStartMousePosition.x);
        float deltaY = -0.0004f * (my - m_dragStartMousePosition.y);

        if (abs(deltaX) > 0.001f)
            m_cameraRotation.x += deltaX;

        if (abs(deltaY) > 0.001f)
            m_cameraRotation.y += deltaY;

        
    }
    else if (!isTargetEngine)
    {
        float rotationSpeed = 0.06f;
        m_cameraRotation.x = (1.0f - rotationSpeed) * m_cameraRotation.x - rotationSpeed * (m_simulator.getVehicle()->m_rotation + 0.5f * ysMath::Constants::PI);
        
        int ry = m_engine.GetJoystickAxisRY();

        if(abs(ry)>2000)
            m_cameraRotation.y += 0.000001f * ry;
    }
    else
    {
        int rx = m_engine.GetJoystickAxisRX();
        int ry = m_engine.GetJoystickAxisRY();

        if(abs(rx)>2000)
            m_cameraRotation.x -= 0.000001f * rx;
        if(abs(ry)>2000)
            m_cameraRotation.y += 0.000001f * ry;
    }

    m_cameraRotation.y = clamp(m_cameraRotation.y, 0.01f * ysMath::Constants::PI, 0.5f * ysMath::Constants::PI);

    if (!isTargetEngine)
    {
        float vehicleAngle = m_simulator.getVehicle()->m_rotation;
        float cameraTargetForward = 0.9f;

        m_shaders.CalculateCamera(
            cameraAspectRatio * m_displayHeight / m_engineView->m_zoom,
            m_displayHeight / m_engineView->m_zoom,

            m_engineView->m_bounds,

            m_screenWidth,
            m_screenHeight,

            m_fovY,

            m_cameraRotation.x,
            m_cameraRotation.y,
            m_zoom,

            m_simulator.getVehicle()->m_transform_camera->GetWorldPosition()[0] + cameraTargetForward * sin(vehicleAngle),
            m_simulator.getVehicle()->m_transform_camera->GetWorldPosition()[1],
            m_simulator.getVehicle()->m_transform_camera->GetWorldPosition()[2] + cameraTargetForward * cos(vehicleAngle));
    }
    else
    {
        m_shaders.CalculateCamera(
            cameraAspectRatio * m_displayHeight / m_engineView->m_zoom,
            m_displayHeight / m_engineView->m_zoom,

            m_engineView->m_bounds,

            m_screenWidth,
            m_screenHeight,

            m_fovY,

            m_cameraRotation.x,
            m_cameraRotation.y,
            m_zoom,

            m_simulator.getVehicle()->m_transform_engine->GetWorldPosition()[0],
            m_simulator.getVehicle()->m_transform_engine->GetWorldPosition()[1],
            m_simulator.getVehicle()->m_transform_engine->GetWorldPosition()[2] + m_simulator.getEngine()->getCylinderCount() / m_simulator.getEngine()->getCylinderBankCount() * 0.04f);
    }

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

void EngineSimApplication::refreshUserInterface()
{
    m_uiManager.destroy();
    m_uiManager.initialize(this);

    m_engineView = m_uiManager.getRoot()->addElement<EngineView>();
    m_rightGaugeCluster = m_uiManager.getRoot()->addElement<RightGaugeCluster>();
    m_customGaugeCluster = m_uiManager.getRoot()->addElement<CustomGaugeCluster>();
    m_oscCluster = m_uiManager.getRoot()->addElement<OscilloscopeCluster>();
    m_performanceCluster = m_uiManager.getRoot()->addElement<PerformanceCluster>();
    m_loadSimulationCluster = m_uiManager.getRoot()->addElement<LoadSimulationCluster>();
    m_mixerCluster = m_uiManager.getRoot()->addElement<MixerCluster>();
    m_infoCluster = m_uiManager.getRoot()->addElement<InfoCluster>();

    m_infoCluster->setEngine(m_iceEngine);

    m_rightGaugeCluster->m_simulator = &m_simulator;
    m_rightGaugeCluster->setEngine(m_iceEngine);

    m_customGaugeCluster->m_simulator = &m_simulator;
    m_customGaugeCluster->setEngine(m_iceEngine);

    m_oscCluster->setSimulator(&m_simulator);
    if (m_iceEngine != nullptr)
    {
        m_oscCluster->setDynoMaxRange(units::toRpm(m_iceEngine->getRedline()));
    }
    m_performanceCluster->setSimulator(&m_simulator);
    m_loadSimulationCluster->setSimulator(&m_simulator);
    m_mixerCluster->setSimulator(&m_simulator);
}

void EngineSimApplication::startRecording()
{
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

void EngineSimApplication::updateScreenSizeStability()
{
    m_screenResolution[m_screenResolutionIndex][0] = m_engine.GetScreenWidth();
    m_screenResolution[m_screenResolutionIndex][1] = m_engine.GetScreenHeight();

    m_screenResolutionIndex = (m_screenResolutionIndex + 1) % ScreenResolutionHistoryLength;
}

bool EngineSimApplication::readyToRecord()
{
    const int w = m_screenResolution[0][0];
    const int h = m_screenResolution[0][1];

    if (w <= 0 && h <= 0)
        return false;
    if ((w % 2) != 0 || (h % 2) != 0)
        return false;

    for (int i = 1; i < ScreenResolutionHistoryLength; ++i)
    {
        if (m_screenResolution[i][0] != w)
            return false;
        if (m_screenResolution[i][1] != h)
            return false;
    }

    return true;
}

void EngineSimApplication::stopRecording()
{
    m_recording = false;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    m_encoder.commit();
    m_encoder.stop();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

void EngineSimApplication::recordFrame()
{
#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    atg_dtv::Frame *frame = m_encoder.newFrame(false);
    if (frame != nullptr && m_encoder.getError() == atg_dtv::Encoder::Error::None)
    {
        m_engine.GetDevice()->ReadRenderTarget(m_engine.GetScreenRenderTarget(), frame->m_rgb);
    }

    m_encoder.submitFrame();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

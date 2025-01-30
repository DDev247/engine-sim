
#include "esbridge.h"

ESBRIDGE_API void ESBridge_CreateEngine(int cylinderBanks, int cylinderCount, int crankshaftCount, int exhaustCount, int intakeCount) {
    Engine::Parameters engParams;
    engParams.cylinderBanks = cylinderBanks;
    engParams.cylinderCount = cylinderCount;
    engParams.crankshaftCount = crankshaftCount;
    engParams.exhaustSystemCount = exhaustCount;
    engParams.intakeCount = intakeCount;
    engParams.name = "ESBridge Engine";

    DirectThrottleLinkage::Parameters dtlParams;
    dtlParams.gamma = 1.0;
    // TODO: make this static
    DirectThrottleLinkage* dtl = new DirectThrottleLinkage;
    dtl->initialize(dtlParams);
    engParams.throttle = dtl;

    // 10kHz for now
    engParams.initialSimulationFrequency = 10000.0;
    engParams.initialHighFrequencyGain = 1.0;
    engParams.initialNoise = 1.0;
    engParams.initialJitter = 1.0;

    ESBridge_Engine = new Engine;
    ESBridge_Engine->initialize(engParams);
}

ESBRIDGE_API void ESBridge_CreateBank(
        int bank, double bankAngle, double bankBore, double bankDeckHeight, double bankDisplayDepth, int bankCylinderCount,
        int crankshaft, double crankMass, double crankFlywheelMass, double crankMomentOfInertia, double crankThrow, double crankTdc, double crankFrictionTorque, double* crankJournals) {
    if (ESBridge_Engine->getCylinderBankCount() < bank) {
        throw std::runtime_error("bank number is too high");
    }

    Crankshaft* crk = ESBridge_Engine->getCrankshaft(crankshaft);
    Crankshaft::Parameters crkParams;
    crkParams.mass = crankMass;
    crkParams.flywheelMass = crankFlywheelMass;
    crkParams.momentOfInertia = crankMomentOfInertia;
    crkParams.crankThrow = crankThrow;
    crkParams.tdc = crankTdc;
    crkParams.frictionTorque = crankFrictionTorque;
    crkParams.rodJournals = bankCylinderCount;
    crk = new Crankshaft;
    crk->initialize(crkParams);
    for (int i = 0; i < bankCylinderCount; i++) {
        crk->setRodJournalAngle(i, crankJournals[i]);
    }

    CylinderBank* bnk = ESBridge_Engine->getCylinderBank(bank);
    CylinderBank::Parameters bnkParams;
    bnkParams.crankshaft = crk;
    bnkParams.positionX = 0;
    bnkParams.positionY = 0;
    bnkParams.angle = bankAngle;
    bnkParams.bore = bankBore;
    bnkParams.deckHeight = bankDeckHeight;
    bnkParams.displayDepth = bankDisplayDepth;
    bnkParams.cylinderCount = bankCylinderCount;
    bnkParams.index = bank;
    bnk = new CylinderBank;
    bnk->initialize(bnkParams);
}

ESBRIDGE_API Function* ESBridge_CreateFunction(int length, double* x, double* y, double filterRadius) {
    Function* fun = new Function;
    fun->initialize(length, filterRadius);
    for (int i = 1; i <= length; i++) {
        fun->addSample(x[i - 1], y[i - 1]);
    }

    return fun;
}

ESBRIDGE_API ESBridge_ValvetrainStruct* ESBridge_CreateStandardValvetrain(int crankshaft, int lobes,
    Function* intake, double* intakeLobes, Function* exhaust, double* exhaustLobes) {
    ESBridge_ValvetrainStruct* vs = new ESBridge_ValvetrainStruct;
    vs->crankshaft = crankshaft;
    vs->lobes = lobes;
    vs->intake = intake;
    vs->intakeLobes = intakeLobes;
    vs->exhaust = exhaust;
    vs->exhaustLobes = exhaustLobes;
    return vs;
}

ESBRIDGE_API ESBridge_ValvetrainStruct* ESBridge_CreateVTECValvetrain(int crankshaft, int lobes,
    Function* intake, double* intakeLobes, Function* exhaust, double* exhaustLobes,
    Function* vtecIntake, double* vtecIntakeLobes, Function* vtecExhaust, double* vtecExhaustLobes,
    double vtecMinRpm, double vtecMinSpeed, double vtecMinThrottlePosition, double vtecManifoldVacuum) {
    ESBridge_ValvetrainStruct* vs = new ESBridge_ValvetrainStruct;
    vs->crankshaft = crankshaft;
    vs->lobes = lobes;
    vs->intake = intake;
    vs->intakeLobes = intakeLobes;
    vs->exhaust = exhaust;
    vs->exhaustLobes = exhaustLobes;

    vs->vtec = true;

    vs->vtecIntake = vtecIntake;
    vs->vtecIntakeLobes = vtecIntakeLobes;
    vs->vtecExhaust = vtecExhaust;
    vs->vtecExhaustLobes = vtecExhaustLobes;
    vs->vtecMinRpm = vtecMinRpm;
    vs->vtecMinSpeed = vtecMinSpeed;
    vs->vtecMinThrottlePosition = vtecMinThrottlePosition;
    vs->vtecManifoldVacuum = vtecManifoldVacuum;
    return vs;
}

ESBRIDGE_API void ESBridge_CreateCylinderHead(
    int head, int bank,
    Function* intakePortFlow, Function* exhaustPortFlow, ESBridge_ValvetrainStruct* valvetrain,
    double headCombustionVolume, double headIntakeRunnerVolume, double headIntakeRunnerArea, double headExhaustRunnerVolume, double headExhaustRunnerArea) {
    if (ESBridge_Engine->getCylinderBankCount() < head) {
        throw std::runtime_error("head number is too high");
    }

    if (ESBridge_Engine->getCylinderBankCount() < bank) {
        throw std::runtime_error("bank number is too high");
    }

    CylinderHead* hd = ESBridge_Engine->getHead(head);
    CylinderHead::Parameters hdParams;
    hdParams.Bank = ESBridge_Engine->getCylinderBank(bank);
    hdParams.CombustionChamberVolume = headCombustionVolume;
    hdParams.IntakeRunnerVolume = headIntakeRunnerVolume;
    hdParams.IntakeRunnerCrossSectionArea = headIntakeRunnerArea;
    hdParams.ExhaustRunnerVolume = headExhaustRunnerVolume;
    hdParams.ExhaustRunnerCrossSectionArea = headExhaustRunnerArea;
    hdParams.IntakePortFlow = intakePortFlow;
    hdParams.ExhaustPortFlow = exhaustPortFlow;
    
    Valvetrain* valves;
    if (valvetrain->vtec) {
        // create valvetrain
        VtecValvetrain* v = new VtecValvetrain;

        // create camshafts
        Camshaft* ishaft = new Camshaft;
        Camshaft::Parameters ishaftParams;
        ishaftParams.crankshaft = ESBridge_Engine->getCrankshaft(valvetrain->crankshaft);
        ishaftParams.lobeProfile = valvetrain->intake;
        ishaftParams.lobes = valvetrain->lobes;
        ishaft->initialize(ishaftParams);

        Camshaft* eshaft = new Camshaft;
        Camshaft::Parameters eshaftParams;
        eshaftParams.crankshaft = ESBridge_Engine->getCrankshaft(valvetrain->crankshaft);
        eshaftParams.lobeProfile = valvetrain->exhaust;
        eshaftParams.lobes = valvetrain->lobes;
        eshaft->initialize(eshaftParams);

        Camshaft* vishaft = new Camshaft;
        Camshaft::Parameters vishaftParams;
        vishaftParams.crankshaft = ESBridge_Engine->getCrankshaft(valvetrain->crankshaft);
        vishaftParams.lobeProfile = valvetrain->vtecIntake;
        vishaftParams.lobes = valvetrain->lobes;
        vishaft->initialize(vishaftParams);

        Camshaft* veshaft = new Camshaft;
        Camshaft::Parameters veshaftParams;
        veshaftParams.crankshaft = ESBridge_Engine->getCrankshaft(valvetrain->crankshaft);
        veshaftParams.lobeProfile = valvetrain->vtecExhaust;
        veshaftParams.lobes = valvetrain->lobes;
        veshaft->initialize(veshaftParams);

        // add lobes
        for (int i = 0; i < valvetrain->lobes; i++) {
            ishaft->setLobeCenterline(i, valvetrain->intakeLobes[i]);
            eshaft->setLobeCenterline(i, valvetrain->exhaustLobes[i]);
            vishaft->setLobeCenterline(i, valvetrain->vtecIntakeLobes[i]);
            veshaft->setLobeCenterline(i, valvetrain->vtecExhaustLobes[i]);
        }

        // initialise valvetrain
        VtecValvetrain::Parameters vParams;
        vParams.minRpm = valvetrain->vtecMinRpm;
        vParams.minSpeed = valvetrain->vtecMinSpeed;
        vParams.manifoldVacuum = valvetrain->vtecManifoldVacuum;
        vParams.minThrottlePosition = valvetrain->vtecMinThrottlePosition;
        vParams.intakeCamshaft = ishaft;
        vParams.exhaustCamshaft = eshaft;
        vParams.vtecIntakeCamshaft = vishaft;
        vParams.vtexExhaustCamshaft = veshaft;
        vParams.engine = ESBridge_Engine;
        v->initialize(vParams);

        valves = v;
    }
    else {
        // create valvetrain
        StandardValvetrain* v = new StandardValvetrain;

        // create camshafts
        Camshaft* ishaft = new Camshaft;
        Camshaft::Parameters ishaftParams;
        ishaftParams.crankshaft = ESBridge_Engine->getCrankshaft(valvetrain->crankshaft);
        ishaftParams.lobeProfile = valvetrain->intake;
        ishaftParams.lobes = valvetrain->lobes;
        ishaft->initialize(ishaftParams);

        Camshaft* eshaft = new Camshaft;
        Camshaft::Parameters eshaftParams;
        eshaftParams.crankshaft = ESBridge_Engine->getCrankshaft(valvetrain->crankshaft);
        eshaftParams.lobeProfile = valvetrain->exhaust;
        eshaftParams.lobes = valvetrain->lobes;
        eshaft->initialize(eshaftParams);

        // add lobes
        for (int i = 0; i < valvetrain->lobes; i++) {
            ishaft->setLobeCenterline(i, valvetrain->intakeLobes[i]);
            eshaft->setLobeCenterline(i, valvetrain->exhaustLobes[i]);
        }

        // initialise valvetrain
        StandardValvetrain::Parameters vParams;
        vParams.intakeCamshaft = ishaft;
        vParams.exhaustCamshaft = eshaft;
        v->initialize(vParams);

        valves = v;
    }

    hdParams.Valvetrain = valves;

    hd = new CylinderHead;
    hd->initialize(hdParams);
}

ESBRIDGE_API void ESBridge_CreateCylinder(int bank, int cylinder, int crankshaft, int intake, int exhaust,
    double pistonBlowby, double pistonCompressionHeight, double pistonWristPinPosition, double pistonDisplacement, double pistonMass,
    double rodMass, double rodMomentOfInertia, double rodCenterOfMass, double rodLength) {
    if (ESBridge_Engine->getCylinderBankCount() < bank) {
        throw std::runtime_error("bank number is too high");
    }

    Piston* p = ESBridge_Engine->getPiston(cylinder);
    ConnectingRod* cr = ESBridge_Engine->getConnectingRod(cylinder);

    Piston::Parameters pParams;
    pParams.Bank = ESBridge_Engine->getCylinderBank(bank);
    pParams.Rod = cr;
    pParams.CylinderIndex = cylinder;
    pParams.BlowbyFlowCoefficient = pistonBlowby;
    pParams.CompressionHeight = pistonCompressionHeight;
    pParams.WristPinPosition = pistonWristPinPosition;
    pParams.Displacement = pistonDisplacement;
    pParams.mass = pistonMass;

    ConnectingRod::Parameters crParams;
    crParams.mass = rodMass;
    crParams.momentOfInertia = rodMomentOfInertia;
    crParams.centerOfMass = rodCenterOfMass;
    crParams.length = rodLength;
    crParams.piston = p;
    crParams.crankshaft = ESBridge_Engine->getCrankshaft(crankshaft);

    p = new Piston;
    p->initialize(pParams);

    cr = new ConnectingRod;
    cr->initialize(crParams);
}

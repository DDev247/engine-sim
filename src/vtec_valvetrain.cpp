#include "../include/vtec_valvetrain.h"

#include "../include/lerp_function.h"

#include "../include/engine.h"

VtecValvetrain::VtecValvetrain() {
    m_intakeCamshaft = nullptr;
    m_exhaustCamshaft = nullptr;

    m_vtecIntakeCamshaft = nullptr;
    m_vtecExhaustCamshaft = nullptr;

    m_engine = nullptr;

    m_minRpm = 0.0;
    m_minSpeed = 0.0;
    m_minThrottlePosition = 0.0;
    m_manifoldVacuum = 0.0;

    c = nullptr;
    c2 = nullptr;
}

VtecValvetrain::~VtecValvetrain() {
    /* void */
}

void VtecValvetrain::initialize(const Parameters &parameters) {
    m_intakeCamshaft = parameters.intakeCamshaft;
    m_exhaustCamshaft = parameters.exhaustCamshaft;
    m_vtecIntakeCamshaft = parameters.vtecIntakeCamshaft;
    m_vtecExhaustCamshaft = parameters.vtexExhaustCamshaft;

    m_minRpm = parameters.minRpm;
    m_minSpeed = parameters.minSpeed;
    m_minThrottlePosition = parameters.minThrottlePosition;
    m_manifoldVacuum = parameters.manifoldVacuum;
    m_engine = parameters.engine;
}

double VtecValvetrain::intakeValveLift(int cylinder) {
    return LerpFunction::lerp(m_intakeCamshaft->valveLift(cylinder), m_vtecIntakeCamshaft->valveLift(cylinder), m_vtecPct / 100);
}

double VtecValvetrain::exhaustValveLift(int cylinder) {
    return LerpFunction::lerp(m_exhaustCamshaft->valveLift(cylinder), m_vtecExhaustCamshaft->valveLift(cylinder), m_vtecPct / 100);
}

Camshaft *VtecValvetrain::getActiveIntakeCamshaft() {
    // TODO: fix this mess
    /*if (c != nullptr) {
        c->destroy();
        delete c;
    }

    c = (Camshaft*)malloc(sizeof(Camshaft));

    Camshaft::Parameters params;
    params.advance = LerpFunction::lerp(m_intakeCamshaft->getAdvance(), m_vtecIntakeCamshaft->getAdvance(), m_vtecPct / 100);
    params.baseRadius = LerpFunction::lerp(m_intakeCamshaft->getBaseRadius(), m_vtecIntakeCamshaft->getBaseRadius(), m_vtecPct / 100);
    params.crankshaft = m_intakeCamshaft->getCrankshaft();
    params.lobeProfile = &LerpFunction(m_intakeCamshaft->getLobeProfile(), m_vtecIntakeCamshaft->getLobeProfile(), m_vtecPct / 100);
    params.lobes = (int) LerpFunction::lerp(m_intakeCamshaft->getLobes(), m_vtecIntakeCamshaft->getLobes(), m_vtecPct / 100);
    
    c->initialize(params);

    return c;*/

    return isVtecEnabled()
        ? m_vtecIntakeCamshaft
        : m_intakeCamshaft;
}

Camshaft *VtecValvetrain::getActiveExhaustCamshaft() {
    /*if (c2 != nullptr) {
        c2->destroy();
        delete c2;
    }

    c2 = (Camshaft*)malloc(sizeof(Camshaft));

    Camshaft::Parameters params;
    params.advance = LerpFunction::lerp(m_exhaustCamshaft->getAdvance(), m_vtecExhaustCamshaft->getAdvance(), m_vtecPct / 100);
    params.baseRadius = LerpFunction::lerp(m_exhaustCamshaft->getBaseRadius(), m_vtecExhaustCamshaft->getBaseRadius(), m_vtecPct / 100);
    params.crankshaft = m_exhaustCamshaft->getCrankshaft();
    params.lobeProfile = &LerpFunction(m_exhaustCamshaft->getLobeProfile(), m_vtecExhaustCamshaft->getLobeProfile(), m_vtecPct / 100);
    params.lobes = (int)LerpFunction::lerp(m_exhaustCamshaft->getLobes(), m_vtecExhaustCamshaft->getLobes(), m_vtecPct / 100);

    c2->initialize(params);

    return c2;*/
    
    return isVtecEnabled()
        ? m_vtecExhaustCamshaft
        : m_exhaustCamshaft;
}

bool VtecValvetrain::isVtecEnabled() const {
    return m_vtecEnabled;

    // we dont need that
    /*return
        m_engine->getManifoldPressure() > m_manifoldVacuum
        && m_engine->getSpeed() > m_minRpm
        && (1 - m_engine->getThrottle()) > m_minThrottlePosition;*/
}

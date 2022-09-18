#include "../include/turbocharger_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"

TurbochargerObject::TurbochargerObject() {
    m_turbocharger = nullptr;
    m_turbo = nullptr;
}

TurbochargerObject::~TurbochargerObject() {
    /* void */
}

void TurbochargerObject::generateGeometry() {
    GeometryGenerator* gen = m_app->getGeometryGenerator();

    //GeometryGenerator::Circle2dParameters circleParams;
    //circleParams.center_x = 0.0f;
    //circleParams.center_y = (float)m_piston->getWristPinLocation();
    //circleParams.maxEdgeLength = m_app->pixelsToUnits(5.0f);
    //circleParams.radius = (float)(m_piston->getCylinderBank()->getBore() / 10) * 0.75f;
    //gen->startShape();
    //gen->generateCircle2d(circleParams);
    //gen->endShape(&m_wristPinHole);
}

float turboangle = 0;
void TurbochargerObject::render(const ViewParameters* view) {
    const int layer = 0;
    if (layer > view->Layer1 || layer < view->Layer0) return;

    const ysVector col = tintByLayer(m_app->getBlue(), layer - view->Layer0);
    const ysVector col2 = tintByLayer(m_app->getPink(), layer - view->Layer0);
    const ysVector holeCol = tintByLayer(m_app->getBackgroundColor(), layer - view->Layer0);
    
    resetShader();

    if (this->m_turbocharger->spoolSpin > 0.1) {
        turboangle += this->m_turbocharger->spoolSpin * this->angleMultiplier;
        this->m_turbocharger->spoolSpin -= 0.005;
    }

    if (this->m_turbocharger->spoolSpin >= this->m_turbocharger->wastegateTrigger) {
        this->m_turbocharger->spoolSpin = this->m_turbocharger->wastegateTrigger;
    }

    if (this->m_turbocharger->spoolSpin <= 0.1)
        this->m_turbocharger->spoolSpin = 0;

    float angle = fmod(turboangle, 360) * 0.01745329251f;
    const float scaleMultiplier = 0.53f;
    const float positionX = 0.12f;
    const float positionY = 0.298f;
    const float bodyRotation = 270.0f * 0.01745329251f;

    setTransform(
        &m_turbo->m_body,
        0.075f * scaleMultiplier,
        0.0f + positionX,
        0.0f + positionY,
        angle);

    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TurboImpeller"),
        0x32 - layer + 3);

    m_app->getShaders()->SetBaseColor(holeCol);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TurboHole"),
        0x32 - layer + 1);

    setTransform(
        &m_turbo->m_body,
        0.02f * scaleMultiplier,
        0.0f + positionX,
        0.0f + positionY,
        0.0f);

    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TurboHole"),
        0x32 - layer + 2);

    setTransform(
        &m_turbo->m_body,
        0.1f * scaleMultiplier,
        0.0f + positionX,
        0.0f + positionY,
        bodyRotation);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TurboBody"),
        0x32 - layer);
}

void TurbochargerObject::process(float dt) {
    //this->rotation += this->m_turbocharger->spool * this->angleMultiplier * dt;
    /* void */
}

void TurbochargerObject::destroy() {
    /* void */
}

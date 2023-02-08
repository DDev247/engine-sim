#include "../include/supercharger_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"
#include "../include/logger.h"

SuperchargerObject::SuperchargerObject() {
    m_supercharger = nullptr;
    m_charger = nullptr;
}

SuperchargerObject::~SuperchargerObject() {
    /* void */
}

void SuperchargerObject::generateGeometry() {
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

void SuperchargerObject::setTransformm(
    atg_scs::RigidBody* rigidBody,
    float scalex,
    float scaley,
    float lx,
    float ly,
    float angle)
{
    double p_x, p_y;
    rigidBody->localToWorld(lx, ly, &p_x, &p_y);

    const ysMatrix rot = ysMath::RotationTransform(
        ysMath::Constants::ZAxis,
        (float)rigidBody->theta + angle);
    const ysMatrix trans = ysMath::TranslationTransform(
        ysMath::LoadVector((float)p_x, (float)p_y, 0.0f));
    const ysMatrix scaleTransform = ysMath::ScaleTransform(ysMath::LoadVector(scalex, scaley, 0.0f));

    m_app->getShaders()->SetObjectTransform(
        ysMath::MatMult(ysMath::MatMult(trans, rot), scaleTransform));
}

float chargerangle = 0;
void SuperchargerObject::render(const ViewParameters* view) {
    const int layer = 0;
    //if (layer > view->Layer1 || layer < view->Layer0) return;
    if (view->Sublayer != 2) return;

    const ysVector col = tintByLayer(m_app->getBlue(), layer - view->Layer0);
    const ysVector col2 = tintByLayer(m_app->getPink(), layer - view->Layer0);
    const ysVector holeCol = tintByLayer(m_app->getBackgroundColor(), layer - view->Layer0);
    
    resetShader();

    if (this->m_supercharger->spoolSpin > 0.1) {
        double simSpeed = this->m_app->getSimulator()->getSimulationSpeed();
        //Logger::DebugLine(std::to_string(simSpeed));
        chargerangle += (this->m_supercharger->spoolSpin * this->angleMultiplier) * simSpeed;
        //this->m_supercharger->spoolSpin -= 0.005;
    }

    if (this->m_supercharger->spoolSpin <= 0.1)
        this->m_supercharger->spoolSpin = 0;

    float TBscale = m_app->m_iceEngine->getThrottle();

    float angle = fmod(chargerangle, 360) * 0.01745329251f;
    float angle1 = fmod(chargerangle + 120, 360) * 0.01745329251f;
    float angle2 = fmod(chargerangle + 240, 360) * 0.01745329251f;

    float angles = -60 * 0.01745329251f;
    const float scaleMultiplier = 0.75f;
    //const float positionX = 0.12f;
    const float positionX = 0.0f;
    const float positionY = 0.198f;

    const float lobePositionX1 = 0.024f;
    const float lobePositionX2 = -0.024f;
    const float lobePositionY = 0.088f;

    const float lobeScale = 0.025f*1.05;
    const float lobeScale2 = 0.021f*1.05;

    const float bodyRotation = 270.0f * 0.01745329251f;

#pragma region LOBE 1

    setTransform(
        &m_charger->m_body,
        lobeScale * scaleMultiplier,
        0.0f + positionX + lobePositionX1,
        0.0f + positionY + lobePositionY,
        angle);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe"),
        0x32 - layer + 3);

    /*
    setTransform(
        &m_charger->m_body,
        lobeScale2 * scaleMultiplier,
        0.0f + positionX + lobePositionX1,
        0.0f + positionY + lobePositionY,
        angle);

    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe-001"),
        0x32 - layer + 3);

    setTransform(
        &m_charger->m_body,
        lobeScale2 * scaleMultiplier,
        0.0f + positionX + lobePositionX1,
        0.0f + positionY + lobePositionY,
        angle1);

    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe-002"),
        0x32 - layer + 3);

    setTransform(
        &m_charger->m_body,
        lobeScale2 * scaleMultiplier,
        0.0f + positionX + lobePositionX1,
        0.0f + positionY + lobePositionY,
        angle2);

    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe-003"),
        0x32 - layer + 3);
    */

#pragma endregion

#pragma region LOBE 2

    setTransform(
        &m_charger->m_body,
        lobeScale * scaleMultiplier,
        0.0f + positionX + lobePositionX2,
        0.0f + positionY + lobePositionY,
        angles + -angle);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe"),
        0x32 - layer + 3);

    /*
    setTransform(
        &m_charger->m_body,
        lobeScale2 * scaleMultiplier,
        0.0f + positionX + lobePositionX2,
        0.0f + positionY + lobePositionY,
        angles + -angle);

    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe-001"),
        0x32 - layer + 3);

    setTransform(
        &m_charger->m_body,
        lobeScale2 * scaleMultiplier,
        0.0f + positionX + lobePositionX2,
        0.0f + positionY + lobePositionY,
        angles + -angle1);

    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe-002"),
        0x32 - layer + 3);

    setTransform(
        &m_charger->m_body,
        lobeScale2 * scaleMultiplier,
        0.0f + positionX + lobePositionX2,
        0.0f + positionY + lobePositionY,
        angles + -angle2);

    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Lobe-003"),
        0x32 - layer + 3);
    */

#pragma endregion

    setTransform(
        &m_charger->m_body,
        0.1f * scaleMultiplier,
        0.0f + positionX,
        0.16f + positionY,
        0.0f);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Supercharger-Throttle"),
        0x32 - layer + 1);

    setTransformm(
        &m_charger->m_body,
        0.03f * scaleMultiplier,
        0.03f * TBscale * scaleMultiplier,
        -0.06f + positionX,
        0.2f + positionY,
        0.0f);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TB"),
        0x32 - layer + 1);

    setTransformm(
        &m_charger->m_body,
        0.03f * scaleMultiplier,
        0.03f * scaleMultiplier,
        -0.06f + positionX,
        0.2f + positionY,
        0.0f);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TB-Outline"),
        0x32 - layer + 1);

    setTransformm(
        &m_charger->m_body,
        0.04f * scaleMultiplier,
        0.04f * TBscale * scaleMultiplier,
        0.0f + positionX,
        0.2f + positionY,
        0.0f);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TB"),
        0x32 - layer + 1);

    setTransformm(
        &m_charger->m_body,
        0.04f * scaleMultiplier,
        0.04f * scaleMultiplier,
        0.0f + positionX,
        0.2f + positionY,
        0.0f);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TB-Outline"),
        0x32 - layer + 1);

    setTransformm(
        &m_charger->m_body,
        0.03f * scaleMultiplier,
        0.03f * TBscale * scaleMultiplier,
        0.06f + positionX,
        0.2f + positionY,
        0.0f);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TB"),
        0x32 - layer + 1);

    setTransformm(
        &m_charger->m_body,
        0.03f * scaleMultiplier,
        0.03f * scaleMultiplier,
        0.06f + positionX,
        0.2f + positionY,
        0.0f);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("TB-Outline"),
        0x32 - layer + 1);

    setTransform(
        &m_charger->m_body,
        0.1f * scaleMultiplier,
        0.0f + positionX,
        0.0f + positionY,
        0.0f);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Supercharger-Outline"),
        0x32 - layer + 2);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Supercharger-Top-Outline"),
        0x32 - layer + 2);

    setTransform(
        &m_charger->m_body,
        0.04f * scaleMultiplier,
        0.0f + positionX,
        0.025f + positionY,
        0.0f);

    m_app->getShaders()->SetBaseColor(col2);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Supercharger-Body"),
        0x32 - layer);
}

void SuperchargerObject::process(float dt) {
    //this->rotation += this->m_turbocharger->spool * this->angleMultiplier * dt;
    /* void */
}

void SuperchargerObject::destroy() {
    /* void */
}

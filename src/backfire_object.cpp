#include "../include/backfire_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"
#include "../include/constants.h"

BackfireObject::BackfireObject() {
    m_chamber = nullptr;
}

BackfireObject::~BackfireObject() {
    /* void */
}

void BackfireObject::generateGeometry() {
    /*GeometryGenerator* gen = m_app->getGeometryGenerator();
    CylinderHead *head = m_chamber->getCylinderHead();
    CylinderBank *bank = head->getCylinderBank();

    const float lineWidth = (float)m_chamber->m_flameEvent.travel_x * 2;
    double flameTop_x, flameTop_y;
    double flameBottom_x, flameBottom_y;
    double chamberHeight = head->getCombustionChamberVolume() / bank->boreSurfaceArea();

    bank->getPositionAboveDeck(chamberHeight, &flameTop_x, &flameTop_y);
    bank->getPositionAboveDeck(chamberHeight - m_chamber->m_flameEvent.travel_y, &flameBottom_x, &flameBottom_y);

    GeometryGenerator::Line2dParameters params;
    params.lineWidth = lineWidth;

    gen->startShape();

    params.x0 = (float)flameTop_x;
    params.y0 = (float)flameTop_y;
    params.x1 = (float)flameBottom_x;
    params.y1 = (float)flameBottom_y;
    gen->generateLine2d(params);

    gen->endShape(&m_indices);*/
}

void BackfireObject::render(const ViewParameters* view) {
    resetShader();

    CylinderHead* head = m_chamber->getCylinderHead();
    CylinderBank* bank = head->getCylinderBank();

    if (m_app->getSimulator()->backfiring) {

        m_app->getShaders()->SetBaseColor(
            ysMath::Mul(
                m_app->getOrange(),
                ysMath::LoadVector(1.0f, 1.0f, 1.0f, 0.6f)));

        setTransform(
            &head->m_body,
            0.1f,
            0.0f + positionX + lobePositionX1,
            0.0f + positionY + lobePositionY,
            angle);

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Lobe"),
            0x32 - layer + 3);
    }

    /*
    Piston *frontmostPiston = getForemostPiston(bank, view->Layer0);
    if (m_chamber->getPiston() == frontmostPiston) {
        if (m_chamber->m_lit) {
            m_app->getShaders()->SetBaseColor(
                ysMath::Mul(
                    m_app->getOrange(),
                    ysMath::LoadVector(1.0f, 1.0f, 1.0f, 0.6f)));
            m_app->drawGenerated(m_indices, 0x35);
        }
    }*/
}

void BackfireObject::process(float dt) {
    /* void */
}

void BackfireObject::destroy() {
    /* void */
}

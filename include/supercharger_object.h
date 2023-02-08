#ifndef DDEV_ENGINE_SIM_SUPERCHARGER_OBJECT_H
#define DDEV_ENGINE_SIM_SUPERCHARGER_OBJECT_H

#include "simulation_object.h"

#include "supercharger.h"
#include "charger.h"
#include "geometry_generator.h"

class SuperchargerObject : public SimulationObject {
public:
    SuperchargerObject();
    //SuperchargerObject(TurboCharger charger);
    virtual ~SuperchargerObject();

    virtual void generateGeometry();
    virtual void render(const ViewParameters* view);
    virtual void process(float dt);
    virtual void destroy();

    SuperCharger* m_supercharger;
    Charger* m_charger;
    
    void setTransformm(
        atg_scs::RigidBody* rigidBody,
        float scalex,
        float scaley,
        float lx,
        float ly,
        float angle);

protected:
    float rotation = 0.0f;
    const float angleMultiplier = 2.0f;

//protected:
//    GeometryGenerator::GeometryIndices
//        m_wristPinHole;
};

#endif /* DDEV_ENGINE_SIM_SUPERCHARGER_OBJECT_H */

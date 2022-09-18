#ifndef DDEV_ENGINE_SIM_TURBOCHARGER_OBJECT_H
#define DDEV_ENGINE_SIM_TURBOCHARGER_OBJECT_H

#include "simulation_object.h"

#include "turbocharger.h"
#include "turbo.h"
#include "turbo.h"
#include "geometry_generator.h"

class TurbochargerObject : public SimulationObject {
public:
    TurbochargerObject();
    //TurbochargerObject(TurboCharger charger);
    virtual ~TurbochargerObject();

    virtual void generateGeometry();
    virtual void render(const ViewParameters* view);
    virtual void process(float dt);
    virtual void destroy();

    TurboCharger* m_turbocharger;
    Turbo* m_turbo;

protected:
    float rotation = 0.0f;
    const float angleMultiplier = 2.0f;

//protected:
//    GeometryGenerator::GeometryIndices
//        m_wristPinHole;
};

#endif /* DDEV_ENGINE_SIM_TURBOCHARGER_OBJECT_H */

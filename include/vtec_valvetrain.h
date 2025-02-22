#ifndef ATG_ENGINE_SIM_VTEC_STANDARD_VALVETRAIN_H
#define ATG_ENGINE_SIM_VTEC_STANDARD_VALVETRAIN_H

#include "valvetrain.h"

class Engine;
class VtecValvetrain : public Valvetrain {
public:
    struct Parameters {
        double minRpm;
        double minSpeed;
        double manifoldVacuum;
        double minThrottlePosition;

        Camshaft *intakeCamshaft;
        Camshaft *exhaustCamshaft;

        Camshaft *vtecIntakeCamshaft;
        Camshaft *vtexExhaustCamshaft;

        Engine *engine;
    };

public:
    VtecValvetrain();
    virtual ~VtecValvetrain();

    void initialize(const Parameters &parameters);

    virtual double intakeValveLift(int cylinder) override;
    virtual double exhaustValveLift(int cylinder) override;

    virtual Camshaft *getActiveIntakeCamshaft() override;
    virtual Camshaft *getActiveExhaustCamshaft() override;

private:
    bool isVtecEnabled() const;

    Camshaft *m_intakeCamshaft;
    Camshaft *m_exhaustCamshaft;

    Camshaft *m_vtecIntakeCamshaft;
    Camshaft *m_vtecExhaustCamshaft;

    Engine *m_engine;

    double m_minRpm;
    double m_minSpeed;
    double m_manifoldVacuum;
    double m_minThrottlePosition;
    Camshaft* c;
    Camshaft* c2;
};

#endif /* ATG_ENGINE_SIM_VTEC_STANDARD_VALVETRAIN_H */

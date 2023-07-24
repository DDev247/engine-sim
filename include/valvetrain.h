#ifndef ATG_ENGINE_SIM_VALVETRAIN_H
#define ATG_ENGINE_SIM_VALVETRAIN_H

class Camshaft;
class Valvetrain {
public:
    Valvetrain();
    virtual ~Valvetrain();

    virtual double intakeValveLift(int cylinder) = 0;
    virtual double exhaustValveLift(int cylinder) = 0;

    virtual Camshaft *getActiveIntakeCamshaft() = 0;
    virtual Camshaft *getActiveExhaustCamshaft() = 0;

    bool m_vtecEnabled = false;
    float m_vtecPct = 0.0f;
};

#endif /* ATG_ENGINE_SIM_VALVETRAIN_H */

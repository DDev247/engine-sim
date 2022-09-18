#ifndef DDEV_ENGINE_SIM_TURBO_H
#define DDEV_ENGINE_SIM_TURBO_H

#include "part.h"

class TurboCharger;
class Turbo : public Part {
public:
    struct Parameters {
        TurboCharger* turbocharger;
    };

public:
    Turbo();
    virtual ~Turbo();

    void initialize(const Parameters& params);
    inline void setCylinderConstraint(atg_scs::LineConstraint* constraint);
    virtual void destroy();

    double relativeX() const;
    double relativeY() const;

    //double calculateCylinderWallForce() const;
    //inline ConnectingRod* getRod() const { return m_rod; }
    //inline CylinderBank* getCylinderBank() const { return m_bank; }
    //inline int getCylinderIndex() const { return m_cylinderIndex; }
    //inline double getCompressionHeight() const { return m_compressionHeight; }
    //inline double getDisplacement() const { return m_displacement; }
    //inline double getWristPinLocation() const { return m_wristPinLocation; }
    //inline double getMass() const { return m_mass; }
    //inline double getBlowbyK() const { return m_blowby_k; }
    
    
protected:
    TurboCharger* m_turbocharger;
    /*
    ConnectingRod* m_rod;
    CylinderBank* m_bank;
    atg_scs::LineConstraint* m_cylinderConstraint;
    int m_cylinderIndex;
    double m_compressionHeight;
    double m_displacement;
    double m_wristPinLocation;
    double m_mass;
    double m_blowby_k;
    */
};

#endif /* DDEV_ENGINE_SIM_TURBO_H */

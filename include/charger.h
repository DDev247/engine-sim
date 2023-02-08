#ifndef DDEV_ENGINE_SIM_CHARGER_H
#define DDEV_ENGINE_SIM_CHARGER_H

#include "part.h"

class SuperCharger;
class Charger : public Part {
public:
    struct Parameters {
        SuperCharger* supercharger;
    };

public:
    Charger();
    virtual ~Charger();

    void initialize(const Parameters& params);
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
    SuperCharger* m_supercharger;
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

#endif /* DDEV_ENGINE_SIM_CHARGER_H */

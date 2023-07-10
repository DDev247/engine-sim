#ifndef ATG_ENGINE_SIM_IGNITION_MODULE_H
#define ATG_ENGINE_SIM_IGNITION_MODULE_H

#include "part.h"

#include "crankshaft.h"
#include "function.h"
#include "units.h"

class IgnitionModule : public Part {
    public:
        struct Parameters {
            int cylinderCount;
            Crankshaft *crankshaft;
            Function *timingCurve;
            double revLimit = units::rpm(6000.0);
            double limiterDuration = 0.5 * units::sec;
        };

        struct SparkPlug {
            double angle = 0;
            bool ignitionEvent = false;
            bool enabled = false;
        };

    public:
        IgnitionModule();
        virtual ~IgnitionModule();

        virtual void destroy();

        void initialize(const Parameters &params);
        void setFiringOrder(int cylinderIndex, double angle);
        void reset();
        void update(double dt);

        bool getIgnitionEvent(int index) const;
        void resetIgnitionEvents();

        double getTimingAdvance();

        bool m_enabled;

        bool m_2stepEnabled = false;
        double m_2stepSoftCutLimit = 4000;
        double m_2stepSoftCutAngle = -20;
        double m_2stepHardCutLimit = 5000;
        
        bool m_3stepEnabled = false;
        double m_3stepSoftCutLimit = 5500;
        double m_3stepSoftCutAngle = -20;

        bool m_launchingSoft = false;
        bool m_launchingHard = false;

        bool m_shiftingHard = false;

        double m_lastCrankshaftAngle;
        double m_revLimit;
        double m_revLimitTimer;
        double m_limiterDuration;

        double m_currentTableValue = 30;
        double m_retardAmount = 0;
        // set this to true to have absolute timing retard
        // set this to false to have relative timing retard
        bool m_retard = false;
        bool m_limiter = false;

        uint32_t m_ignitionCount;
        
    protected:
        SparkPlug *getPlug(int i);

        Function *m_timingCurve;
        SparkPlug *m_plugs;
        Crankshaft *m_crankshaft;
        int m_cylinderCount;
};

#endif /* ATG_ENGINE_SIM_IGNITION_MODULE_H */

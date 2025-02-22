#include "../include/ignition_module.h"

#include "../include/utilities.h"
#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>

IgnitionModule::IgnitionModule() {
    m_plugs = nullptr;
    m_crankshaft = nullptr;
    m_timingCurve = nullptr;
    m_cylinderCount = 0;
    m_lastCrankshaftAngle = 0.0;
    m_enabled = false;
    m_revLimitTimer = 0.0;
    m_revLimit = 0;
    m_limiterDuration = 0;
}

IgnitionModule::~IgnitionModule() {
    assert(m_plugs == nullptr);
}

void IgnitionModule::destroy() {
    delete[] m_plugs;

    m_plugs = nullptr;
    m_cylinderCount = 0;
}

void IgnitionModule::initialize(const Parameters &params) {
    m_cylinderCount = params.cylinderCount;
    m_plugs = new SparkPlug[m_cylinderCount];
    m_crankshaft = params.crankshaft;
    m_timingCurve = params.timingCurve;
    m_revLimit = params.revLimit;
    m_limiterDuration = params.limiterDuration;
}

void IgnitionModule::setFiringOrder(int cylinderIndex, double angle) {
    assert(cylinderIndex < m_cylinderCount);

    m_plugs[cylinderIndex].angle = angle;
    m_plugs[cylinderIndex].enabled = true;
}

void IgnitionModule::reset() {
    m_lastCrankshaftAngle = m_crankshaft->getCycleAngle();
    resetIgnitionEvents();
}

void IgnitionModule::update(double dt) {
    const double cycleAngle = m_crankshaft->getCycleAngle();

    if (m_enabled && m_revLimitTimer <= 0) {
        const double fourPi = 4 * constants::pi;
        const double advance = getTimingAdvance();

        for (int i = 0; i < m_cylinderCount; ++i) {
            double adjustedAngle = positiveMod(m_plugs[i].angle - advance, fourPi);
            const double r0 = m_lastCrankshaftAngle;
            double r1 = cycleAngle;

            if (m_crankshaft->m_body.v_theta < 0) {
                if (r1 < r0) {
                    r1 += fourPi;
                    adjustedAngle += fourPi;
                }

                if (adjustedAngle >= r0 && adjustedAngle < r1) {
                    m_plugs[i].ignitionEvent = m_plugs[i].enabled;
                    m_ignitionCount++;
                }
            }
            else {
                if (r1 > r0) {
                    r1 -= fourPi;
                    adjustedAngle -= fourPi;
                }

                if (adjustedAngle >= r1 && adjustedAngle < r0) {
                    m_plugs[i].ignitionEvent = m_plugs[i].enabled;
                    m_ignitionCount++;
                }
            }
        }
    }


    m_revLimitTimer -= dt;
    /*m_revLimitTimer -= dt;
    if (std::fabs(m_crankshaft->m_body.v_theta) > m_revLimit) {
        m_revLimitTimer = m_limiterDuration;
    }
    */

    if (m_revLimitTimer < 0) {
        m_revLimitTimer = 0;
    }

    if (std::fabs(m_crankshaft->m_body.v_theta) > units::rpm(m_2stepSoftCutLimit) && m_2stepEnabled) {
        m_retard = true;
        m_retardAmount = m_2stepSoftCutAngle;
        //m_revLimitTimer = 0.05;
        m_launchingSoft = true;
    }
    else if (m_revLimitTimer <= 0) {
        if (!m_limiter) {
            m_retard = false;
            m_retardAmount = 0;
        }
        m_launchingSoft = false;
        m_launchingHard = false;
    }

    if (std::fabs(m_crankshaft->m_body.v_theta) > units::rpm(m_2stepHardCutLimit) && m_2stepEnabled) {
        m_revLimitTimer = 0.1;
        m_launchingHard = true;
    }

    if (std::fabs(m_crankshaft->m_body.v_theta) > units::rpm(m_3stepSoftCutLimit) && m_3stepEnabled) {
        m_retard = true;
        m_retardAmount = m_3stepSoftCutAngle;
        m_revLimitTimer = 0.05;
        m_shiftingHard = true;
    }
    else if (m_revLimitTimer <= 0) {
        if (!m_limiter) {
            m_retard = false;
            m_retardAmount = 0;
        }
        m_shiftingHard = false;
    }

    m_lastCrankshaftAngle = cycleAngle;
}

bool IgnitionModule::getIgnitionEvent(int index) const {
    return m_plugs[index].ignitionEvent;
}

void IgnitionModule::resetIgnitionEvents() {
    for (int i = 0; i < m_cylinderCount; ++i) {
        m_plugs[i].ignitionEvent = false;
    }
}

double IgnitionModule::getTimingAdvance() {
    float mult = 1;
    if (m_ccw)
        mult = -1;

    if (m_retard) {
        return (m_retardAmount * units::deg) * mult;
    }
    else {
        return ((m_currentTableValue * units::deg) - (m_retardAmount * units::deg)) * mult;
    }
}

IgnitionModule::SparkPlug *IgnitionModule::getPlug(int i) {
    return &m_plugs[((i % m_cylinderCount) + m_cylinderCount) % m_cylinderCount];
}


#ifndef ATG_ENGINE_SIM_TURBOCHARGER_H
#define ATG_ENGINE_SIM_TURBOCHARGER_H

#include "engine.h"

class TurboCharger {
public:
	TurboCharger();
	~TurboCharger();

	void process(float dt);

	// In mBar.
	double m_boost = 0.0;
	double m_flow = 0.0;

	// In mBar.
	double m_wastegate = 700;

	inline void SetEngine(Engine* engine) { m_engine = engine; };
	inline double GetRotorRPM() { return m_rotorRPM; };

private:
	double m_rotorRPM = 0.0;

	// ?
	double m_flowMultiplier = 100.0;
	double m_rpmDown = 50;
	// Engine RPM
	double m_erpmToFlow = 100.0;
	// Turbo RPM
	double m_rpmToPressure = 0.01;
	double m_rpmToFlow = 0.1;

	/*
	turbo_flow_mult = 10000
	turbo_down = 0.05
	
	turbo_to_press = 5
	turbo_press_to_flow = 5000
	turbo_rpm_to_flow = 10000
	*/

	Engine* m_engine;
};

#endif /* ATG_ENGINE_SIM_TURBOCHARGER_H */

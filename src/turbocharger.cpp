
#include "../include/turbocharger.h"

TurboCharger::TurboCharger() {
	m_engine = nullptr;
}

TurboCharger::~TurboCharger() {

}

void TurboCharger::process(float dt) {
	/*
	
	flow = es.engine.ExhaustFlow * turbo_flow_mult

	flow = flow * (es.engine.RPM / turbo_rpm_to_flow)

	if INPUT_L == "true" then
		flow = flow * turbo_antilag_flow
	end

	turbo_spool = turbo_spool + flow

	if turbo_spool < turbo_max then
		turbo_spool = turbo_max
	else
		turbo_spool = turbo_spool + turbo_down
	end

	if turbo_spool > 0.5 then
		turbo_spool = 0.5
	end

	press = -turbo_spool / turbo_to_press
	flow = press * turbo_press_to_flow
	
	*/

	float flow = 0;

	for (int i = 0; i < m_engine->getExhaustSystemCount(); i++) {
		ExhaustSystem* exhaust = m_engine->getExhaustSystem(i);
		
		float floww = exhaust->getFlow();
		if (floww < 0)
			floww = -floww;

		flow += floww;
	}

	flow *= m_flowMultiplier;
	flow *= (units::toRpm(m_engine->getRpm()) / m_erpmToFlow);
	m_rotorRPM += flow;

	if (m_rotorRPM < 0)
		m_rotorRPM = 0;

	// calculate boost and flow
	m_boost = m_rotorRPM * m_rpmToPressure;
	m_flow = m_rotorRPM * m_rpmToFlow;

	if (m_boost >= m_wastegate) {
		m_rotorRPM *= 0.95f;
		m_boost = m_rotorRPM * m_rpmToPressure;
		m_flow = m_rotorRPM * m_rpmToFlow;
	}
	else {
		m_rotorRPM -= m_rpmDown;
		m_boost = m_rotorRPM * m_rpmToPressure;
		m_flow = m_rotorRPM * m_rpmToFlow;
	}

	for (int i = 0; i < m_engine->getIntakeCount(); i++) {
		Intake* intake = m_engine->getIntake(i);
		//intake->m_boost = m_boost * (1 - m_engine->getThrottle());
		//intake->m_additionalFlow = m_flow * (1 - m_engine->getThrottle());
	}
}

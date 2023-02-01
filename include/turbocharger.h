#pragma once

#include "engine.h"
#include "transmission.h"
#include "combustion_chamber.h"
#include "vehicle.h"
#include "synthesizer.h"
#include "dynamometer.h"
#include "starter_motor.h"
#include "derivative_filter.h"
#include "vehicle_drag_constraint.h"

//#include "engine_sim_application.h"

#include "scs.h"

class EngineSimApplication;
class TurboCharger {
public:
	TurboCharger();
	
	EngineSimApplication* app;
	void LoadStuff(EngineSimApplication* sim);
	void frame();

	//void LoadShit(EngineSimApplication* sim);
	//EngineSimApplication* sim;

	bool IS_ENABLED = true;

	double spool;
	double spoolSpin;

	// constants
	const double spoolBoostMult = 0.00001;
	//const double spoolBoostMult = 0.0;
	const double spoolMult = 100;
	const double frictionSub = 0.001;
	double wastegateTrigger = 5;
	const double pressMult = 20;
	const double outputPressDiv = 5;

	const double antilagBoost = 0.1;
	
	double throttle = 0.0;
	bool mthr = false;

	bool play = false;

	// default config
	/*

	
	turbo_wastegate_trigger=5
	turbo_min_flow=0.000015
	turbo_antilag_boost=0.1
	turbo_friction=0.001
	
	const double MIN_WASTEGATE = 5;
	const double MIN_FLOW = 0.000015;
	const double MIN_ANTILAG_BOOST = 0.1;
	const double MIN_FRICTION = 0.001;

	// current config

	double CURRENT_WASTEGATE = MIN_WASTEGATE;
	double CURRENT_FLOW = MIN_FLOW;
	double CURRENT_ANTILAG_BOOST = MIN_ANTILAG_BOOST;
	double CURRENT_FRICTION = MIN_FRICTION;
	*/

	// DEBUG
	double lastExh = 0;
	double lastExhaust = 0;

	// returns how much more cfms to add to output bruh
	double AddMoPowahBaby();

	// returns how much more pressure to add to output bruh
	double AddPress();

	// tries to spool turbo bruh
	void AddWhoosh(double exhaust);

	// tries to say current turbo stats
	void Log();
};

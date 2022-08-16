#pragma once

#include "../include/engine.h"
#include "../include/transmission.h"
#include "../include/combustion_chamber.h"
#include "../include/vehicle.h"
#include "../include/synthesizer.h"
#include "../include/dynamometer.h"
#include "../include/starter_motor.h"
#include "../include/derivative_filter.h"
#include "../include/vehicle_drag_constraint.h"

#include "../include/scs.h"

class ProCharger {
public:
	ProCharger();

	const bool IS_ENABLED = true;

	const double maxSpool = 5;
	double spool;
	const double spoolBoostMult = 0.00001;
	//const double spoolBoostMult = 0.0;
	const double spoolMult = 0.1;
	const double frictionSub = 4;
	const double outputPressDiv = 5;

	// returns how much more pressure to add to output bruh
	double AddMoPowahBaby();

	// returns how much more pressure to add to output bruh
	double AddPress();

	// tries to spool turbo bruh
	void AddWhoosh(double rpm);

	// tries to get current turbo boost
	double CurrentWhoosh();
};

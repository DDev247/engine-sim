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

class SuperCharger {
public:
	SuperCharger();

	bool IS_ENABLED = true;

	double spool;
	double spoolSpin;

	// constants
	double ratio = 1;
	double maxSpool = 5;
	double spoolBoostMult = 0.00001;
	double spoolMult = 0.1;
	double frictionSub = 4;
	double outputPressDiv = 5;

	// returns how much more pressure to add to output bruh
	double AddMoPowahBaby();

	// returns how much more pressure to add to output bruh
	double AddPress();

	// tries to spool procharger bruh
	void AddWhoosh(double rpm);

	// tries to get current turbo boost
	double CurrentWhoosh();
};

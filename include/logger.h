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

#include "scs.h"

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

#include <windows.h>

class Logger {
public:
	Logger();

public:
	

	static void Debug(std::string message);
	static void DebugLine(std::string message);

};

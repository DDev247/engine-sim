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

#include "engine_sim_application.h"
#include "simulator.h"

#include "scs.h"

#include "logger.h"
#include <fstream>

class ConfigLoader {
public:
	ConfigLoader();

public:

	EngineSimApplication* app;

	std::string inputFile;

	void Init(EngineSimApplication* sim);

	void Load();

};

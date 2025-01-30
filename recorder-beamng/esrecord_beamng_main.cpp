
#include <iostream>
#include <fstream>
#include <chrono>
#include <map>

#include "../recorder/esrecord_config.h"

int main()
{
	enableAnsi();
	std::cout << "ES-Record v" ES_RECORD_VERSION ", BeamNG.drive converter" << std::endl;
	std::cout << "Reading configuration..." << std::endl;
	
	Config config;
	bool success = loadConfig(config);

	if (!success) {
		std::cout << "Failed to load configuration" << std::endl;
		return 1;
	}

	std::ifstream dynoCurve("dyno.csv");
	if (!dynoCurve.is_open()) {
		std::cout << "Failed to open dyno.csv, continuing without dyno curve." << std::endl;
	}
	else {
		std::ofstream jbeamOut("torque.jbeam");
		if (!jbeamOut.is_open()) {
			std::cout << "Failed to open torque.jbeam";
			return 1;
		}

		float dynamicFriction, staticFriction;
		std::cout << "Enter dynamic friction: ";
		std::cin >> dynamicFriction;
		std::cout << "Enter static friction: ";
		std::cin >> staticFriction;

		jbeamOut << "\"torque\": [" << std::endl;
		jbeamOut << "    [\"rpm\", \"torque\"]," << std::endl;

		std::string line;
		int i = 0;
		while (std::getline(dynoCurve, line)) {
			if (i == 0) { i++; continue; } // ignore header
			
			int rpm, throttle;
			float power, torque;
			
			int pos1 = line.find(",");
			rpm = std::stoi(line.substr(0, pos1));
			
			int pos2 = line.find(",", pos1+1);
			throttle = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));

			pos1 = line.find(",", pos2+1);
			power = std::stof(line.substr(pos2 + 1, pos1 - pos2 - 1));

			pos2 = line.find(",", pos1+1);
			torque = std::stof(line.substr(pos1 + 1, pos2 - pos1 - 1));

			torque = torque - staticFriction - ((rpm / 9.55) * dynamicFriction); // apply friction correction

			if (throttle == 100) {
				jbeamOut << "    [" << rpm << ", " << torque << "]," << std::endl;
			}

			i++;
		}

		jbeamOut << "]," << std::endl;

		jbeamOut.flush();
		jbeamOut.close();
	}

	//dynoCurve << "rpm,throttle,power_hp,torque_nm" << std::endl;

	bool has0 = false, has100 = false;
	for (int i = 0; i < config.sampleThrottleValuesCount; i++) {
		if (config.sampleThrottleValues[i] == 0) has0 = true;
		if (config.sampleThrottleValues[i] == 100) has100 = true;
	}

	if (!has0 || !has100) {
		std::cout << "Your throttle values don't contain 0 and 100." << std::endl;
		return 1;
	}

	std::string blendName = "";
	std::cout << "Enter desired blend name: ";
	std::cin >> blendName;

	std::ofstream blendOut(blendName + ".sfxBlend2D.json");
	if (!blendOut.is_open()) {
		std::cout << "Failed to open blend file" << std::endl;
		return 1;
	}

	blendOut << "{" << std::endl;
	blendOut << "    \"header\": {" << std::endl;
	blendOut << "        \"version\": 1" << std::endl;
	blendOut << "    }," << std::endl;
	blendOut << "    \"eventName\": \"event:>Engine>default\"," << std::endl;
	blendOut << "    \"samples\": [" << std::endl;
	blendOut << "        [" << std::endl; // beginning of throttle 0 array

	for (int i = 0; i < config.sampleCount; i++) {
		int rpm = config.sampleRPMValues[i];

		std::string path0 = "output-" + std::to_string(rpm) + "-0.wav";
		//std::string path100 = "output-" + std::to_string(rpm) + "-100.wav";

		blendOut << "            [\"art/sound/engine/" << blendName << "/" + path0 + "\", " << rpm << "]";
		if (i != config.sampleCount - 1) blendOut << "," << std::endl;
		else blendOut << std::endl;
	}

	blendOut << "        ]," << std::endl; // end of throttle 0 array
	blendOut << "        [" << std::endl; // beginning of throttle 100 array

	for (int i = 0; i < config.sampleCount; i++) {
		int rpm = config.sampleRPMValues[i];

		//std::string path0 = "output-" + std::to_string(rpm) + "-0.wav";
		std::string path100 = "output-" + std::to_string(rpm) + "-100.wav";

		blendOut << "            [\"art/sound/engine/" << blendName << "/" + path100 + "\", " << rpm << "]";
		if (i != config.sampleCount - 1) blendOut << "," << std::endl;
		else blendOut << std::endl;
	}
	blendOut << "        ]" << std::endl; // end of throttle 100 array
	blendOut << "    ]" << std::endl;
	blendOut << "}" << std::endl;

	blendOut.flush();
	blendOut.close();

	dynoCurve.close();

	std::cout << "Done" << std::endl;
	std::cout << "Copy your samples into \"art/sound/engine/" << blendName << "/\"" << std::endl;

	return 0;
}

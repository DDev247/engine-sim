
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

#include "esrecord.h"

Simulator* ESRecord_Simulator = nullptr;
Engine* ESRecord_Engine = nullptr;
Transmission* ESRecord_Transmission = nullptr;
Vehicle* ESRecord_Vehicle = nullptr;

void loadES() {
	std::cout << "Compiling Simulation (using ../assets/main.mr)..." << std::endl;

	initialise("../assets/main.mr");

	std::cout << "Using engine: \"" << ESRecord_Engine->getName() << "\"" << std::endl;
}

void recordSample(SampleConfig config, float &power, float &torque) {
	std::string headerString = "Recording sample: RPM=" + std::to_string(config.rpm) + ", Throttle=" + std::to_string(config.throttle) + ", Frequency=" + std::to_string(config.frequency) + "...";
	std::cout << headerString << std::endl;
	float throttleValue = (config.throttle / 100.0f);

	std::ofstream outputFile(config.output, std::ios::out | std::ios::binary);

	if (!outputFile.is_open()) {
		std::cout << "Failed to open output" << std::endl;
		return;
	}

	// Wave stuff
	int32_t fileSize = 0;
	int32_t fileSizeData = 0;

	// Wave header (RIFF + fmt + data)
	{
		// header chunk
		outputFile << "RIFF"; fileSize += 4;
		outputFile.write((const char*)(&fileSize), sizeof(int32_t)); fileSize += sizeof(int32_t);
		outputFile << "WAVE"; fileSize += 4;

		// format chunk
		int32_t formatChunkSize = 16; // pcm
		int16_t audioFormat = 0x0001; // pcm
		int16_t numChannels = 1; // mono
		int32_t sampleRate = 44100; // mono

		outputFile.write("fmt ", 4); fileSize += 4;
		outputFile.write((const char*)(&formatChunkSize), sizeof(int32_t)); fileSize += sizeof(int32_t);
		outputFile.write((const char*)(&audioFormat), sizeof(int16_t)); fileSize += sizeof(int16_t);
		outputFile.write((const char*)(&numChannels), sizeof(int16_t)); fileSize += sizeof(int16_t);
		outputFile.write((const char*)(&sampleRate), sizeof(int32_t)); fileSize += sizeof(int32_t);

		int16_t bitDepth = 16;
		int32_t numBytesPerSecond = (int32_t)((numChannels * sampleRate * bitDepth) / 8);
		outputFile.write((const char*)(&numBytesPerSecond), sizeof(int32_t)); fileSize += sizeof(int32_t);

		int16_t numBytesPerBlock = numChannels * (bitDepth / 8);
		outputFile.write((const char*)(&numBytesPerBlock), sizeof(int16_t)); fileSize += sizeof(int16_t);

		outputFile.write((const char*)(&bitDepth), sizeof(int16_t)); fileSize += sizeof(int16_t);

		outputFile << "data"; fileSize += 4;
		outputFile.write((const char*)(&fileSizeData), sizeof(int32_t)); fileSize += sizeof(int32_t);
	}

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	
	// initialise shit
	initialise(); // recreate simulation again

	ESRecord_Simulator->setSimulationFrequency(config.frequency);
	ESRecord_Simulator->m_dyno.m_enabled = false; // afr fix
	ESRecord_Simulator->m_dyno.m_hold = true;
	ESRecord_Simulator->m_dyno.m_rotationSpeed = units::rpm(config.rpm);

	ESRecord_Engine->setSpeedControl(1.0); // start engine
	ESRecord_Simulator->m_starterMotor.m_enabled = true;
	ESRecord_Simulator->getEngine()->getIgnitionModule()->m_enabled = true;

	if (config.overrideRevlimit)
		ESRecord_Engine->getIgnitionModule()->setRevlimit(units::rpm(config.rpm + 1000));

	int chunksRecorded = 0;
	const int chunkTarget = 44100 * config.length;
	
	const unsigned int bufferSize = 2048;
	int16_t *buffer = new int16_t[bufferSize];

	// cycle the simulation for a bit
	std::string s = "Cycling the simulator...";
	std::cout << s;
	for (int i = 0; i < config.prerunCount; i++) {
		update(60.0f);

		// starter might skew the results
		if(i > config.prerunCount / 2) {
			ESRecord_Simulator->m_dyno.m_enabled = true;
			ESRecord_Simulator->m_starterMotor.m_enabled = false;
			ESRecord_Engine->setSpeedControl(throttleValue);

			constexpr double RC = 0.1;
			const double dt = (1.0 / 60.0); // 0.0166
			const double alpha = dt / (dt + RC);

			const double current_torque = units::convert(ESRecord_Simulator->getFilteredDynoTorque(), units::Nm);
			const double current_power = units::convert(ESRecord_Simulator->getDynoPower(), units::hp);

			torque = (1 - alpha) * torque + alpha * current_torque;
			power = (1 - alpha) * power + alpha * current_power;
			//std::cout << power << "hp " << torque << "Nm";
		}

		int chunksThisFrame = ESRecord_Simulator->readAudioOutput(bufferSize, buffer);
		std::cout << "\r\033[" << s.length() << "C"; // goto end
		std::cout << " " << i+1 << "/" << config.prerunCount << " (" << chunksThisFrame << ")";
	}
	std::cout << "\33[2K\r";

	// record

	int simcalls = 0;
	while (chunksRecorded < chunkTarget) {
		update(60.0f); simcalls++;

		{
			constexpr double RC = 0.1;
			const double dt = (1.0 / 60.0); // 0.0166
			const double alpha = dt / (dt + RC);

			const double current_torque = units::convert(ESRecord_Simulator->getFilteredDynoTorque(), units::Nm);
			const double current_power = units::convert(ESRecord_Simulator->getDynoPower(), units::hp);

			torque = (1 - alpha) * torque + alpha * current_torque;
			power = (1 - alpha) * power + alpha * current_power;
			//std::cout << power << "hp " << torque << "Nm (" << ESRecord_Engine->getRpm() << "rpm)";
		}

		int chunksThisFrame = ESRecord_Simulator->readAudioOutput(bufferSize, buffer);

		unsigned int length = chunksThisFrame;
		if (chunksRecorded + chunksThisFrame > chunkTarget) {
			length = chunkTarget - chunksRecorded;
		}

		outputFile.write((const char*)buffer, length * (sizeof int16_t));
		fileSize += length * (sizeof int16_t);
		fileSizeData += length * (sizeof int16_t);

		chunksRecorded += length;
		//std::cout << (chunksRecorded / chunkTarget) << "%";
		std::cout << chunksRecorded << "/" << chunkTarget << " (" << chunksThisFrame << ")";
		std::cout << "\r";
	}
	// \33[2K clear line
	// \033[A move up

	// Adjust wave header
	outputFile.seekp(4, std::ios::beg);
	outputFile.write((const char*)(&fileSize), sizeof(int32_t));
	outputFile.seekp(40, std::ios::beg);
	outputFile.write((const char*)(&fileSizeData), sizeof(int32_t));

	outputFile.flush();
	outputFile.close();

	delete[] buffer;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	
	std::cout << "\33[2K\033[A\r"; // cursor is now at the header
	std::cout << "\033[" << headerString.length() << "C"; // goto end

	long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
	float ratio = (float)config.length / ((float)millis / 1000.0f);
	std::cout << std::setprecision(3);
	std::cout << " OK (" << millis << "ms, " << ratio << "x, " << simcalls << " simcalls)" << std::endl;
}

int main()
{
	enableAnsi();
	std::cout << "ES-Record v" ES_RECORD_VERSION << std::endl;
	std::cout << "Reading configuration..." << std::endl;

	Config config;
	bool success = loadConfig(config);

	if (!success) {
		std::cout << "Failed to load configuration" << std::endl;
		return 1;
	}

	loadES();

	std::ofstream dynoCurve("dyno.csv");
	if (!dynoCurve.is_open()) {
		std::cout << "Failed to open dyno.csv, continuing without dyno curve." << std::endl;
	}

	if (dynoCurve.is_open()) {
		dynoCurve << "rpm,throttle,power_hp,torque_nm" << std::endl;
		dynoCurve << std::setprecision(3);
	}

	for (int i = 0; i < config.sampleCount; i++) {
		for (int j = 0; j < config.sampleThrottleValuesCount; j++) {
			SampleConfig sample;
			sample.prerunCount = config.prerunCount;
			sample.rpm = config.sampleRPMValues[i];
			sample.throttle = config.sampleThrottleValues[j];
			sample.frequency = config.sampleRPMFrequencyValues[i];
			sample.length = config.sampleLength;
			sample.overrideRevlimit = config.overrideRevlimit;

			sample.output = "../samples/output-" + std::to_string(sample.rpm) + "-" + std::to_string(sample.throttle) + ".wav";

			float power = 0;
			float torque = 0;
			recordSample(sample, power, torque);

			if (dynoCurve.is_open())
				dynoCurve << sample.rpm << "," << sample.throttle << "," << power << "," << torque << std::endl;
		}
	}

	dynoCurve.flush();
	dynoCurve.close();

	delete[] config.sampleRPMValues;
	delete[] config.sampleRPMFrequencyValues;
	delete[] config.sampleThrottleValues;

	std::cout << "Done" << std::endl;

	return 0;
}

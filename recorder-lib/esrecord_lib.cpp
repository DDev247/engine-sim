
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

#include "esrecord_lib.h"

#define MAX_INSTANCES 8

Simulator* ESRecord_Simulator		[MAX_INSTANCES];
Engine* ESRecord_Engine				[MAX_INSTANCES];
Transmission* ESRecord_Transmission	[MAX_INSTANCES];
Vehicle* ESRecord_Vehicle			[MAX_INSTANCES];

ESRecordState ESRecord_CurrentState[MAX_INSTANCES] = { ESRECORD_STATE_IDLE };
int ESRecord_Progress[MAX_INSTANCES] = { 0 };

ESRECORD_API SampleResult ESRecord_Record(int instanceId, SampleConfig config) {
	ESRecord_CurrentState[instanceId] = ESRECORD_STATE_PREPARING;
	float throttleValue = (config.throttle / 100.0f);
	SampleResult result;
	result.power = 0;
	result.torque = 0;

	std::ofstream outputFile(config.output, std::ios::out | std::ios::binary);

	if (!outputFile.is_open()) {
		result.success = false;
		return result;
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
	ESRecord_Initialise(instanceId); // recreate simulation again

	ESRecord_Simulator[instanceId]->setSimulationFrequency(config.frequency);
	ESRecord_Simulator[instanceId]->m_dyno.m_enabled = false; // afr fix
	ESRecord_Simulator[instanceId]->m_dyno.m_hold = true;
	ESRecord_Simulator[instanceId]->m_dyno.m_rotationSpeed = units::rpm(config.rpm);

	ESRecord_Engine[instanceId]->setSpeedControl(1.0); // start engine
	ESRecord_Simulator[instanceId]->m_starterMotor.m_enabled = true;
	ESRecord_Simulator[instanceId]->getEngine()->getIgnitionModule()->m_enabled = true;

	if (config.overrideRevlimit)
		ESRecord_Engine[instanceId]->getIgnitionModule()->setRevlimit(units::rpm(config.rpm + 1000));

	int chunksRecorded = 0;
	const int chunkTarget = 44100 * config.length;
	
	const unsigned int bufferSize = 2048;
	int16_t *buffer = new int16_t[bufferSize];

	// cycle the simulation for a bit
	ESRecord_CurrentState[instanceId] = ESRECORD_STATE_WARMUP;
	for (int i = 0; i < config.prerunCount; i++) {
		ESRecord_Update(instanceId, 60.0f);

		// starter might skew the results
		if(i > config.prerunCount / 2) {
			ESRecord_Simulator[instanceId]->m_dyno.m_enabled = true;
			ESRecord_Simulator[instanceId]->m_starterMotor.m_enabled = false;
			ESRecord_Engine[instanceId]->setSpeedControl(throttleValue);
		}

		if (i > config.prerunCount / 2 + config.prerunCount / 4) {
			constexpr double RC = 0.1;
			const double dt = (1.0 / 60.0); // 0.0166
			const double alpha = dt / (dt + RC);

			const double current_torque = units::convert(ESRecord_Simulator[instanceId]->getFilteredDynoTorque(), units::Nm);
			const double current_power = units::convert(ESRecord_Simulator[instanceId]->getDynoPower(), units::hp);

			result.torque = (1 - alpha) * result.torque + alpha * current_torque;
			result.power = (1 - alpha) * result.power + alpha * current_power;
		}

		int chunksThisFrame = ESRecord_Simulator[instanceId]->readAudioOutput(bufferSize, buffer);
		ESRecord_Progress[instanceId] = (int)(((float)i / (float)config.prerunCount) * 100.0f);
	}
	//std::cout << "\33[2K\r";

	// record

	ESRecord_Progress[instanceId] = 0;
	ESRecord_CurrentState[instanceId] = ESRECORD_STATE_RECORDING;
	int simcalls = 0;
	while (chunksRecorded < chunkTarget) {
		ESRecord_Update(instanceId, 60.0f); simcalls++;

		{
			constexpr double RC = 0.1;
			const double dt = (1.0 / 60.0); // 0.0166
			const double alpha = dt / (dt + RC);

			const double current_torque = units::convert(ESRecord_Simulator[instanceId]->getFilteredDynoTorque(), units::Nm);
			const double current_power = units::convert(ESRecord_Simulator[instanceId]->getDynoPower(), units::hp);

			result.torque = (1 - alpha) * result.torque + alpha * current_torque;
			result.power = (1 - alpha) * result.power + alpha * current_power;
		}

		int chunksThisFrame = ESRecord_Simulator[instanceId]->readAudioOutput(bufferSize, buffer);

		unsigned int length = chunksThisFrame;
		if (chunksRecorded + chunksThisFrame > chunkTarget) {
			length = chunkTarget - chunksRecorded;
		}

		outputFile.write((const char*)buffer, length * (sizeof int16_t));
		fileSize += length * (sizeof int16_t);
		fileSizeData += length * (sizeof int16_t);

		chunksRecorded += length;
		ESRecord_Progress[instanceId] = (int)(((float)chunksRecorded / (float)chunkTarget) * 100.0f);
	}
	
	OutputDebugStringW(L"Recording done\n");

	// write INFO chunk
	{
		outputFile << "LIST"; fileSize += 4;

		int32_t pos = outputFile.tellp();
		int32_t infoSize = 0;
		static std::string authorName = "ES-Record " ES_RECORD_VERSION ", engine-sim 0.1.11a";
		std::string trackName = std::to_string(config.rpm) + "RPM / " + std::to_string(config.throttle) + "%";

		outputFile.write((const char*)(&infoSize), sizeof(int32_t)); fileSize += sizeof(int32_t);
		outputFile << "INFO"; fileSize += 4; infoSize += 4;
		
		// track name
		outputFile << "INAM"; fileSize += 4; infoSize += 4; // chunk header
		int32_t len = trackName.size() + 1; // chunk size
		outputFile.write((const char*)(&len), sizeof(int32_t)); fileSize += sizeof(int32_t); infoSize += sizeof(int32_t);
		outputFile.write(trackName.c_str(), len); fileSize += len; infoSize += len;

		// author
		outputFile << "IART"; fileSize += 4; infoSize += 4; // chunk header
		len = authorName.size() + 1; // chunk size
		outputFile.write((const char*)(&len), sizeof(int32_t)); fileSize += sizeof(int32_t); infoSize += sizeof(int32_t);
		outputFile.write(authorName.c_str(), len); fileSize += len; infoSize += len;

		// album
		outputFile << "IPRD"; fileSize += 4; infoSize += 4; // chunk header
		len = ESRecord_Engine[instanceId]->getName().size() + 1; // chunk size
		outputFile.write((const char*)(&len), sizeof(int32_t)); fileSize += sizeof(int32_t); infoSize += sizeof(int32_t);
		outputFile.write(ESRecord_Engine[instanceId]->getName().c_str(), len); fileSize += len; infoSize += len;

		// write chunk size
		outputFile.seekp(pos, std::ios::beg);
		outputFile.write((const char*)(&infoSize), sizeof(int32_t));
	}

	// Adjust wave header
	outputFile.seekp(4, std::ios::beg);
	outputFile.write((const char*)(&fileSize), sizeof(int32_t));
	outputFile.seekp(40, std::ios::beg);
	outputFile.write((const char*)(&fileSizeData), sizeof(int32_t));

	outputFile.flush();
	outputFile.close();

	delete[] buffer;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	
	ESRecord_CurrentState[instanceId] = ESRECORD_STATE_IDLE;
	// todo?
	long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
	float ratio = (float)config.length / ((float)millis / 1000.0f);
	result.millis = millis;
	result.ratio = ratio;

	result.success = true;
	return result;
}

ESRECORD_API bool ESRecord_GetSimState(int instanceId) {
	if (ESRecord_Simulator[instanceId] != nullptr &&
		ESRecord_Engine[instanceId] != nullptr &&
		ESRecord_Transmission[instanceId] != nullptr &&
		ESRecord_Vehicle[instanceId] != nullptr) {
		return true;
	}
	return false;
}

ESRECORD_API ESRecordState ESRecord_GetState(int instanceId, int& progress) {
	progress = ESRecord_Progress[instanceId];
	return ESRecord_CurrentState[instanceId];
}

ESRECORD_API int ESRecord_GetVersion() {
	return ES_RECORD_LIBRARY_VERSION;
}

ESRECORD_API char* ESRecord_Engine_GetName(int instanceId) {
	if (ESRecord_Engine != nullptr) {
		char* cstr = new char[ESRecord_Engine[instanceId]->getName().length() + 1];
		strcpy(cstr, ESRecord_Engine[instanceId]->getName().c_str());
		return cstr;
	}

	return "";
}

ESRECORD_API float ESRecord_Engine_GetRedline(int instanceId) {
	if (ESRecord_Engine != nullptr)
		return units::toRpm(ESRecord_Engine[instanceId]->getRedline());
	return -1.0f;
}

ESRECORD_API float ESRecord_Engine_GetDisplacement(int instanceId) {
	if (ESRecord_Engine != nullptr)
		return (float)units::convert(ESRecord_Engine[instanceId]->getDisplacement(), units::L);
	return -1.0f;
}

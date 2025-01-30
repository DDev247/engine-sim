
#include <iostream>
#include <string>
#include <algorithm>

#include "esrecord_config.h"

#include <windows.h>

// https://www.reddit.com/r/C_Programming/comments/ktnpbv/comment/gio6m2a
void enableAnsi()
{
#ifdef _WIN32
	/* Best effort enable ANSI escape processing. */
	//void* GetStdHandle(unsigned);
	//int GetConsoleMode(void*, unsigned*);
	//int SetConsoleMode(void*, unsigned);
	void* handle;
	unsigned long mode;
	handle = GetStdHandle(-11); /* STD_OUTPUT_HANDLE */
	if (GetConsoleMode(handle, &mode)) {
		mode |= 0x0004; /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
		SetConsoleMode(handle, mode); /* ignore errors */
	}
#endif
}

bool loadConfig(Config& out)
{
	//config.sampleCount = 0;

	std::ifstream file("../config.txt");
	if (!file.is_open()) {
		std::cout << "Failed to open config file" << std::endl;
		return false;
	}

	std::string line;
	int i = 0;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') // comment
			continue;

		int ptr, pos, j;
		switch (i) {
		case 0:
			out.prerunCount = std::stoi(line);
			break;
		case 1:
			out.sampleLength = std::stoi(line);
			break;
		case 2:
			out.sampleCount = std::stoi(line);
			break;
		case 3:
			out.sampleRPMValues = new int[out.sampleCount];
			ptr = 0;
			pos = line.find(",");
			j = 0;
			while (pos != std::string::npos) {
				out.sampleRPMValues[j] = std::stoi(line.substr(ptr, pos));

				ptr = pos + 1;
				pos = line.find(",", ptr);
				j++;
			}

			break;
		case 4:
			out.sampleRPMFrequencyValues = new int[out.sampleCount];
			ptr = 0;
			pos = line.find(",");
			j = 0;
			while (pos != std::string::npos) {
				out.sampleRPMFrequencyValues[j] = std::stoi(line.substr(ptr, pos));

				ptr = pos + 1;
				pos = line.find(",", ptr);
				j++;
			}

			break;
		case 5:
			out.sampleThrottleValuesCount = std::stoi(line);
			break;
		case 6:
			out.sampleThrottleValues = new int[out.sampleThrottleValuesCount];
			ptr = 0;
			pos = line.find(",");
			j = 0;
			while (pos != std::string::npos) {
				out.sampleThrottleValues[j] = std::stoi(line.substr(ptr, pos));

				ptr = pos + 1;
				pos = line.find(",", ptr);
				j++;
			}
			break;
		case 7:
			std::transform(line.begin(), line.end(), line.begin(),
				[](unsigned char c) { return std::tolower(c); });

			out.overrideRevlimit = (line == "true");
		}

		i++;
	}
	file.close();

	std::cout << "Config {" << std::endl;
	std::cout << "    sampleLength: " << out.sampleLength << std::endl;
	std::cout << "    sampleCount: " << out.sampleCount << std::endl;
	std::cout << "    sampleRPMValues: ";
	for (int i = 0; i < out.sampleCount; i++) {
		std::cout << out.sampleRPMValues[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "    sampleRPMFrequencyValues: ";
	for (int i = 0; i < out.sampleCount; i++) {
		std::cout << out.sampleRPMFrequencyValues[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "    sampleThrottleValuesCount: " << out.sampleThrottleValuesCount << std::endl;
	std::cout << "    sampleThrottleValues: ";
	for (int i = 0; i < out.sampleThrottleValuesCount; i++) {
		std::cout << out.sampleThrottleValues[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "    overrideRevlimit: " << (out.overrideRevlimit ? "true" : "false") << std::endl;
	std::cout << "}" << std::endl;

	return true;
}

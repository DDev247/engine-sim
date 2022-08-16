
#include "..\include\config_loader.h"

ConfigLoader::ConfigLoader()
{

}

void ConfigLoader::Init(EngineSimApplication* sim)
{
	this->app = sim;
}

void ConfigLoader::Load()
{
	std::string line;
	std::ifstream fileStream;
	fileStream.open("./config.txt");
	Logger::DebugLine("----TURBO MOD LOADING----");

	if (fileStream.is_open())
	{
		while (std::getline(fileStream, line))
		{
			if(line._Starts_with("turbo_wastegate_trigger"))
			{
				std::string str = line.substr(25, line.length() - 1);
				double value = std::stod(str);
				Logger::DebugLine("wastegate trigger: " + str);
			}

			if (line._Starts_with("turbo_min_flow"))
			{
				std::string str = line.substr(16, line.length() - 1);
				double value = std::stod(str);
				Logger::DebugLine("min flow: " + str);
			}

			if (line._Starts_with("turbo_antilag_boost"))
			{
				std::string str = line.substr(21, line.length() - 1);
				double value = std::stod(str);
				Logger::DebugLine("antilage boost: " + str);
			}

			if (line._Starts_with("turbo_friction"))
			{
				std::string str = line.substr(16, line.length() - 1);
				double value = std::stod(str);
				Logger::DebugLine("friction: " + str);
			}
		}

		fileStream.close();
	}
	else
	{
		Logger::DebugLine("Unable to open the file");
	}
}


#include "../include/logger.h"

Logger::Logger()
{
	
}

void Logger::Debug(std::string message)
{
	char* cstr = new char[message.length() + 1];
	strcpy(cstr, message.c_str());
	OutputDebugStringA(cstr);
}

void Logger::DebugLine(std::string message)
{
	Debug(message + "\n");
}

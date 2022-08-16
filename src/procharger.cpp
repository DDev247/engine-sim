
#include "../include/procharger.h"

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

#include <windows.h>

//std::string to_string(double x);

ProCharger::ProCharger()
{
	this->spool = 0;
}
/*

double Turbo::AddMoPowahBaby()
{
	if (this->spool < 0)
		this->spool = 0;
	else if (this->spool > this->maxSpool)
		this->spool = this->maxSpool;

	double output = (this->spool * this->spoolBoostMult) - this->frictionSub;

	return output;
}

void Turbo::AddTurboWhoosh(double exhaustVelocity)
{
	double output = (this->spool + (exhaustVelocity * this->spoolMult)) - this->frictionSub;
	if (exhaustVelocity < 0)
		this->spool -= this->frictionSub;
	else
		this->spool += output;
}
*/

double ProCharger::AddMoPowahBaby()
{
	if (this->IS_ENABLED)
	{
		double output = this->spool * this->spoolBoostMult;

		return output;
	}
	else
		return 0;
}

double ProCharger::AddPress()
{
	if (this->IS_ENABLED)
	{
		double output = this->spool / this->outputPressDiv;

		return output;
	}
	else
	{
		return 0;
	}
}

void ProCharger::AddWhoosh(double rpm)
{
	if (this->IS_ENABLED)
	{
		double ratio = 1;
		spool = (rpm / 1000) * ratio;
	}
}

double ProCharger::CurrentWhoosh()
{
	std::string str = std::to_string(this->spool);
	char* cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	OutputDebugStringA("[ProCharger] Spool ");
	OutputDebugStringA(cstr);
	OutputDebugStringA("\n");

	str = std::to_string(this->AddMoPowahBaby());
	cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	OutputDebugStringA("[ProCharger] Boost ");
	OutputDebugStringA(cstr);
	OutputDebugStringA("\n");

	str = std::to_string(this->AddPress());
	cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	OutputDebugStringA("[ProCharger] Pressure ");
	OutputDebugStringA(cstr);
	OutputDebugStringA("\n");

	return this->spool;
}


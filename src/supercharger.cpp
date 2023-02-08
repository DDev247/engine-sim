
#include "../include/supercharger.h"
#include "../include/charger.h"

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

#include <windows.h>

//std::string to_string(double x);

SuperCharger::SuperCharger()
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

double SuperCharger::AddMoPowahBaby()
{
	if (this->IS_ENABLED)
	{
		double output = this->spool * this->spoolBoostMult;

		return output;
	}
	else
		return 0;
}

double SuperCharger::AddPress()
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

void SuperCharger::AddWhoosh(double rpm)
{
	if (this->IS_ENABLED)
	{
		spool = (rpm / 1000) * ratio;

		spoolSpin = spool;
	}
}

double SuperCharger::CurrentWhoosh()
{
	std::string str = std::to_string(this->spool);
	char* cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	OutputDebugStringA("[SuperCharger] Spool ");
	OutputDebugStringA(cstr);
	OutputDebugStringA("\n");

	str = std::to_string(this->AddMoPowahBaby());
	cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	OutputDebugStringA("[SuperCharger] Boost ");
	OutputDebugStringA(cstr);
	OutputDebugStringA("\n");

	str = std::to_string(this->AddPress());
	cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	OutputDebugStringA("[SuperCharger] Pressure ");
	OutputDebugStringA(cstr);
	OutputDebugStringA("\n");

	return this->spool;
}

Charger::Charger() {
	m_supercharger = nullptr;
}

Charger::~Charger() {
	/* void */
}

void Charger::initialize(const Parameters& params) {
	m_supercharger = params.supercharger;
}

void Charger::destroy() {
	/* void */
}

double Charger::relativeX() const {
	return m_body.p_x;
}

double Charger::relativeY() const {
	return m_body.p_y;
}

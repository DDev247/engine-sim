
#include "../include/turbocharger.h"
#include "../include/turbo.h"
#include "../include/logger.h"

//std::string to_string(double x);

TurboCharger::TurboCharger()
{
	this->spool = 0;
}

/*
dbasic::AudioAsset audio;
void TurboCharger::LoadShit(EngineSimApplication* sim)
{
	//ysError err = sim->getAssetManager()->LoadAudioFile("flutter.wav", "flutter");
	audio = sim->getAssetManager()->GetAudioAsset("flutter");
}
*/

double TurboCharger::AddMoPowahBaby()
{
	if (this->IS_ENABLED)
	{
		double output = this->spool * this->spoolBoostMult;

		return output;
	}
	else
	{
		return 0;
	}
}

double TurboCharger::AddPress()
{
	if (this->IS_ENABLED)
	{
		double output = this->spool / this->outputPressDiv;

		return output * throttle;
	}
	else
	{
		return 0;
	}
}

double lastThrottle = 0;
bool played = false;
int count = 0;
int delay = 10;

void TurboCharger::AddWhoosh(double exhaust)
{
	if (this->IS_ENABLED)
	{
		double exh = 0;

		if (exhaust < 0)
			exhaust = -exhaust;
		else
			exhaust = exhaust;

		exh = exhaust;

		if (exh < 0.000015) {
			exh = 0;
			spool -= frictionSub;
		}

		exh *= 1;

		double result = exh * this->pressMult;

		spool += result;
		lastExh = exh;
		lastExhaust = exhaust * 1;

		if (throttle == 0)
		{
			if (!played)
			{
				this->play = true;
				played = true;
			}

			count++;
			if (count >= delay && spool >= 0) {
				count = 0;
				delay = rand() % 150;
				//stu
				this->play = true;
				//Logger::DebugLine("Stu ");
			}

			if (spool > -0.5)
				spool -= 0.000000000000000000000000000000000000001;
		}
		else
		{
			count = 0;
			played = false;
		}

		if (spool > 0.1) {
			spoolSpin += spool / 10;
		}
		if (spoolSpin >= wastegateTrigger) {
			spoolSpin = wastegateTrigger;
		}

		if (spool >= this->wastegateTrigger)
			spool = this->wastegateTrigger;
		else if (spool < -0.5)
			spool = -0.5;
	}
}

void TurboCharger::Log()
{
	/*
	Logger::Debug("[TurboCharger] Spool ");
	Logger::Debug(std::to_string(this->spool));
	Logger::Debug("\n");

	Logger::Debug("[TurboCharger] Boost ");
	Logger::Debug(std::to_string(this->AddMoPowahBaby()));
	Logger::Debug("\n");

	Logger::Debug("[TurboCharger] Pressure ");
	Logger::Debug(std::to_string(this->AddPress()));
	Logger::Debug(" Bar\n");

	Logger::Debug("[TurboCharger] Exhaust Press ");
	Logger::Debug(std::to_string(this->lastExh));
	Logger::Debug(" / ");
	Logger::Debug(std::to_string(this->lastExhaust));
	Logger::Debug(" Molecules\n");

	Logger::Debug("[TurboCharger] Throttle ");
	Logger::Debug(std::to_string(this->throttle));
	Logger::Debug("\n");
	*/

	Logger::DebugLine("[TurboCharger] spin: " + std::to_string(this->spoolSpin));
	Logger::DebugLine("[TurboCharger] Spool: " + std::to_string(this->spool));
	Logger::DebugLine("[TurboCharger] Pressure: " + std::to_string(this->AddPress()) + " Bar");
}

Turbo::Turbo() {
	m_turbocharger = nullptr;
}

Turbo::~Turbo() {
	/* void */
}

void Turbo::initialize(const Parameters& params) {
	m_turbocharger = params.turbocharger;
}

void Turbo::destroy() {
	/* void */
}

double Turbo::relativeX() const {
	return m_body.p_x;
}

double Turbo::relativeY() const {
	return m_body.p_y;
}


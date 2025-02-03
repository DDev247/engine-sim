#include "../include/impulse_response.h"

ImpulseResponse::ImpulseResponse() {
    m_volume = 1.0;
}

ImpulseResponse::~ImpulseResponse() {
    /* void */
}

void ImpulseResponse::initialize(
    const std::string &filename,
    double volume)
{
    m_filename = std::string(filename.begin(), filename.end());
    m_volume = volume;
}

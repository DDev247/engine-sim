#include "../include/lerp_function.h"

#include <algorithm>
#include <string.h>
#include <assert.h>
#include <cmath>

LerpFunction::LerpFunction(Function* from, Function* to, float time) {
    m_x = m_y = nullptr;
    m_capacity = 0;
    m_size = 0;
    m_filterRadius = 0;
    m_yMin = m_yMax = 0;
    m_inputScale = 1.0;
    m_outputScale = 1.0;

    if (DefaultGaussianFilter == nullptr) {
        DefaultGaussianFilter = new GaussianFilter;
        DefaultGaussianFilter->initialize(1.0, 3.0, 1024);
    }

    m_gaussianFilter = nullptr;

    this->from = from;
    this->to = to;
    this->time = time;
}

LerpFunction::~LerpFunction() {
    assert(m_x == nullptr);
    assert(m_y == nullptr);
}

/*void LerpFunction::initialize(int size, double filterRadius, GaussianFilter* filter) {
    resize(size);
    m_size = 0;
    m_filterRadius = filterRadius;

    m_gaussianFilter = (filter != nullptr)
        ? filter
        : DefaultGaussianFilter;
}

void LerpFunction::destroy() {
    delete[] m_x;
    delete[] m_y;

    m_x = nullptr;
    m_y = nullptr;

    m_capacity = 0;
    m_size = 0;
}*/

double LerpFunction::sampleTriangle(double x) const {
    return lerp(from->sampleTriangle(x), to->sampleTriangle(x), time);
}

double LerpFunction::sampleGaussian(double x) const {
    return lerp(from->sampleGaussian(x), to->sampleGaussian(x), time);
}

double LerpFunction::triangle(double x) const {
    return lerp(from->triangle(x), to->triangle(x), time);
}

/*int LerpFunction::closestSample(double x) const {
    return (int)lerp(from->closestSample(x), to->closestSample(x), time);
}*/

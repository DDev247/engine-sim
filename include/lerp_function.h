#ifndef ATG_ENGINE_SIM_LERP_FUNCTION_H
#define ATG_ENGINE_SIM_LERP_FUNCTION_H

#include "function.h"

class LerpFunction : public Function {
    public:
        LerpFunction(Function* from, Function* to, float time);
        virtual ~LerpFunction();

        //void initialize(int size, double filterRadius, GaussianFilter *filter = nullptr);
        //void destroy();

        void setInputScale(double s) { m_inputScale = s; }
        void setOutputScale(double s) { m_outputScale = s; }

        double sampleTriangle(double x) const;
        double sampleGaussian(double x) const;
        double triangle(double x) const;
        //int closestSample(double x) const;

        inline static float lerp(float a, float b, float f) {
            return a * (1.0 - f) + (b * f);
        }

    protected:
        Function* from;
        Function* to;
        float time;
};

#endif /* ATG_ENGINE_SIM_LERP_FUNCTION_H */

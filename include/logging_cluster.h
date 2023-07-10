
#ifndef ATG_ENGINE_SIM_LOGGING_CLUSTER_H
#define ATG_ENGINE_SIM_LOGGING_CLUSTER_H

#include "ui_element.h"

#include "simulator.h"
#include "labeled_gauge.h"

class LoggingCluster : public UiElement {
public:
    LoggingCluster();
    virtual ~LoggingCluster();

    virtual void initialize(EngineSimApplication* app);
    virtual void destroy();

    virtual void update(float dt);
    virtual void render();
};

#endif /* ATG_ENGINE_SIM_LOGGING_CLUSTER_H */


#include "../include/logging_cluster.h"

#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

#include <delta-studio/include/yds_color.h>
#include <sstream>
#include <iomanip>

LoggingCluster::LoggingCluster() {
    
}

LoggingCluster::~LoggingCluster() {
    /* void */
}

void LoggingCluster::initialize(EngineSimApplication* app) {
    UiElement::initialize(app);

    //constexpr float shortenAngle = (float)units::angle(1.0, units::deg);
}

void LoggingCluster::destroy() {
    /* void */
}

void LoggingCluster::update(float dt) {
    UiElement::update(dt);
}

void LoggingCluster::render() {
    const Bounds top = m_bounds.verticalSplit(0.95f, 1.0f);
    const Bounds topInset = top.inset(5.0f);
    const Bounds bottom = m_bounds.verticalSplit(0.0f, 0.95f);
    const Bounds bottomInset = bottom.inset(5.0f);
    Grid grid;
    grid.h_cells = 1;
    grid.v_cells = 50;

    // grid.get

    drawFrame(top, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor());
    drawText("Logs", topInset, topInset.height(), Bounds::lm);
    
    drawFrame(bottom, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor());

    /*std::list<std::string> logs = m_app->tscpp.Log.GetLogs();
    auto log = logs.begin();
    int a = logs.size() > 50 ? logs.size() - 50 : 0;
    while (a > 50) {
        a = a > 50 ? a - 50 : 0;
    }
    for (int i = a; i < logs.size(); i++) {
        const Bounds bounds = grid.get(bottomInset, 0, i - a);
        drawText(*log, bounds, bounds.height(), Bounds::lm);
        log++;
    }*/

    UiElement::render();
}

/*
void LoadSimulationCluster::drawSystemStatus(const Bounds& bounds) {
    const Bounds left = bounds.horizontalSplit(0.0f, 0.6f);
    const Bounds right = bounds.horizontalSplit(0.6f, 1.0f);

    drawFrame(bounds, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor());

    Grid grid;
    grid.v_cells = 4;
    grid.h_cells = 1;

    drawText(
        "Ignition",
        grid.get(left, 0, 0).inset(10.0f),
        20.0,
        Bounds::lm);
    drawText(
        "Starter",
        grid.get(left, 0, 1).inset(10.0f),
        20.0,
        Bounds::lm);
    drawText(
        "Dyno.",
        grid.get(left, 0, 2).inset(10.0f),
        20.0,
        Bounds::lm);
    drawText(
        "Hold",
        grid.get(left, 0, 3).inset(10.0f),
        20.0,
        Bounds::lm);

    for (int i = 0; i < 4; ++i) {
        const Bounds rawBounds = grid.get(right, 0, i);
        const float width = std::fmax(rawBounds.width(), rawBounds.height());
        const Bounds squareBounds(width - 20.0f, 5.0f, rawBounds.getPosition(Bounds::center), Bounds::center);

        drawFrame(
            squareBounds,
            1.0f,
            mix(m_app->getBackgroundColor(), m_app->getForegroundColor(), 0.001f),
            mix(m_app->getBackgroundColor(), m_app->getRed(), m_systemStatusLights[i])
        );
    }
}*/

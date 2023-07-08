#include "../include/fuel_cluster.h"

#include "../include/engine_sim_application.h"

#include <sstream>
#include <iomanip>

FuelCluster::FuelCluster() {
    m_engine = nullptr;
    m_simulator = nullptr;
}

FuelCluster::~FuelCluster() {
    /* void */
}

void FuelCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_pulseWidthGauge = addElement<LabeledGauge>();
    m_sparkAdvanceGauge = addElement<LabeledGauge>();

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);

    m_pulseWidthGauge->m_title = "PULSE WIDTH";
    m_pulseWidthGauge->m_unit = "ms";
    m_pulseWidthGauge->m_precision = 2;
    m_pulseWidthGauge->setLocalPosition({ 0, 0 });
    m_pulseWidthGauge->m_gauge->m_min = 0;
    m_pulseWidthGauge->m_gauge->m_max = 50;
    m_pulseWidthGauge->m_gauge->m_minorStep = 1;
    m_pulseWidthGauge->m_gauge->m_majorStep = 5;
    m_pulseWidthGauge->m_gauge->m_maxMinorTick = 7000;
    m_pulseWidthGauge->m_gauge->m_thetaMin = (float)constants::pi * 1.2f;
    m_pulseWidthGauge->m_gauge->m_thetaMax = -(float)constants::pi * 0.2f;
    m_pulseWidthGauge->m_gauge->m_needleWidth = 4.0f;
    m_pulseWidthGauge->m_gauge->m_gamma = 1.0f;
    m_pulseWidthGauge->m_gauge->m_needleKs = 1000.0f;
    m_pulseWidthGauge->m_gauge->m_needleKd = 20.0f;
    m_pulseWidthGauge->m_gauge->setBandCount(0);

    m_sparkAdvanceGauge->m_title = "SPARK ADV.";
    m_sparkAdvanceGauge->m_unit = "deg";
    m_sparkAdvanceGauge->m_precision = 2;
    m_sparkAdvanceGauge->setLocalPosition({ 0, 0 });
    m_sparkAdvanceGauge->m_gauge->m_min = -10;
    m_sparkAdvanceGauge->m_gauge->m_max = 50;
    m_sparkAdvanceGauge->m_gauge->m_minorStep = 5;
    m_sparkAdvanceGauge->m_gauge->m_majorStep = 10;
    m_sparkAdvanceGauge->m_gauge->m_maxMinorTick = 200;
    m_sparkAdvanceGauge->m_gauge->m_thetaMin = (float)constants::pi * 1.2f;
    m_sparkAdvanceGauge->m_gauge->m_thetaMax = -(float)constants::pi * 0.2f;
    m_sparkAdvanceGauge->m_gauge->m_needleWidth = 4.0f;
    m_sparkAdvanceGauge->m_gauge->m_gamma = 1.0f;
    m_sparkAdvanceGauge->m_gauge->m_needleKs = 1000.0f;
    m_sparkAdvanceGauge->m_gauge->m_needleKd = 20.0f;
    m_sparkAdvanceGauge->m_gauge->setBandCount(0);
}

void FuelCluster::destroy() {
    UiElement::destroy();
}

void FuelCluster::update(float dt) {
    UiElement::update(dt);
}

void FuelCluster::render() {
    const Bounds top = m_bounds.verticalSplit(0.5f, 1.0f);
    const Bounds bottom = m_bounds.verticalSplit(0.0f, 0.5f);

    // draw gauges
    m_pulseWidthGauge->m_bounds = top;
    m_pulseWidthGauge->m_gauge->m_value = (m_simulator->getEngine() != nullptr) ?
        m_simulator->getEngine()->getIntake(0)->m_fuelInjectAmount / 50 :
        0.0f;

    m_sparkAdvanceGauge->m_bounds = bottom;
    m_sparkAdvanceGauge->m_gauge->m_value = (m_simulator->getEngine() != nullptr) ?
        (m_simulator->getEngine()->getIgnitionModule()->getTimingAdvance() / units::deg) * (m_simulator->getEngine()->getStarterSpeed() < 0 ? -1 : 1) :
        0.0f;

    /*drawCenteredText("FUEL", title.inset(10.0f), 24.0f);

    Grid grid;
    grid.h_cells = 1;
    grid.v_cells = 10;

    std::stringstream ss;
    ss << std::setprecision(3) << std::fixed;
    ss << units::convert(getTotalVolumeFuelConsumed(), units::L);
    ss << " L";

    const Bounds totalFuelLiters = grid.get(bodyBounds, 0, 1, 1, 2);
    drawText(ss.str(), totalFuelLiters, 32.0f, Bounds::lm);

    ss = std::stringstream();
    ss << std::setprecision(3) << std::fixed;
    ss << units::convert(getTotalVolumeFuelConsumed(), units::gal);
    ss << " gal";

    const Bounds totalFuelGallons = grid.get(bodyBounds, 0, 3, 1, 1);
    drawText(ss.str(), totalFuelGallons, 16.0f, Bounds::lm);

    const double fuelConsumed = getTotalVolumeFuelConsumed();
    const double fuelConsumed_gallons = units::convert(fuelConsumed, units::gal);

    ss = std::stringstream();
    ss << std::setprecision(2) << std::fixed;
    ss << "$" << 4.761 * fuelConsumed_gallons << " USD";

    const Bounds costUSD = grid.get(bodyBounds, 0, 4);
    drawText(ss.str(), costUSD, 16.0f, Bounds::lm);

    const double travelledDistance = (m_simulator->getVehicle() != nullptr)
        ? m_simulator->getVehicle()->getTravelledDistance()
        : 0.0;
    const double mpg = units::convert(travelledDistance, units::mile) / fuelConsumed_gallons;

    ss = std::stringstream();
    ss << std::setprecision(2) << std::fixed;
    ss << mpg << " MPG";

    const Bounds mpgBounds = grid.get(bodyBounds, 0, 6);
    drawText(ss.str(), mpgBounds, 16.0f, Bounds::lm);

    const double lp100km = (travelledDistance != 0)
        ? units::convert(fuelConsumed, units::L)
            / (units::convert(travelledDistance, units::km) / 100.0)
        : 0;

    ss = std::stringstream();
    ss << std::setprecision(2) << std::fixed;
    ss << ((lp100km > 100.0) ? 100.0 : lp100km) << " L/100 KM";

    const Bounds lp100kmBounds = grid.get(bodyBounds, 0, 7);
    drawText(ss.str(), lp100kmBounds, 12.0f, Bounds::lm);*/

    UiElement::render();
}

double FuelCluster::getTotalVolumeFuelConsumed() const {
    return (m_engine != nullptr)
        ? m_engine->getTotalVolumeFuelConsumed()
        : 0.0;
}

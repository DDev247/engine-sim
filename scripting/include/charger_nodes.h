#ifndef ATG_ENGINE_SIM_CHARGER_NODES_H
#define ATG_ENGINE_SIM_CHARGER_NODES_H

#include "object_reference_node.h"

#include "engine_sim.h"

namespace es_script {

    enum class ChargerType {
        TurboCharger,
        ProCharger,
        None
    };

    class Charger {
    public:
        Charger() { /* void */ }
        ~Charger() { /* void */ }

        ChargerType m_type = ChargerType::None;
    };

    class ChargerNode : public ObjectReferenceNode<ChargerNode> {
    public:
        ChargerNode() { /* void */ }
        virtual ~ChargerNode() { /* void */ }

        virtual Charger *generate() const = 0;

    protected:
        virtual void _evaluate() {
            setOutput(this);
            readAllInputs();
        }
    };


    class TurboCharger : public Charger {
    public:
        struct Parameters {
            double spoolBoostMult = 0.00001;
            double spoolMult = 100;

            double frictionSub = 0.001;

            double wastegateTrigger = 5;

            double pressMult = 20;
            double outputPressDiv = 5;

            double antilagBoost = 0.1;
        };

    public:
        TurboCharger() { /* void */ }
        virtual ~TurboCharger() { /* void */ }

        void initialize(const Parameters& params) {
            m_params = params;
            m_type = ChargerType::TurboCharger;
        };

    public:
        Parameters m_params;
    };

    class TurboChargerNode : public ChargerNode {
    public:
        TurboChargerNode() { /* void */ }
        virtual ~TurboChargerNode() { /* void */ }

        virtual Charger *generate() const override {
            TurboCharger*charger = new TurboCharger;
            charger->initialize(m_parameters);

            return static_cast<Charger *>(charger);
        }

    protected:
        virtual void registerInputs() override {
            addInput("boost_mult", &m_parameters.spoolBoostMult);
            addInput("spool_mult", &m_parameters.spoolMult);
            addInput("friction", &m_parameters.frictionSub);
            addInput("wastegate", &m_parameters.wastegateTrigger);
            addInput("press_mult", &m_parameters.pressMult);
            addInput("output_press_div", &m_parameters.outputPressDiv);
            addInput("antilag_boost", &m_parameters.antilagBoost);

            ChargerNode::registerInputs();
        }

        TurboCharger::Parameters m_parameters;
    };

    class ProCharger : public Charger {
    public:
        struct Parameters {
            double maxSpool = 5;

            double spoolBoostMult = 0.00001;
            double spoolMult = 0.1;

            double frictionSub = 4;

            double outputPressDiv = 5;
        };

    public:
        ProCharger() { /* void */ }
        virtual ~ProCharger() { /* void */ }

        void initialize(const Parameters& params) {
            m_params = params;
            m_type = ChargerType::ProCharger;
        };

    public:
        Parameters m_params;
    };

    class ProChargerNode : public ChargerNode {
    public:
        ProChargerNode() { /* void */ }
        virtual ~ProChargerNode() { /* void */ }

        virtual Charger* generate() const override {
            ProCharger* charger = new ProCharger;
            charger->initialize(m_parameters);

            return static_cast<Charger*>(charger);
        }

    protected:
        virtual void registerInputs() override {
            addInput("max", &m_parameters.maxSpool);
            addInput("boost_mult", &m_parameters.spoolBoostMult);
            addInput("spool_mult", &m_parameters.spoolMult);
            addInput("friction", &m_parameters.frictionSub);
            addInput("output_press_div", &m_parameters.outputPressDiv);

            ChargerNode::registerInputs();
        }

        ProCharger::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CHARGER_NODES_H */

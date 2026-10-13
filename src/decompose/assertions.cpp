#include "decompose/assertions.h"

#include <set>

// [assertions] add correction gates?
#define CORRECTIONS 1
// [assertions] add classical conditions to correction gates?
// (should be 1 usually, but depth not well-defined with classical conditions)
#define CONDITIONS 1

namespace qcore {
QCircuit insert_assertion_interpretation(QCircuit &circuit) {
    QCircuit result = circuit;
    result.getGates().clear();

    std::vector<AssertionInfo> assertions;
    std::vector<QCircuit> subcircuits;

    // get assertions, and divide existing gates into subcircuits
    unsigned sub_start=0;
    for(unsigned i=0;i<circuit.getGates().size();++i) {
        if(circuit.getGates()[i]->isAssertion()) {
            auto assertion = circuit.getGates()[i]->getAssertionInfo();
            if(assertion.isSymbolic() || assertion.type == phase_concrete) {
                throw QcoreException("[readQASM3][assertions] symbolic assertion expressions (including  and declare) and concrete phase expressions currently without decomposition");
            }
            QCircuit subcircuit;
            subcircuit.setQreg(circuit.getQregSize());
            subcircuit.addQCircuit(circuit, sub_start, circuit.getGates().size() - i);
            subcircuits.push_back(subcircuit);
            assertions.push_back(assertion);
            sub_start = i + 1;
        } else if(i==circuit.getGates().size()-1) {
            QCircuit subcircuit;
            subcircuit.addQCircuit(circuit, sub_start, 0);
            subcircuits.push_back(subcircuit);
        }
    }

    if(assertions.size() > subcircuits.size() || subcircuits.size()>assertions.size()+1) {
        std::cout << "assertions: " << assertions.size() << " vs subcircuits " << subcircuits.size() << std::endl;
        // either as many assertions as subcircuits
        // or one subcircuit more (no assertion at the end)
        throw QcoreException("[readQASM3][assertions] mismatch between assertions and subcircuits");
    }

    std::uint64_t next_qubit_index = result.getQregSize();
    std::uint64_t next_cbit_index = result.getCregSize();
    // determine ancillas
    Qubit reuse_ancilla = 0;
    for(unsigned i=0;i<assertions.size();++i) {
        for (unsigned j = 0; j < assertions[i].assert_qubits.size(); ++j) {
            assertions[i].assert_cbits.push_back(next_cbit_index);
            ++next_cbit_index;
            if (1 + (next_qubit_index - result.getQregSize()) <= result.getMaxNumAncilla()) { // new qubit possible
                assertions[i].ancilla_qubits.push_back(next_qubit_index);
                ++next_qubit_index;
            } else { // reuse
                assertions[i].ancilla_qubits.push_back(result.getQregSize() + reuse_ancilla);
            }
            reuse_ancilla = (reuse_ancilla + 1) % result.getMaxNumAncilla();
        }
    }

    // update qubits and cbits
    for (Qubit i = result.getQregSize(); i < next_qubit_index; ++i) {
        result.addQubit(i);
    }
    for (Cbit i = result.getCregSize(); i < next_cbit_index; ++i) {
        result.addCbit(i);
    }
    result.setCreg(next_cbit_index);
    result.setQreg(next_qubit_index);
    std::set<std::string> gen_refs; // generated custom gates for superposition / classical reference
    for(unsigned assertion_it=0;assertion_it<assertions.size();++assertion_it) {
        // add subcircuit before assertion
        result.addQCircuit(subcircuits[assertion_it]);
        // add assertion gates
        ClassicalCondition cond{}; // TODO assertion overall?
        AssertionInfo assertion_empty;
        if(assertions[assertion_it].type == classic_concrete) {
            std::string state = assertions[assertion_it].argument;
            std::string ref_name = "referenceC"+state;
            if(gen_refs.find(ref_name) == gen_refs.end()) {
                GateDefinition def;
                QCircuit body;
                def.name = ref_name;
                for(unsigned i=0;i<state.size(); ++i) {
                    std::string qarg = "q"+std::to_string(i);
                    def.qargs.push_back(qarg);
                    if(state[i] == '1') {
                        def.body.emplace_back(TokenType::IDENTIFIER, "x");
                        def.body.emplace_back(TokenType::IDENTIFIER, qarg);

                        // add X gate to ref circuit
                        addGate(GateType::X, 1, {}, {}, {}, {i}, cond, assertion_empty, body);
                    }
                }
                result.defineGateTxt(def.name, def);
                result.defineGate(def.name, std::make_shared<QCircuit>(body));
                assertions[assertion_it].argument = def.name;
                gen_refs.insert(def.name);
            }
        } else if(assertions[assertion_it].type == superposition_concrete) {
            std::string ref_name = "referenceS"+std::to_string(assertions[assertion_it].assert_qubits.size());
            if(gen_refs.find(ref_name) == gen_refs.end()) {
                GateDefinition def;
                QCircuit body;
                def.name = ref_name;
                for(auto assert_qubit : assertions[assertion_it].assert_qubits) {
                    std::string qarg = "q"+std::to_string(assert_qubit);
                    def.qargs.push_back(qarg);
                    def.body.emplace_back(TokenType::IDENTIFIER, "h");
                    def.body.emplace_back(TokenType::IDENTIFIER, qarg);
                    // add H gate to ref circuit
                    addGate(GateType::H, 1, {}, {}, {}, {assert_qubit}, cond, assertion_empty, body);
                }
                result.defineGateTxt(def.name, def);
                result.defineGate(def.name, std::make_shared<QCircuit>(body));
                assertions[assertion_it].argument = ref_name;
                gen_refs.insert(def.name);
            }
        }

        result.addSubcircuitGate(assertions[assertion_it].argument, assertions[assertion_it].assert_qubits, true, cond);

        // swap
        for(unsigned i=0;i<assertions[assertion_it].assert_qubits.size();++i) {
            addGate(GateType::CX, 2, CbitSet{}, {}, ControlSet{assertions[assertion_it].assert_qubits[i]},
                    TargetSet{assertions[assertion_it].ancilla_qubits[i]}, cond, assertions[assertion_it], result);
            addGate(GateType::MEASURE, 1, CbitSet{assertions[assertion_it].assert_cbits[i]}, {}, ControlSet{},
                    TargetSet{assertions[assertion_it].ancilla_qubits[i]}, cond, assertions[assertion_it], result);
            addGate(GateType::RESET, 1, CbitSet{}, {}, ControlSet{},
                    TargetSet{assertions[assertion_it].ancilla_qubits[i]}, cond, assertions[assertion_it], result);

#if CORRECTIONS == 1
#if CONDITIONS == 1
                ClassicalCondition cond_measure{eq, "c[" + std::to_string(assertions[assertion_it].assert_cbits[i]) + "]", 1};
#else
                ClassicalCondition cond_measure{};
#endif
                addGate(GateType::X, 1, CbitSet{}, {}, ControlSet{},
                        TargetSet{assertions[assertion_it].assert_qubits[i]}, cond_measure, assertions[assertion_it], result);
#endif
        }
        // redo state
        result.addSubcircuitGate(assertions[assertion_it].argument, assertions[assertion_it].assert_qubits, false, cond);
    }
    if(subcircuits.size()>assertions.size()) {
        result.addQCircuit(subcircuits.back());
    }
    return result;
}
}

/*
 * This file is part of the core quantum library package.
 *
 * Developed for the Deutsches Forschungszentrum für Künstliche
 * Intelligenz GmbH (DFKI), Cyber-Physical Systems Dept.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/**
 *  @file   Dynamic.cpp
 *  @brief  Instance Description for Dynamic Quantum Circuit Decomposition
 *  @author Majd Assaad
 *  @date   04.02.2026
 ***********************************************************/

#include "decompose/Dynamic.hpp"
#include "Definition.hpp"
#include "QGate.hpp"
#include "QCircuit.hpp"

namespace qcore {

QCircuit decompose_intermediate_RCCX_to_dynamic(
    QCircuit &intermediate_circuit,
    Qubit ancilla,
    Cbit cbit
) {
    // Create circuit with appropriate qreg and creg sizes
    regsize_t qreg_size = intermediate_circuit.getQregSize();
    regsize_t creg_size = (intermediate_circuit.getCregSize() > cbit + 1) ?
                          intermediate_circuit.getCregSize() : (cbit + 1);

    QCircuit dynamic_circuit(qreg_size, creg_size);

    // Set qubits for the output circuit (same as input)
    dynamic_circuit.setQubits(intermediate_circuit.getQubits());

    // STEP 1: Find the LAST RCCX gate that has ancilla as its TARGET
    // This is the gate that uncomputes the ancilla - we'll replace it
    // Extract its controls for the conditional CZ
    int last_rccx_index = -1;
    Qubit rccx_control_0 = 0;
    Qubit rccx_control_1 = 0;
    const auto& gates = intermediate_circuit.getGates();

    for (int i = gates.size() - 1; i >= 0; i--) {
        QGate& gate = *gates[i];
        if (gate.getType() == GateType::RCCX) {
            QubitSet targets = gate.getTargets();
            if (targets.size() == 1 && targets[0] == ancilla) {
                last_rccx_index = i;

                // Extract the two controls from this RCCX
                QubitSet controls = gate.getControls();
                if (controls.size() == 2) {
                    rccx_control_0 = controls[0];
                    rccx_control_1 = controls[1];
                }
                break;
            }
        }
    }

    if (last_rccx_index == -1) {
        throw QcoreException(
            "[decompose_intermediate_RCCX_to_dynamic] No RCCX gate with ancilla qubit " +
            std::to_string(ancilla) + " as target was found"
        );
    }

    // STEP 2: Build the output circuit
    // Copy all gates, but when we reach the last RCCX, replace it with dynamic pattern
    for (int i = 0; i < gates.size(); i++) {
        if (i == last_rccx_index) {
            // Replace final RCCX with dynamic pattern:

            // 1. SDG(ancilla) - Phase correction before measurement
            QGate sdg_gate(GateType::SDG, 1, TargetSet{ancilla});
            dynamic_circuit.addQGate(sdg_gate);

            // 2. H(ancilla) - Prepare for Hadamard basis measurement
            QGate h_gate(GateType::H, 1, TargetSet{ancilla});
            dynamic_circuit.addQGate(h_gate);

            // 3. MEASURE(ancilla) -> c[cbit]
            QGate measure_gate(GateType::MEASURE, 1,
                              CbitSet{cbit}, TargetSet{ancilla});
            dynamic_circuit.addQGate(measure_gate);

            // 4. IF (c[cbit] == 1): CZ(rccx_control_0, rccx_control_1)
            // The CZ operates on the two controls from the RCCX being replaced
            QGate cz_gate(GateType::CZ, 2,
                         ControlSet{rccx_control_0}, TargetSet{rccx_control_1});
            cz_gate.setClassical(1); // Execute when measurement result == 1
            dynamic_circuit.addQGate(cz_gate);
        } else {
            // Copy all other gates as-is (including the CCX!)
            dynamic_circuit.addQGate(*gates[i]);
        }
    }

    return dynamic_circuit;
}

QCircuit decompose_intermediate_RC3X_to_dynamic(
    QCircuit &intermediate_circuit,
    Qubit ancilla,
    Cbit cbit
) {
    // Create circuit with appropriate qreg and creg sizes
    regsize_t qreg_size = intermediate_circuit.getQregSize();
    regsize_t creg_size = (intermediate_circuit.getCregSize() > cbit + 1) ?
                          intermediate_circuit.getCregSize() : (cbit + 1);

    QCircuit dynamic_circuit(qreg_size, creg_size);

    // Set qubits for the output circuit (same as input)
    dynamic_circuit.setQubits(intermediate_circuit.getQubits());

    // STEP 1: Find the LAST RC3X gate that has ancilla as its TARGET
    // This is the gate that uncomputes the ancilla - we'll replace it
    // Extract its 3 controls for the conditional operations
    int last_rc3x_index = -1;
    Qubit rc3x_control_0 = 0;
    Qubit rc3x_control_1 = 0;
    Qubit rc3x_control_2 = 0;
    const auto& gates = intermediate_circuit.getGates();

    for (int i = gates.size() - 1; i >= 0; i--) {
        QGate& gate = *gates[i];
        if (gate.getType() == GateType::RC3X) {
            QubitSet targets = gate.getTargets();
            if (targets.size() == 1 && targets[0] == ancilla) {
                last_rc3x_index = i;

                // Extract the three controls from this RC3X
                QubitSet controls = gate.getControls();
                if (controls.size() == 3) {
                    rc3x_control_0 = controls[0];
                    rc3x_control_1 = controls[1];
                    rc3x_control_2 = controls[2];
                }
                break;
            }
        }
    }

    if (last_rc3x_index == -1) {
        throw QcoreException(
            "[decompose_intermediate_RC3X_to_dynamic] No RC3X gate with ancilla qubit " +
            std::to_string(ancilla) + " as target was found"
        );
    }

    // STEP 2: Build the output circuit
    // Copy all gates, but when we reach the last RC3X, replace it with dynamic pattern
    for (int i = 0; i < gates.size(); i++) {
        if (i == last_rc3x_index) {
            // Replace final RC3X with dynamic pattern:

            // 1. SDG(ancilla) - Phase correction before measurement
            QGate sdg_gate(GateType::SDG, 1, TargetSet{ancilla});
            dynamic_circuit.addQGate(sdg_gate);

            // 2. H(ancilla) - Prepare for Hadamard basis measurement
            QGate h_gate(GateType::H, 1, TargetSet{ancilla});
            dynamic_circuit.addQGate(h_gate);

            // 3. MEASURE(ancilla) -> c[cbit]
            QGate measure_gate(GateType::MEASURE, 1,
                              CbitSet{cbit}, TargetSet{ancilla});
            dynamic_circuit.addQGate(measure_gate);

            // 4. IF (c[cbit] == 1): CX(control_0, control_2)
            // The CX operates between first and last control
            QGate cx_gate(GateType::CX, 2,
                         ControlSet{rc3x_control_0}, TargetSet{rc3x_control_2});
            cx_gate.setClassical(1); // Execute when measurement result == 1
            dynamic_circuit.addQGate(cx_gate);

            // 5. IF (c[cbit] == 1): CC(iZ) - doubly-controlled iZ on three controls
            // Use the dedicated CCIZ gate type (doubly-controlled iZ)
            QGate cciz_gate(GateType::CCIZ, 3,
                           ControlSet{rc3x_control_0, rc3x_control_1},
                           TargetSet{rc3x_control_2});
            cciz_gate.setClassical(1); // Execute when measurement result == 1
            dynamic_circuit.addQGate(cciz_gate);

            // 6. IF (c[cbit] == 0): CS†(control_0, control_1)
            // Controlled-S-dagger between first and second control (more economical when measurement is 0)
            QGate csdg_gate(GateType::CSDG, 2,
                           ControlSet{rc3x_control_0}, TargetSet{rc3x_control_1});
            csdg_gate.setClassical(0); // Execute when measurement result == 1
            dynamic_circuit.addQGate(csdg_gate);

        } else {
            // Copy all other gates as-is (including the CCX!)
            dynamic_circuit.addQGate(*gates[i]);
        }
    }

    return dynamic_circuit;
}

}  // namespace qcore

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
 *  @file   Distributed.cpp
 *  @brief  Instance Description for Distributed Quantum Computing remote-gate protocol
 *  @author Majd Assaad 
 *  @date   07.08.2026
 ***********************************************************/

#include "decompose/Distributed.hpp"
#include "Definition.hpp"
#include "QGate.hpp"
#include "QCircuit.hpp"

namespace qcore {

// Grow the classical register (if needed) so that c[cbit] exists
static void ensureCbitExists(QCircuit &circuit, Cbit cbit) {
    if (cbit < circuit.getCregSize()) {
        return;
    }
    for (Cbit i = circuit.getCregSize(); i <= cbit; ++i) {
        circuit.addCbit(i);
    }
    circuit.setCreg(cbit + 1);
}

void append_pre_process(QCircuit &circuit, Qubit e_local, Qubit e_remote, Cbit cbit,
                         bool reset_e_local) {
    if (e_local == e_remote) {
        throw QcoreException("[append_pre_process] e_local and e_remote must be different qubits");
    }
    const auto &gates = circuit.getGates();
    bool preceded_by_matching_cx = !gates.empty() &&
        gates.back()->getType() == GateType::CX &&
        !gates.back()->getTargets().empty() &&
        gates.back()->getTargets()[0] == e_local;
    if (!preceded_by_matching_cx) {
        throw QcoreException(
            "[append_pre_process] expected the preceding gate to be CX(control, e_local)");
    }

    ensureCbitExists(circuit, cbit);

    // 1. MEASURE(e_local) -> c[cbit]
    QGate measure_gate(GateType::MEASURE, 1, CbitSet{cbit}, TargetSet{e_local});
    circuit.addQGate(measure_gate);

    // 2. RESET(e_local), so the qubit can be safely reused for a later protocol round
    if (reset_e_local) {
        QGate reset_gate(GateType::RESET, 1, TargetSet{e_local});
        circuit.addQGate(reset_gate);
    }

    // 3. IF (c[cbit] == 1): X(e_remote)
    QGate x_gate(GateType::X, 1, TargetSet{e_remote});
    x_gate.setClassical(1);
    circuit.addQGate(x_gate);
}

void append_post_process(QCircuit &circuit, Qubit original_control, Qubit e_remote,
                          Cbit cbit, bool reset_e_remote) {
    if (original_control == e_remote) {
        throw QcoreException("[append_post_process] original_control and e_remote must be different qubits");
    }

    ensureCbitExists(circuit, cbit);

    // 1. H(e_remote)
    QGate h_gate(GateType::H, 1, TargetSet{e_remote});
    circuit.addQGate(h_gate);

    // 2. MEASURE(e_remote) -> c[cbit]
    QGate measure_gate(GateType::MEASURE, 1, CbitSet{cbit}, TargetSet{e_remote});
    circuit.addQGate(measure_gate);

    // 3. RESET(e_remote), so the qubit can be safely reused for a later protocol round
    if (reset_e_remote) {
        QGate reset_gate(GateType::RESET, 1, TargetSet{e_remote});
        circuit.addQGate(reset_gate);
    }

    // 4. IF (c[cbit] == 1): Z(original_control) -- restores the control qubit
    QGate z_gate(GateType::Z, 1, TargetSet{original_control});
    z_gate.setClassical(1);
    circuit.addQGate(z_gate);
}

void append_teleport_data(QCircuit &circuit, Qubit c, Qubit e_local, Qubit e_remote,
                           Cbit cbit_c, Cbit cbit_e,
                           bool reset_c, bool reset_e_local) {
    if (c == e_local || c == e_remote || e_local == e_remote) {
        throw QcoreException("[append_teleport_data] c, e_local, e_remote must be pairwise distinct");
    }
    if (cbit_c == cbit_e) {
        throw QcoreException("[append_teleport_data] cbit_c and cbit_e must be different classical bits");
    }

    ensureCbitExists(circuit, cbit_c);
    ensureCbitExists(circuit, cbit_e);

    // 1. CX(c, e_local)
    QGate cx_gate(GateType::CX, 2, ControlSet{c}, TargetSet{e_local});
    circuit.addQGate(cx_gate);

    // 2. H(c)
    QGate h_gate(GateType::H, 1, TargetSet{c});
    circuit.addQGate(h_gate);

    // 3. MEASURE(c) -> c[cbit_c]
    QGate measure_c_gate(GateType::MEASURE, 1, CbitSet{cbit_c}, TargetSet{c});
    circuit.addQGate(measure_c_gate);

    // 4. RESET(c)
    if (reset_c) {
        QGate reset_c_gate(GateType::RESET, 1, TargetSet{c});
        circuit.addQGate(reset_c_gate);
    }

    // 5. MEASURE(e_local) -> c[cbit_e]
    QGate measure_e_gate(GateType::MEASURE, 1, CbitSet{cbit_e}, TargetSet{e_local});
    circuit.addQGate(measure_e_gate);

    // 6. RESET(e_local)
    if (reset_e_local) {
        QGate reset_e_gate(GateType::RESET, 1, TargetSet{e_local});
        circuit.addQGate(reset_e_gate);
    }

    // 7. IF (c[cbit_e] == 1): X(e_remote)
    QGate x_gate(GateType::X, 1, TargetSet{e_remote});
    x_gate.setClassical(1);
    circuit.addQGate(x_gate);

    // 8. IF (c[cbit_c] == 1): Z(e_remote)
    QGate z_gate(GateType::Z, 1, TargetSet{e_remote});
    z_gate.setClassical(1);
    circuit.addQGate(z_gate);
}

}

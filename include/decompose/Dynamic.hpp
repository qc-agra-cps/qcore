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
 *  @file   Dynamic.hpp
 *  @brief  Specification of Dynamic Quantum Circuit Decomposition
 *  @author Majd Assaad
 *  @date   04.02.2026
 ***********************************************************/

#pragma once

#include "Definition.hpp"
#include "GateType.hpp"
#include "QCircuit.hpp"

namespace qcore {

/** @brief Convert RCCX-based intermediate circuit to dynamic circuit with measurement
 *
 * @details Based on "Adaptive Clifford+T Decomposition of MCT Gates with One Clean Ancilla" (Figure 3).
 *          Replaces the ancilla CCX gate with S†, H, MEASURE, and conditional CZ.
 *          Reduces T-count by 4 compared to static decomposition.
 *
 * @param intermediate_circuit The input circuit containing RCCX and CCX gates
 * @param ancilla The clean ancilla qubit used in the decomposition
 * @param cbit The classical bit to store the measurement result (default: 0)
 * @return QCircuit The dynamic circuit with measurement and conditional operations
 * @throws QcoreException if no CCX gate using the ancilla is found
 *
 * @see decompose_intermediate_RC3X_to_dynamic
 */
QCircuit decompose_intermediate_RCCX_to_dynamic(
    QCircuit &intermediate_circuit,
    Qubit ancilla,
    Cbit cbit = 0
);

/** @brief Convert RC3X-based intermediate circuit to dynamic circuit with measurement
 *
 * @details Based on "Adaptive Clifford+T Decomposition of MCT Gates with One Clean Ancilla" (Figure 4).
 *          Similar to RCCX variant but with two conditional branches (IF c==1 and IF c==0).
 *          More efficient when measurement outcome is 0.
 *
 * @param intermediate_circuit The input circuit containing RC3X and CCX gates
 * @param ancilla The clean ancilla qubit used in the decomposition
 * @param cbit The classical bit to store the measurement result (default: 0)
 * @return QCircuit The dynamic circuit with measurement and conditional operations
 * @throws QcoreException if no CCX gate using the ancilla is found
 *
 * @see decompose_intermediate_RCCX_to_dynamic
 */
QCircuit decompose_intermediate_RC3X_to_dynamic(
    QCircuit &intermediate_circuit,
    Qubit ancilla,
    Cbit cbit = 0
);

}  // namespace qcore

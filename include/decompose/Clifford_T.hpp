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
 *  @file   Clifford_T.hpp
 *  @brief  Specification of Quantum Gate Decomposition using Clifford+T gates
 *  @author Abhoy Kole
 *  @date   04.05.2025
 ***********************************************************/
//need to check
#pragma once

#include "Definition.hpp"
#include "GateType.hpp"
#include "QCircuit.hpp"

namespace qcore {

/** @brief Decomposing a 2-control Toffoli gate using a set of Clifford+T gates
 *
 *
 * @details Amy, Matthew, et al. "A meet-in-the-middle algorithm for fast synthesis of depth-optimal
 *          quantum circuits." IEEE Transactions on Computer-Aided Design of Integrated Circuits and
 *          Systems 32.6 (2013): 818-830. Fig. 7(a)
 * 
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit
 *  @param inverse The inverse flag for performing inverse decompositon (Default False)
 *  @return Qcircuit& The decomposed 3-qubit quantum circuit realizing Toffoli operation using Clifford+T gates
 */

QCircuit decompose_CCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t, bool inverse = false);

/** @brief Decomposing a 2-control relative phase Toffoli gate using a set of Clifford+T gates
 *
 * @details Dmitri, Maslov. "On the advantages of using relative phase Toffolis with an application to
 *          multiple control Toffoli optimization." Phys. Rev. A 93 (2015). Fig. 3, dashed (Identical Inverse)
 * 
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit
 *  @return Qcircuit& The decomposed 3-qubit quantum circuit realizing relative phase Toffoli operation using Clifford+T gates
 */
QCircuit decompose_RCCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t);  // self inverse

/** @brief Decomposing a 2-control relative phase Toffoli gate followed by a V (square root of NOT) gate using a set of Clifford+T gates
 *
 * @details Dmitri, Maslov. "On the advantages of using relative phase Toffolis with an application to
 *          multiple control Toffoli optimization." Phys. Rev. A 93 (2015). Fig. 3, gates 2-6
 * 
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit [V(c2,t)]
 *  @param inverse The inverse flag for performing inverse decompositon (Default False)
 *  @return Qcircuit& The decomposed 3-qubit quantum circuit realizing relative phase Toffoli operation using Clifford+T gates
 */
QCircuit decompose_SRCCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t, bool inverse = false);

/** @brief Decomposing a 2-control special form relative phase Toffoli gate followed by a V (square root of NOT) gate using a set of Clifford+T gates
 *
 * @details Dmitri, Maslov. "On the advantages of using relative phase Toffolis with an application to
 *          multiple control Toffoli optimization." Phys. Rev. A 93 (2015). Equation. 3, dashed
 * 
 *  @param c1 The 1st control qubit    
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit [V(c1,t)]
 *  @param inverse The inverse flag for performing inverse decompositon (Default False)
 *  @return Qcircuit& The decomposed 3-qubit quantum circuit realizing relative phase Toffoli operation using Clifford+T gates
 */
QCircuit decompose_SSRCCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t, bool inverse = false);

/** @brief Decomposing a 3-control relative phase Toffoli gate using a set of Clifford+T gates
 *
 * @details Dmitri, Maslov. "On the advantages of using relative phase Toffolis with an application to
 *          multiple control Toffoli optimization." Phys. Rev. A 93 (2015). Fig. 4 (Identical Inverse)
 * 
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param c3 The 3rd control qubit
 *  @param t The target qubit
 *  @return Qcircuit& The decomposed 4-qubit quantum circuit realizing a 3-control relative phase Toffoli operation using Clifford+T gates
 */
QCircuit decompose_RC3X_Clifford_T(Qubit &c1, Qubit &c2, Qubit &c3, Qubit &t);  // self inverse

/** @brief Decomposing a 3-control relative phase Toffoli gate followed by a V gate using a set of Clifford+T gates
 *
 * @details Dmitri, Maslov. "On the advantages of using relative phase Toffolis with an application to
 *          multiple control Toffoli optimization." Phys. Rev. A 93 (2015). Fig. 4, dashed
 * 
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param c3 The 3rd control qubit
 *  @param t The target qubit [V(c2, c3, t)]
 *  @param inverse The inverse flag for performing inverse decompositon (Default False)
 *  @return Qcircuit& The decomposed 4-qubit quantum circuit realizing a 3-control relative phase Toffoli operation using Clifford+T gates
 */
QCircuit decompose_SRC3X_Clifford_T(Qubit &c1, Qubit &c2, Qubit &c3, Qubit &t, bool inverse = false);

/** @brief Decomposing a multi-control Toffoli gate using a set of Clifford+T gates
 *
 * @details Dmitri, Maslov. "On the advantages of using relative phase Toffolis with an application to
 *          multiple control Toffoli optimization." Phys. Rev. A 93 (2015).
 *
 *  @param controls The set of control qubits
 *  @param t The target qubit
 *  @param dirty The set of dirty ancilla qubits
 *  @param clean The set of clean ancilla qubits
 *  @param inverse The inverse flag for performing inverse decompositon (Default False)
 *  @return Qcircuit& The decomposed quantum circuit realizing a multi-control control Toffoli operation using Clifford+T gates.
 */
QCircuit decompose_MCT_Clifford_T(QubitSet &controls, Qubit target, QubitSet &dirty, QubitSet &clean, bool inverse = false);  // Cliffort + T decomposition

}  // namespace qcore
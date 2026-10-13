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
 *  @author Majd Assaad
 *  @date   04.05.2025
 ***********************************************************/
//need to check
#pragma once

#include "Definition.hpp"
#include "GateType.hpp"
#include "QCircuit.hpp"
#include <set>

namespace qcore {

struct DecompositionConfiguration {
    bool use_low_tdepth_decompositions;
    bool allow_target_error = false;
    bool allow_dynamic_measurement = false;

    // For use during the dewcomposition
    bool has_uncomputation_partner = false;
    bool is_inverse = false;

  DecompositionConfiguration withRelativePhase(bool is_inverse, bool allow_dynamic_measurement=false, bool allow_target_error=false) const {
        DecompositionConfiguration copy = *this;
        copy.has_uncomputation_partner = true;
        copy.is_inverse = is_inverse;
		copy.allow_target_error = allow_target_error;
        copy.allow_dynamic_measurement = allow_dynamic_measurement;
        return copy;
    }

    static const DecompositionConfiguration ReduceTDepth;
    static const DecompositionConfiguration ReduceCNotCount;
};
inline constexpr DecompositionConfiguration DecompositionConfiguration::ReduceTDepth = {
    .use_low_tdepth_decompositions = true,
};
inline constexpr DecompositionConfiguration DecompositionConfiguration::ReduceCNotCount = {
    .use_low_tdepth_decompositions = false,
};

QCircuit decompose_meta_gates(const QCircuit &in);
QCircuit decompose_resolve_RCCX_gates(const QCircuit &in, const DecompositionConfiguration &config);
QCircuit decompose_resolve_Toffoli(const DecompositionConfiguration &config,
                                   const QubitSet &controls, Qubit target,
                                   const QubitSet &clean_ancilla, const QubitSet &dirty_ancilla);

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

QCircuit decompose_CCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse = false);

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
QCircuit decompose_RCCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t);  // self inverse

/** @brief CC(iX)^(2): T-depth-2 relative-phase Toffoli, no ancilla required
 *
 * @details Schnittka et al. "Low T-Depth Clifford+T Decomposition of Large Toffoli
 *          Gates using Dynamic Phase Correction." Fig. 1c (equiv. to Gidney, "Halving the
 *          cost of quantum addition"; Jones, "Low-overhead constructions for the
 *          fault-tolerant Toffoli gate").
 *
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit
 *  @return QCircuit The decomposed 3-qubit quantum circuit realizing relative phase Toffoli operation using Clifford+T gates at T-depth 2 (T-count 4)
 */
QCircuit decompose_RCCX_Clifford_T_depth2(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse);

QCircuit decompose_RCCX_Clifford_T_dynamic_uncompute(const Qubit &c1, const Qubit &c2, const Qubit &t);

/** @brief CC(iX)^(1): T-depth-1 relative-phase Toffoli using one clean ancilla
 *
 * @details Schnittka et al. "Low T-Depth Clifford+T Decomposition of Large Toffoli
 *          Gates using Dynamic Phase Correction." Fig. 1d, building on P. Selinger,
 *          "Quantum circuits of T-depth one." Phys. Rev. A 87, 042302 (2013).
 *
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit
 *  @param a A clean ancilla qubit
 *  @return QCircuit The decomposed 4-qubit quantum circuit realizing relative phase Toffoli operation using Clifford+T gates at T-depth 1 (T-count 4)
 */
QCircuit decompose_RCCX_Clifford_T_depth1(const Qubit &c1, const Qubit &c2, const Qubit &t, const Qubit &a);

/** @brief CC(iZ)^(4): T-depth-4 doubly-controlled-iZ
 *
 * @details Dmitri, Maslov. "On the advantages of using relative phase Toffolis with an application to
 *          multiple control Toffoli optimization." Phys. Rev. A 93 (2015). Fig. 3, dashed (Identical Inverse).
 *          Schnittka et al. "Low T-Depth Clifford+T Decomposition of Large Toffoli
 *          Gates using Dynamic Phase Correction." Fig. 1b
 *
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit
 *  @return QCircuit The decomposed 3-qubit quantum circuit realizing CC(iZ) operation using Clifford+T gates
 */
QCircuit decompose_CCIZ_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t);

/** @brief CC(iZ)^(2): T-depth-2 doubly-controlled-iZ
 *
 * @details Schnittka et al. "Low T-Depth Clifford+T Decomposition of Large Toffoli
 *          Gates using Dynamic Phase Correction." Fig. 1c.
 *
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit
 *  @return QCircuit The decomposed 3-qubit quantum circuit realizing CC(iZ) operation using Clifford+T gates at T-depth 2 (T-count 4)
 */
QCircuit decompose_CCIZ_Clifford_T_depth2(const Qubit &c1, const Qubit &c2, const Qubit &t);

/** @brief CC(iZ)^(1): T-depth-1 doubly-controlled-iZ using one clean ancilla
 *
 * @details Schnittka, Till, et al. "Low T-Depth Clifford+T Decomposition of Large Toffoli
 *          Gates using Dynamic Phase Correction." Fig. 1d,
 *          building on P. Selinger, "Quantum circuits of T-depth one." Phys. Rev. A 87,
 *          042302 (2013).
 *
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param t The target qubit
 *  @param a A clean ancilla qubit 
 *  @return QCircuit The decomposed 4-qubit quantum circuit realizing CC(iZ) operation using Clifford+T gates at T-depth 1 (T-count 4)
 */
QCircuit decompose_CCIZ_Clifford_T_depth1(const Qubit &c1, const Qubit &c2, const Qubit &t, const Qubit &a);


/** @brief CS^(1): T-depth-1 controlled-S using one clean ancilla
 *
 * @details Schnittka et al. Fig. 2c.
 *          T-count 3, T-depth 1.
 *
 *  @param c The control qubit
 *  @param t The target qubit
 *  @param a A clean ancilla qubit
 *  @return QCircuit The decomposed 3-qubit quantum circuit realizing CS operation using Clifford+T gates at T-depth 1 (T-count 3)
 */
QCircuit decompose_CS_Clifford_T_depth1(const Qubit &c, const Qubit &t, const Qubit &a);

/** @brief CS†^(1): T-depth-1 controlled-S-dagger using one clean ancilla
 *
 * @details Adjoint of CS^(1)
 *          T-count 3, T-depth 1.
 *
 *  @param c The control qubit
 *  @param t The target qubit
 *  @param a A clean ancilla qubit
 *  @return QCircuit The decomposed 3-qubit quantum circuit realizing CS† operation using Clifford+T gates at T-depth 1 (T-count 3)
 */
QCircuit decompose_CSDG_Clifford_T_depth1(const Qubit &c, const Qubit &t, const Qubit &a);


/** @brief Decomposing a controlled-S-dagger gate using a set of Clifford+T gates
 *
 * @details The controlled-S-dagger (CSDG) gate applies the S† (S-dagger) phase gate to the target
 *          qubit when the control qubit is |1⟩.
 *
 *  @param c The control qubit
 *  @param t The target qubit
 *  @return QCircuit The decomposed 2-qubit quantum circuit realizing CSDG operation using Clifford+T gates
 */
QCircuit decompose_CSDG_Clifford_T(const Qubit &c, const Qubit &t);

/** @brief Decomposing a controlled-Z gate using a set of Clifford+T gates
 *
 * @details The controlled-Z (CZ) gate applies a Z gate to the target qubit when the control
 *          qubit is |1⟩.
 *
 *  @param c The control qubit
 *  @param t The target qubit
 *  @return QCircuit The decomposed 2-qubit quantum circuit realizing CZ operation using Clifford+T gates
 */
QCircuit decompose_CZ_Clifford_T(const Qubit &c, const Qubit &t);

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
QCircuit decompose_SRCCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse = false);

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
QCircuit decompose_SSRCCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse = false);

/** @brief C3(iX)^(8): T-depth-8 realization of a 3-control relative phase Toffoli gate using a set of Clifford+T gates
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
QCircuit decompose_RC3X_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &c3, const Qubit &t, bool inverse);  // NOT! self inverse

/** @brief C3(iX)^(4): T-depth-4 realization of a 3-control relative phase Toffoli gate using a set of Clifford+T gates
 *
 * @details Schnittka et al. "Low T-Depth Clifford+T Decomposition of Large Toffoli
 *          Gates using Dynamic Phase Correction." Fig. 4b.
 *
 *  @param c1 The 1st control qubit
 *  @param c2 The 2nd control qubit
 *  @param c3 The 3rd control qubit
 *  @param t The target qubit
 *  @return QCircuit The decomposed 4-qubit quantum circuit realizing a 3-control relative phase Toffoli operation using Clifford+T gates at T-depth 4 (T-count 8)
 */
QCircuit decompose_RC3X_Clifford_T_depth4(const Qubit &c1, const Qubit &c2, const Qubit &c3, const Qubit &t, bool is_inverse);

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
QCircuit decompose_SRC3X_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &c3, const Qubit &t, bool inverse = false);


/** @brief Decompose an n-controlled X gate (CnX) into RCCX, CCX, and X gates using a linear
 *         conditionally-clean ancilla ladder (pair encoding)
 *
 * @details Builds a binary tree of RCCX operations over a marked/unmarked scheduling scheme.
 *          Each RCCX encodes a pair of controls, and conditionally-clean control qubits are
 *          reused as intermediate targets. The ladder computes the n-way AND of all controls
 *          into ancilla + one final control, applies CCX to the target, then uncomputes.
 *          Requires one clean ancilla qubit.
 *
 *  @param controls The set of control qubits (n >= 3)
 *  @param target The target qubit
 *  @param ancilla A clean ancilla qubit (initialized to |0>)
 *  @return QCircuit containing only RCCX, CCX, and X gates
 */
  QCircuit decompose_CNX_Linear_RCCX(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, Qubit ancilla, bool is_dirty=false);

/** @brief Decompose an n-controlled X gate (CnX) into RC3X, CCX, CX, and X gates using a linear
 *         conditionally-clean ancilla ladder (triplet encoding)
 *
 * @details Uses RC3X (3-control relative phase Toffoli) for triplet encoding. The first control
 *          qubit (controls[0]) is used as a fixed "enable" in every RC3X gate, while the remaining
 *          controls are scheduled via the same ladder algorithm as the RCCX variant. This results
 *          in n-3 ladder operations (vs n-2 for RCCX), yielding fewer gates overall.
 *
 *  @param controls The set of control qubits (n >= 3)
 *  @param target The target qubit
 *  @param ancilla A clean ancilla qubit (initialized to |0>)
 *  @return QCircuit containing only RC3X, CCX, CX, and X gates
 */
QCircuit decompose_CNX_Linear_RC3X(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, Qubit ancilla);

/** @brief Decompose an n-controlled X gate (CnX) into RC3X, CCX, CX, and X gates using a loarithmic
 *         conditionally-clean ancilla construction 
 *
 *  @param controls The set of control qubits (n >= 3)
 *  @param target The target qubit
 *  @param ancilla1 A clean ancilla qubit (initialized to |0>)
 *  @param ancilla2 A clean ancilla qubit (initialized to |0>)
 *  @param rc3x_steps Steps of the algorithm to use RC3X-Gates on
 *  @return QCircuit containing only RC3X, CCX, CX, and X gates
 */
QCircuit decompose_CNX_Logarithmic_Mixed(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, Qubit ancilla1, Qubit ancilla2, std::set<int> rc3x_steps = {});

/** @brief Decompose an n-controlled X gate (CnX) into RC3X, RCCX and CCX gates using a tree of
 *         RC3X gates on ceil((n-2)/2) clean ancillas (T-depth grows with log(n))
 *
 *  @param controls The set of control qubits (n >= 3)
 *  @param target The target qubit
 *  @param clean_ancilla At least ceil((n-2)/2) clean ancilla qubits (initialized to |0>)
 *  @return QCircuit containing only RC3X, RCCX and CCX gates
 */
QCircuit decompose_CNX_Tree_RC3X(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, const QubitSet &clean_ancilla);

/** @brief Decompose all MCX gates in a circuit to RCCX-based intermediate form
 *
 * @details Iterates over the circuit and decomposes each MCX gate with n >= 3 controls
 *          using decompose_CNX_Linear_RCCX. MCX with 2 controls becomes CCX, with 1 becomes CX.
 *          All other gate types pass through unchanged.
 *
 *  @param circuit The input quantum circuit
 *  @param ancilla A clean ancilla qubit (initialized to |0>)
 *  @return QCircuit containing RCCX, CCX, CX, X, and other original gates
 */
QCircuit decompose_circuit_to_intermediate_RCCX(QCircuit &circuit, Qubit ancilla);

/** @brief Decompose all MCX gates in a circuit to RC3X-based intermediate form
 *
 * @details Iterates over the circuit and decomposes each MCX gate with n >= 3 controls
 *          using decompose_CNX_Linear_RC3X. MCX with 2 controls becomes CCX, with 1 becomes CX.
 *          All other gate types pass through unchanged.
 *
 *  @param circuit The input quantum circuit
 *  @param ancilla A clean ancilla qubit (initialized to |0>)
 *  @return QCircuit containing RC3X, CCX, CX, X, and other original gates
 */
QCircuit decompose_circuit_to_intermediate_RC3X(QCircuit &circuit, Qubit ancilla);

/** @brief Decompose a circuit to the Clifford+T gate set
 *
 * @details Decomposes all multi-control gates to Clifford+T primitives (H, T, TDG, CX, X).
 *          If MCX gates are present, first decomposes them to intermediate form via
 *          decompose_circuit_to_intermediate_RCCX. Then decomposes all intermediate gates
 *          (RCCX, RC3X, CCX) and helper gates (CCIZ, CSDG, CZ) to Clifford+T.
 *          Preserves conditional execution for dynamic circuits. All other gates pass through unchanged.
 *
 *  @param circuit The input quantum circuit
 *  @param ancilla A clean ancilla qubit (initialized to |0>)
 *  @return QCircuit containing only Clifford+T gates (H, T, TDG, CX, X) plus any original gates that are already in this set
 */
QCircuit decompose_circuit_to_clifford_t(QCircuit &circuit, Qubit ancilla);

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
QCircuit decompose_MCT_Clifford_T(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, const QubitSet &dirty, const QubitSet &clean);  // Cliffort + T decomposition

}  // namespace qcore

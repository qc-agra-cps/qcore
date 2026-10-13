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
 *  @file   Distributed.hpp
 *  @brief  Specification of Distributed Quantum Computing remote-gate protocol
 *  @author Majd Assaad 
 *  @date   04.08.2026
 ***********************************************************/

#pragma once

#include "Definition.hpp"
#include "GateType.hpp"
#include "QCircuit.hpp"

namespace qcore {

/** @brief Complete the cat-entangler half of a remote-gate protocol (CAT-ENT)
 *
 * @details D. Ferrari, S. Carretta and M. Amoretti, "A Modular Quantum Compilation Framework 
 *          for Distributed Quantum Computing," in IEEE Transactions on Quantum Engineering, 
 *          vol. 4, pp. 1-13, 2023.
 *
 * @param circuit Circuit to append to (mutated in place)
 * @param e_local Local half of the shared Bell pair, already CX'd with the control
 * @param e_remote Remote half of the shared Bell pair (now stands in for the control)
 * @param cbit Scratch classical bit for the measurement outcome. Safe to reuse across
 *             independent protocol rounds since it is consumed immediately
 * @param reset_e_local Reset e_local after measuring so it can be safely reused (default true)
 * @throws QcoreException if the last gate in `circuit` is not a CX targeting e_local,
 *         or if e_local == e_remote
 *
 * @see append_post_process
 */
void append_pre_process(QCircuit &circuit, Qubit e_local, Qubit e_remote, Cbit cbit,
                         bool reset_e_local = true);

/** @brief Complete the cat-disentangler half of a remote-gate protocol (CAT-DISENT)
 *
 * @details D. Ferrari, S. Carretta and M. Amoretti, "A Modular Quantum Compilation Framework 
 *          for Distributed Quantum Computing," in IEEE Transactions on Quantum Engineering, 
 *          vol. 4, pp. 1-13, 2023.
 *
 * @param circuit Circuit to append to (mutated in place)
 * @param original_control The original control qubit, restored by this step
 * @param e_remote Remote half of the shared Bell pair being disentangled
 * @param cbit Scratch classical bit for the measurement outcome
 * @param reset_e_remote Reset e_remote after measuring (default true)
 * @throws QcoreException if original_control == e_remote
 *
 * @see append_pre_process
 */
void append_post_process(QCircuit &circuit, Qubit original_control, Qubit e_remote,
                          Cbit cbit, bool reset_e_remote = true);

/** @brief Teleport a qubit's state onto the remote half of a shared Bell pair (TELEPORT)
 *
 * @details D. Ferrari, S. Carretta and M. Amoretti, "A Modular Quantum Compilation Framework 
 *          for Distributed Quantum Computing," in IEEE Transactions on Quantum Engineering, 
 *          vol. 4, pp. 1-13, 2023.
 *
 * @param circuit Circuit to append to (mutated in place)
 * @param c The qubit whose state is being teleported (consumed by this operation)
 * @param e_local Local half of the shared Bell pair
 * @param e_remote Remote half of the shared Bell pair (receives c's teleported state)
 * @param cbit_c Scratch classical bit for c's measurement outcome
 * @param cbit_e Scratch classical bit for e_local's measurement outcome
 * @param reset_c Reset c after measuring (default true)
 * @param reset_e_local Reset e_local after measuring (default true)
 * @throws QcoreException if c, e_local, e_remote are not pairwise distinct, or if
 *         cbit_c == cbit_e
 */
void append_teleport_data(QCircuit &circuit, Qubit c, Qubit e_local, Qubit e_remote,
                           Cbit cbit_c, Cbit cbit_e,
                           bool reset_c = true, bool reset_e_local = true);

}
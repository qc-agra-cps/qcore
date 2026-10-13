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
 *  @file   Clifford_T.cpp
 *  @brief  Instance Description for Quantum Gate Decomposition using Clifford+T Gates
 *  @author Abhoy Kole
 *  @author Majd Assaad
 *  @date   04.05.2025
 ***********************************************************/

#include "decompose/Clifford_T.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <vector>
#include <optional>

namespace qcore {

template<typename T>
std::vector<T> operator|(const std::vector<T> &a, const std::vector<T> &b) {
  std::vector<T> result;
  std::copy(a.begin(), a.end(), std::back_inserter(result));
  std::copy(b.begin(), b.end(), std::back_inserter(result));
  return result;
}

QCircuit decompose_meta_gates(const QCircuit &in) {
    QCircuit qc;
    qc.cloneEmpty(in);

    for (const auto &gate : in.getGates()) {
        auto add_circuit = [&qc, &gate](QCircuit const &circuit) {
            for (const auto &sub : circuit.getGates()) {
                sub->setClassical(gate->getClassical());
                qc.addQGate(*sub);
            }
        };
        switch (gate->getType()) {
            case GateType::CCX:
                add_circuit(decompose_CCX_Clifford_T(gate->getControls()[0], gate->getControls()[1],
                                                            gate->getTargets()[0]));
                break;
            case GateType::RCCXD2:
                add_circuit(decompose_RCCX_Clifford_T_depth2(gate->getControls()[0], gate->getControls()[1],
                                                             gate->getTargets()[0], false));
                break;
            case GateType::RCCXADGDC:
            case GateType::RCCXD2DGDC:
                add_circuit(decompose_RCCX_Clifford_T_dynamic_uncompute(gate->getControls()[0], gate->getControls()[1], gate->getTargets()[0]));
                break;
            case GateType::RCCXD2DG:
                add_circuit(decompose_RCCX_Clifford_T_depth2(gate->getControls()[0], gate->getControls()[1],
                                                             gate->getTargets()[0], true));
                break;
            case GateType::RCCXA:
                add_circuit(decompose_RCCX_Clifford_T_depth1(gate->getControls()[0], gate->getControls()[1],
                                                             gate->getTargets()[0], gate->getControls()[2])
                                .inverse());
                break;
            case GateType::RCCXADG:
                add_circuit(decompose_RCCX_Clifford_T_depth2(gate->getControls()[0], gate->getControls()[1],
                                                             gate->getTargets()[0], true));
                break;
            case GateType::RCCX:
                add_circuit(
                    decompose_RCCX_Clifford_T(gate->getControls()[0], gate->getControls()[1], gate->getTargets()[0]));
                break;
            case GateType::RC3XD4:
                add_circuit(decompose_RC3X_Clifford_T_depth4(gate->getControls()[0], gate->getControls()[1],
                                                      gate->getControls()[2], gate->getTargets()[0], false));
                break;
            case GateType::RC3XD4DG:
                add_circuit(decompose_RC3X_Clifford_T_depth4(gate->getControls()[0], gate->getControls()[1],
                                                      gate->getControls()[2], gate->getTargets()[0], true));
                break;
            case GateType::RC3X:
                add_circuit(decompose_RC3X_Clifford_T(gate->getControls()[0], gate->getControls()[1],
                                                      gate->getControls()[2], gate->getTargets()[0], false));
                break;
            case GateType::RC3XDG:
                add_circuit(decompose_RC3X_Clifford_T(gate->getControls()[0], gate->getControls()[1],
                                                      gate->getControls()[2], gate->getTargets()[0], true));
                break;
            default:
                qc.addQGate(*gate);
        }
    }
    return qc;
}

QCircuit decompose_resolve_Toffoli(const DecompositionConfiguration &config,
                                   const QubitSet &controls, Qubit target,
                                   const QubitSet &clean_ancilla, const QubitSet &dirty_ancilla) {

  // std::cout << "decomposing toffoli: ["; for(auto q : controls) std::cout << q<< ","; std::cout << "] -> " << target << " c:["; for(auto q : clean_ancilla) std::cout << q<< ","; std::cout << "] d:[";for(auto q : dirty_ancilla) std::cout << q<< ","; std::cout << "]\n";
  auto with_meta_gate = [&controls, &target, &clean_ancilla, &dirty_ancilla](GateType type, const QubitSet &gate_controls, Qubit gate_target) {
	  QCircuit qc;
	  qc.setQubits(controls | QubitSet{target} | clean_ancilla | dirty_ancilla);
	  if (type != GateType::NONE) {
		qc.addQGate(QGate(type, gate_controls.size()+1, (ControlSet)gate_controls, {gate_target}));
	  }
	  return qc;
  };

  if (controls.size() < 2) {
      // Resolve to either  identity or CX
      if (!controls.empty()) {
          return with_meta_gate(GateType::CX, {controls.front()}, target);
      }
      return with_meta_gate(GateType::NONE, {controls.front()}, target);
  }
  if (controls.size() < 3) {
      if (!config.has_uncomputation_partner) {
		  return with_meta_gate(GateType::CCX, {controls[0], controls[1]}, target);
      }
      if (!config.use_low_tdepth_decompositions) {
		  return with_meta_gate(GateType::RCCX, {controls[0], controls[1]}, target);
      }
      if (!clean_ancilla.empty()) {
		return with_meta_gate(config.is_inverse ? (config.allow_dynamic_measurement ? GateType::RCCXADGDC : GateType::RCCXADG) : GateType::RCCXA, {controls[0], controls[1], clean_ancilla[0]}, target);
      }
	  return with_meta_gate(config.is_inverse ? (config.allow_dynamic_measurement ? GateType::RCCXD2DGDC : GateType::RCCXD2DG) : GateType::RCCXD2, {controls[0], controls[1]}, target);
  }
  if (controls.size() < 4 && config.has_uncomputation_partner) {
    if (!config.use_low_tdepth_decompositions) {
		return with_meta_gate(config.is_inverse ? GateType::RC3XDG : GateType::RC3X, {controls[0], controls[1], controls[2]}, target);
	}
	return with_meta_gate(config.is_inverse ? GateType::RC3XD4DG : GateType::RC3XD4, {controls[0], controls[1], controls[2]}, target);
  }

  // With ceil((n-2)/2) clean ancillas, a tree of RC3X gates has a lower T-depth than the linear
  // ladders from n = 5 on (same T count; same CNOT count with ReduceCNotCount)
  if (controls.size() >= 5 && clean_ancilla.size() >= (controls.size() - 1) / 2) {
	return decompose_CNX_Tree_RC3X(config, controls, target, clean_ancilla);
  }
#if 0 // make sure logarithmic decomposition is stable
  if (clean_ancilla.size() >= 2) {
	return decompose_CNX_Logarithmic_Mixed(config, controls, target, clean_ancilla[0], clean_ancilla[1]);
  }
#endif 
  if (!clean_ancilla.empty()) {
	std::cout << "as linear r3x!\n";
	// Linear_RC3X is cheaper from n = 11 on (T-depth) and from n = 7 on (same CNOT count, lower T-depth)
	if (controls.size() >= (config.use_low_tdepth_decompositions ? 11 : 7)) {
		return decompose_CNX_Linear_RC3X(config, controls, target, clean_ancilla[0]);
	}
	return decompose_CNX_Linear_RCCX(config, controls, target, clean_ancilla[0], false);
  }
  if (!dirty_ancilla.empty()) {
	return decompose_CNX_Linear_RCCX(config, controls, target, dirty_ancilla[0], true);
  }
  return decompose_MCT_Clifford_T(config, controls, target, dirty_ancilla, clean_ancilla);
}


QCircuit decompose_CCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});
    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    } else {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}

// CC(iX)^(4) - T-depth-4 doubly-controlled-iX gate
QCircuit decompose_RCCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});

    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));

    return qc;
}

// CC(iX)^(2) - T-depth-2 doubly-controlled-iX gate
QCircuit decompose_RCCX_Clifford_T_depth2(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});

    qc.addQGate(QGate(GateType::H,   1, {t}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c2}));
    // T-layer (all three gates are in a single T-depth layer)
    qc.addQGate(QGate(GateType::TDG, 1, {c1}));
    qc.addQGate(QGate(GateType::TDG, 1, {c2}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c2}));
    qc.addQGate(QGate(GateType::H,   1, {t}));

    return inverse ? qc : qc.inverse();
}

// CC(iX)^(1) - T-depth-1 doubly-controlled-iX gate using one clean ancilla
QCircuit decompose_RCCX_Clifford_T_depth1(const Qubit &c1, const Qubit &c2, const Qubit &t, const Qubit &a) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t, a});

    qc.addQGate(QGate(GateType::H,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {a}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c2}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {a}));
    // T-layer (all four gates are in a single T-depth layer)
    qc.addQGate(QGate(GateType::TDG, 1, {c1}));
    qc.addQGate(QGate(GateType::TDG, 1, {c2}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::T,   1, {a}));
    // Uncompute
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {a}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c2}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {a}));
    qc.addQGate(QGate(GateType::H,   1, {t}));

    return qc;
}

QCircuit decompose_RCCX_Clifford_T_dynamic_uncompute(const Qubit &c1, const Qubit &c2, const Qubit &t) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});
	Cbit cbit = qc.getCbits().size();
	qc.addCbit(cbit);

	qc.addQGate({GateType::SDG, 1, (ControlSet){}, (TargetSet){t}});
	qc.addQGate({GateType::H, 1, (ControlSet){}, (TargetSet){t}});

    auto subcircuit = QCircuit{};
    subcircuit.setQubits({c1, c2});
    subcircuit.addCbit(cbit);
	subcircuit.addQGate({GateType::CZ, 2, (ControlSet){c1}, (TargetSet){c2}});

	qc.addQCircuit(subcircuit, ClassicalCondition{"c[" + std::to_string(cbit) + "]", 1});
	return qc;
}

// CC(iZ)^(4) - T-depth-4 doubly-controlled-iZ gate
QCircuit decompose_CCIZ_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});

    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));

    return qc;
}

// CC(iZ)^(2) - T-depth-2 doubly-controlled-iZ gate
QCircuit decompose_CCIZ_Clifford_T_depth2(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});

    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c2}));
    // T-layer (all three gates are in a single T-depth layer)
    qc.addQGate(QGate(GateType::TDG, 1, {c1}));
    qc.addQGate(QGate(GateType::TDG, 1, {c2}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c2}));

    return inverse ? qc : qc.inverse();
}

// CC(iZ)^(1) - T-depth-1 doubly-controlled-iZ gate using one clean ancilla
QCircuit decompose_CCIZ_Clifford_T_depth1(const Qubit &c1, const Qubit &c2, const Qubit &t, const Qubit &a) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t, a});

    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {a}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c2}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {a}));
    // T-layer
    qc.addQGate(QGate(GateType::TDG, 1, {c1}));
    qc.addQGate(QGate(GateType::TDG, 1, {c2}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::T,   1, {a}));
    // Uncompute
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {a}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c2}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {a}));

    return qc;
}

// CS^(2) - T-depth-2 controlled-S gate
// TODO

// CS^(1) - T-depth-1 controlled-S gate using one clean ancilla
QCircuit decompose_CS_Clifford_T_depth1(const Qubit &c, const Qubit &t, const Qubit &a) {
    auto qc = QCircuit{};
    qc.setQubits({c, t, a});

    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {a}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c}, {t}));
    // T-layer
    qc.addQGate(QGate(GateType::TDG, 1, {c}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::T,   1, {a}));
    // Uncompute
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {a}));

    return qc;
}

// CS†^(1) - T-depth-1 controlled-S-dagger gate using one clean ancilla
QCircuit decompose_CSDG_Clifford_T_depth1(const Qubit &c, const Qubit &t, const Qubit &a) {
    auto qc = QCircuit{};
    qc.setQubits({c, t, a});

    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {a}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c}, {t}));
    // T-layer (all conjugated relative to CS^(1))
    qc.addQGate(QGate(GateType::T,   1, {c}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {a}));
    // Uncompute
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {a}));

    return qc;
}

QCircuit decompose_CSDG_Clifford_T(const Qubit &c, const Qubit &t) {
    // Decompose controlled-S-dagger gate to Clifford+T
    auto qc = QCircuit{};
    qc.setQubits({c, t});

    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {c}));

    return qc;
}

QCircuit decompose_CZ_Clifford_T(const Qubit &c, const Qubit &t) {
    // Decompose controlled-Z gate to Clifford+T primitives
    auto qc = QCircuit{};
    qc.setQubits({c, t});

    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c}, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));

    return qc;
}

QCircuit decompose_SRCCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});
    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    } else {
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}

QCircuit decompose_SSRCCX_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});
    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
    } else {
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}

// C3(iX)^(8) - T-depth-8 3-control relative phase Toffoli gate
QCircuit decompose_RC3X_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &c3, const Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, c3, t});

    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));

    return inverse ? qc : qc.inverse();
}

// C3(iX)^(4) - T-depth-4 3-control relative phase Toffoli gate
QCircuit decompose_RC3X_Clifford_T_depth4(const Qubit &c1, const Qubit &c2, const Qubit &c3, const Qubit &t, bool is_inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, c3, t});

    qc.addQGate(QGate(GateType::H,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c3}));
    qc.addQGate(QGate(GateType::TDG, 1, {c3}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c3}));
    qc.addQGate(QGate(GateType::H,   1, {t}));

    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c2}));
    qc.addQGate(QGate(GateType::T,   1, {c1}));
    qc.addQGate(QGate(GateType::T,   1, {c2}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c1}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t}, {c2}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));

    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::H,   1, {t}));

    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c3}, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c3}));
    qc.addQGate(QGate(GateType::TDG, 1, {c3}));
    qc.addQGate(QGate(GateType::T,   1, {t}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){t},  {c3}));
    qc.addQGate(QGate(GateType::CX,  2, (ControlSet){c3}, {t}));
    qc.addQGate(QGate(GateType::H,   1, {t}));

    return is_inverse ? qc : qc.inverse();
}

QCircuit decompose_SRC3X_Clifford_T(const Qubit &c1, const Qubit &c2, const Qubit &c3, const Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, c3, t});

    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    } else {
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}


// Intermediate decomposition of CnX Toffoli gates (MCX) into RCCX, CCX, and X.
// Decomposes an n-controlled X using one clean ancilla and a linear RCCX ladder.
// Uses only RCCX/CCX/X gates: compute a conditional AND, flip the target once, then uncompute.
QCircuit decompose_CNX_Linear_RCCX(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, Qubit ancilla, bool is_dirty) {
    auto qc_comp = QCircuit{};
    auto qc_uncomp = QCircuit{};
    auto n = controls.size();

    // Validation
    if (n < 3) {
        throw QcoreException(
            "[decompose_CNX_Linear_RCCX] Requires at least 3 controls, got " +
            std::to_string(n)
        );
    }

    // Set qubits for the circuit
    QubitSet all_qubits = controls;
    all_qubits.push_back(ancilla);
    all_qubits.push_back(target);
    qc_comp.setQubits(all_qubits);
    qc_uncomp.setQubits(all_qubits);

    auto addRCCX = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit t) {
        // circ.addQGate(QGate(GateType::RCCX, 3, (ControlSet){c1, c2}, {t}));
	    circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {c1,c2}, t, {}, {}));
    };
    auto addRCCX_inv = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit t) {
        // circ.addQGate(QGate(GateType::RCCX, 3, (ControlSet){c1, c2}, {t}));
		circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(true), {c1,c2}, t, {}, {}));
    };
    auto addCCX = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit t) {
        // circ.addQGate(QGate(GateType::CCX, 3, (ControlSet){c1, c2}, {t}));
	    circ.addQCircuit(decompose_resolve_Toffoli(config, {c1,c2}, t, {}, {}));
    };
    auto addX = [](QCircuit &circ, Qubit t) {
        circ.addQGate(QGate(GateType::X, 1, {t}));
    };

    // Special case: C3X is just 2 RCCX + 1 CCX.
    if (n == 3) {
        addRCCX(qc_comp, controls[0], controls[1], ancilla);
        addCCX(qc_comp, controls[2], ancilla, target);
        addRCCX_inv(qc_comp, controls[0], controls[1], ancilla);
		if (is_dirty) {
			addCCX(qc_comp, controls[2], ancilla, target);
		}
        return qc_comp;
    }

    // Build the conditionally-clean ancilla ladder.
    struct Op {
        size_t target;
        size_t c1;
        size_t c2;
    };

    std::vector<Op> ops;
    const size_t total = n + 1; // 0 = ancilla, 1..n = controls[0..n-1]
    std::vector<bool> marked(total, false);
    marked[0] = true;

    while (ops.size() < n - 2) {
        int target = -1;
        for (int idx = static_cast<int>(total) - 1; idx >= 0; --idx) {
            if (!marked[idx]) {
                continue;
            }
            size_t unmarked_right = 0;
            for (size_t j = static_cast<size_t>(idx) + 1; j < total; ++j) {
                if (!marked[j]) {
                    ++unmarked_right;
                }
            }
            if (unmarked_right >= 2) {
                target = idx;
                break;
            }
        }
        if (target < 0) {
            throw QcoreException(
                "[decompose_CNX_Linear_RCCX] Unable to build clean-ancilla ladder"
            );
        }

        int c1 = -1;
        int c2 = -1;
        for (size_t j = static_cast<size_t>(target) + 1; j < total; ++j) {
            if (marked[j]) {
                continue;
            }
            if (c1 < 0) {
                c1 = static_cast<int>(j);
            } else {
                c2 = static_cast<int>(j);
                break;
            }
        }
        if (c1 < 0 || c2 < 0) {
            throw QcoreException(
                "[decompose_CNX_Linear_RCCX] Failed to select control pair"
            );
        }

        ops.push_back(Op{static_cast<size_t>(target), static_cast<size_t>(c1), static_cast<size_t>(c2)});
        marked[static_cast<size_t>(target)] = false;
        marked[static_cast<size_t>(c1)] = true;
        marked[static_cast<size_t>(c2)] = true;
    }

    auto map_idx = [&](size_t idx) -> Qubit {
        return idx == 0 ? ancilla : controls[idx - 1];
    };

    QCircuit compute_phase;
    for (const auto &op : ops) {
        Qubit t = map_idx(op.target);
        Qubit c1 = map_idx(op.c1);
        Qubit c2 = map_idx(op.c2);
		if (t == ancilla)
			continue;

        addRCCX(compute_phase, c1, c2, t);
        if (op.target != 0) {
            // Conditionally-clean targets are flipped after the RCCX.
            addX(compute_phase, t);
        }
    }

	addRCCX(qc_comp, map_idx(ops.front().c1), map_idx(ops.front().c2), map_idx(ops.front().target));
    qc_comp.addQCircuit(compute_phase);

    Qubit final_control = map_idx(ops.back().target);
    addCCX(qc_comp, final_control, ancilla, target);

    qc_uncomp.addQCircuit(compute_phase.inverse());
    addRCCX_inv(qc_uncomp, map_idx(ops.front().c1), map_idx(ops.front().c2), map_idx(ops.front().target));
    if (is_dirty) {
        qc_comp.addQCircuit(qc_uncomp);
        qc_comp.addQCircuit(compute_phase);
        addCCX(qc_comp, final_control, ancilla, target);
        qc_comp.addQCircuit(compute_phase.inverse());
        return qc_comp;
    } else {
        if (config.allow_target_error) {
            if (config.is_inverse) {
                return qc_uncomp;
            } else {
                return qc_comp;
            }
        }
        qc_comp.addQCircuit(qc_uncomp);
        return qc_comp;
    }
}



QCircuit decompose_CNX_Linear_RC3X(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, Qubit ancilla) {
    auto qc = QCircuit{};
    auto n = controls.size();

    // Validation - RC3X approach requires at least 3 controls
    if (n < 3) {
        throw QcoreException(
            "[decompose_CNX_Linear_RC3X] Requires at least 3 controls, got " +
            std::to_string(n)
        );
    }

    // Set qubits for the circuit
    QubitSet all_qubits = controls;
    all_qubits.push_back(ancilla);
    all_qubits.push_back(target);
    qc.setQubits(all_qubits);

    // Helper lambdas
    auto addRC3X = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit c3, Qubit t) {
        // circ.addQGate(QGate(GateType::RC3X, 4, (ControlSet){c1, c2, c3}, {t}));
		circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {c1,c2,c3}, t, {}, {}));
    };
    auto addRCCX = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit t) {
        // circ.addQGate(QGate(GateType::RCCX, 3, (ControlSet){c1, c2}, {t}));
		circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {c1,c2}, t, {}, {}));
    };
    auto addCCX = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit t) {
        // circ.addQGate(QGate(GateType::CCX, 3, (ControlSet){c1, c2}, {t}));
		circ.addQCircuit(decompose_resolve_Toffoli(config, {c1,c2}, t, {}, {}));
    };
    auto addCX = [](QCircuit &circ, Qubit c, Qubit t) {
        circ.addQGate(QGate(GateType::CX, 2, (ControlSet){c}, {t}));
    };
    auto addX = [](QCircuit &circ, Qubit t) {
        circ.addQGate(QGate(GateType::X, 1, {t}));
    };

    // Base RC3X gate (always present), uncomputed with its inverse
    QCircuit base_phase;
    addRC3X(base_phase, controls[0], controls[1], controls[2], ancilla);

    // Special case: n == 3 (C3X gate)
    if (n == 3) {
        qc.addQCircuit(base_phase);
        addCX(qc, ancilla, target);
        qc.addQCircuit(base_phase.inverse());
        return qc;
    }

    // ========== Algorithm ==========

    // Step 1: Add base RC3X gate (always present)
    qc.addQCircuit(base_phase);

    // Special case n=4: Direct CCX only
    if (n == 4) {
        addCCX(qc, controls[3], ancilla, target);
        qc.addQCircuit(base_phase.inverse());
        return qc;
    }

    // Step 2: Build forward phase using general algorithm
    QCircuit forward_phase;
    int n_mod_6 = n % 6;
    int ctrl_idx = 3;  // Start consuming from q[3]

    // ===== Small cases: n = 5, 6, 7, 8 =====
    if (n == 5) {
        addRCCX(forward_phase, controls[3], controls[4], controls[0]);
        addX(forward_phase, controls[0]);
    }
    else if (n == 6) {
        addRC3X(forward_phase, controls[3], controls[4], controls[5], controls[0]);
        addX(forward_phase, controls[0]);
    }
    else if (n == 7) {
        addRCCX(forward_phase, controls[3], controls[4], controls[1]);
        addX(forward_phase, controls[1]);
        addRCCX(forward_phase, controls[5], controls[6], controls[2]);
        addX(forward_phase, controls[2]);
        addRCCX(forward_phase, controls[1], controls[2], controls[0]);
        addX(forward_phase, controls[0]);
    }
    else if (n == 8) {
        addRCCX(forward_phase, controls[3], controls[4], controls[1]);
        addX(forward_phase, controls[1]);
        addRCCX(forward_phase, controls[5], controls[6], controls[2]);
        addX(forward_phase, controls[2]);
        addRC3X(forward_phase, controls[1], controls[2], controls[7], controls[0]);
        addX(forward_phase, controls[0]);
    }
    // ===== General case: n >= 9 =====
    else {
        // ENCODING PHASE: Always start with 2× rc3x for first 6 controls
        addRC3X(forward_phase, controls[3], controls[4], controls[5], controls[1]);
        addX(forward_phase, controls[1]);
        addRC3X(forward_phase, controls[6], controls[7], controls[8], controls[2]);
        addX(forward_phase, controls[2]);
        ctrl_idx = 9;

        // Track intermediate qubits created from batches
        std::vector<int> batch_intermediates;  // Intermediates beyond q[1], q[2]
        int storage_idx = 4;  // Next intermediates go to q[4], q[5], q[7], q[8], ...

        // Process remaining controls in batches of 6
        int remaining = n - ctrl_idx;

        while (remaining >= 6) {
            // Encode next 6 controls with 2× rc3x
            addRC3X(forward_phase, controls[ctrl_idx], controls[ctrl_idx+1], controls[ctrl_idx+2],
                    controls[storage_idx]);
            addX(forward_phase, controls[storage_idx]);
            addRC3X(forward_phase, controls[ctrl_idx+3], controls[ctrl_idx+4], controls[ctrl_idx+5],
                    controls[storage_idx+1]);
            addX(forward_phase, controls[storage_idx+1]);

            batch_intermediates.push_back(storage_idx);
            batch_intermediates.push_back(storage_idx + 1);

            ctrl_idx += 6;
            storage_idx += 3;  // Skip q[6], q[9], ... for staging
            remaining = n - ctrl_idx;
        }

        // COMBINING PHASE: Handle final remaining (0-5) and combine all intermediates
        int staging = storage_idx - 1;  // Use skipped qubit for staging

        if (remaining == 0) {
            // Original n ≡ 3 (mod 6): all remaining controls were consumed as triplets.
            // Reduce batch pairs from right to left, always using the reserved gap qubit
            // immediately before a pair as the next staging wire: q[3], q[6], q[9], ...
            if (batch_intermediates.empty()) {
                addRCCX(forward_phase, controls[1], controls[2], controls[0]);
                addX(forward_phase, controls[0]);
            } else {
                int last_pair = static_cast<int>(batch_intermediates.size()) - 2;
                int current_summary = batch_intermediates[last_pair] - 1;

                // Collapse the rightmost pair with RCCX.
                addRCCX(forward_phase, controls[batch_intermediates[last_pair]],
                        controls[batch_intermediates[last_pair + 1]],
                        controls[current_summary]);
                addX(forward_phase, controls[current_summary]);

                // Fold earlier pairs into the running summary with RC3X.
                for (int pair = last_pair - 2; pair >= 0; pair -= 2) {
                    int next_summary = batch_intermediates[pair] - 1;
                    addRC3X(forward_phase, controls[batch_intermediates[pair]],
                            controls[batch_intermediates[pair + 1]],
                            controls[current_summary], controls[next_summary]);
                    addX(forward_phase, controls[next_summary]);
                    current_summary = next_summary;
                }

                // Finish by combining q[1], q[2], and the reduced summary into q[0].
                addRC3X(forward_phase, controls[1], controls[2],
                        controls[current_summary], controls[0]);
                addX(forward_phase, controls[0]);
            }
        }
        else if (remaining == 1) {
            // Original n ≡ 4 (mod 6): one leftover control after triplet encoding.
            // Combine it with the rightmost pair summary using RC3X, then fold
            // right-to-left through reserved gap wires q[3], q[6], q[9], ...
            if (batch_intermediates.empty()) {
                addRC3X(forward_phase, controls[ctrl_idx], controls[1], controls[2], controls[0]);
                addX(forward_phase, controls[0]);
            } else {
                int last_pair = static_cast<int>(batch_intermediates.size()) - 2;
                int current_summary = batch_intermediates[last_pair] - 1;

                // First fold: leftover + rightmost pair -> gap before that pair.
                addRC3X(forward_phase, controls[ctrl_idx],
                        controls[batch_intermediates[last_pair]],
                        controls[batch_intermediates[last_pair + 1]],
                        controls[current_summary]);
                addX(forward_phase, controls[current_summary]);

                // Continue folding earlier pairs into the running summary.
                for (int pair = last_pair - 2; pair >= 0; pair -= 2) {
                    int next_summary = batch_intermediates[pair] - 1;
                    addRC3X(forward_phase, controls[batch_intermediates[pair]],
                            controls[batch_intermediates[pair + 1]],
                            controls[current_summary], controls[next_summary]);
                    addX(forward_phase, controls[next_summary]);
                    current_summary = next_summary;
                }

                // Final fold with the base pair q[1], q[2] into q[0].
                addRC3X(forward_phase, controls[1], controls[2], controls[current_summary], controls[0]);
                addX(forward_phase, controls[0]);
            }
        }
        else if (remaining == 2) {
            // Original n ≡ 5 (mod 6): two leftover controls after triplet encoding.
            // First form a pair summary with RCCX, then fold batch pairs right-to-left
            // through reserved gap wires q[3], q[6], q[9], ...
            if (batch_intermediates.empty()) {
                addRCCX(forward_phase, controls[ctrl_idx], controls[ctrl_idx+1], controls[3]);
                addX(forward_phase, controls[3]);
                addRC3X(forward_phase, controls[1], controls[2], controls[3], controls[0]);
                addX(forward_phase, controls[0]);
            } else {
                int current_summary = staging;

                // Pair summary for the two leftover controls.
                addRCCX(forward_phase, controls[ctrl_idx], controls[ctrl_idx+1], controls[current_summary]);
                addX(forward_phase, controls[current_summary]);

                // Fold batch pairs into the running summary from right to left.
                for (int pair = static_cast<int>(batch_intermediates.size()) - 2; pair >= 0; pair -= 2) {
                    int next_summary = batch_intermediates[pair] - 1;
                    addRC3X(forward_phase, controls[batch_intermediates[pair]],
                            controls[batch_intermediates[pair + 1]],
                            controls[current_summary], controls[next_summary]);
                    addX(forward_phase, controls[next_summary]);
                    current_summary = next_summary;
                }

                addRC3X(forward_phase, controls[1], controls[2], controls[current_summary], controls[0]);
                addX(forward_phase, controls[0]);
            }
        }
        else if (remaining == 3) {
            // n ≡ 3 (mod 6): rc3x for triplet
            if (batch_intermediates.empty()) {
                addRC3X(forward_phase, controls[ctrl_idx], controls[ctrl_idx+1], controls[ctrl_idx+2],
                        controls[3]);
                addX(forward_phase, controls[3]);
                addRC3X(forward_phase, controls[1], controls[2], controls[3], controls[0]);
                addX(forward_phase, controls[0]);
            } else {
                // pairs right-to-left via gap wires q[3], q[6], q[9], ...
                addRC3X(forward_phase, controls[ctrl_idx], controls[ctrl_idx+1], controls[ctrl_idx+2],
                        controls[staging]);
                addX(forward_phase, controls[staging]);

                int current_summary = staging;
                for (int pair = static_cast<int>(batch_intermediates.size()) - 2; pair >= 0; pair -= 2) {
                    int next_summary = batch_intermediates[pair] - 1;
                    addRC3X(forward_phase, controls[batch_intermediates[pair]],
                            controls[batch_intermediates[pair + 1]],
                            controls[current_summary], controls[next_summary]);
                    addX(forward_phase, controls[next_summary]);
                    current_summary = next_summary;
                }

                addRC3X(forward_phase, controls[1], controls[2], controls[current_summary], controls[0]);
                addX(forward_phase, controls[0]);
            }
        }
        else if (remaining == 4) {
            // n ≡ 1 (mod 6): 2× rccx pairs
            addRCCX(forward_phase, controls[ctrl_idx], controls[ctrl_idx+1], controls[storage_idx]);
            addX(forward_phase, controls[storage_idx]);
            addRCCX(forward_phase, controls[ctrl_idx+2], controls[ctrl_idx+3], controls[storage_idx+1]);
            addX(forward_phase, controls[storage_idx+1]);
            addRCCX(forward_phase, controls[storage_idx], controls[storage_idx+1], controls[staging]);
            addX(forward_phase, controls[staging]);

            if (batch_intermediates.empty()) {
                addRC3X(forward_phase, controls[1], controls[2], controls[staging], controls[0]);
                addX(forward_phase, controls[0]);
            } else {
                // Collapse earlier batch pairs from right to left using the reserved gap wires
                int current_summary = staging;
                for (int pair = static_cast<int>(batch_intermediates.size()) - 2; pair >= 0; pair -= 2) {
                    int next_summary = batch_intermediates[pair] - 1;
                    addRC3X(forward_phase, controls[batch_intermediates[pair]],
                            controls[batch_intermediates[pair + 1]],
                            controls[current_summary], controls[next_summary]);
                    addX(forward_phase, controls[next_summary]);
                    current_summary = next_summary;
                }

                addRC3X(forward_phase, controls[1], controls[2], controls[current_summary], controls[0]);
                addX(forward_phase, controls[0]);
            }
        }
        else if (remaining == 5) {
            // n ≡ 2 (mod 6): 2× rccx + 1× rc3x with leftover
            addRCCX(forward_phase, controls[ctrl_idx], controls[ctrl_idx+1], controls[storage_idx]);
            addX(forward_phase, controls[storage_idx]);
            addRCCX(forward_phase, controls[ctrl_idx+2], controls[ctrl_idx+3], controls[storage_idx+1]);
            addX(forward_phase, controls[storage_idx+1]);
            addRC3X(forward_phase, controls[ctrl_idx+4], controls[storage_idx], controls[storage_idx+1],
                    controls[staging]);
            addX(forward_phase, controls[staging]);

            if (batch_intermediates.empty()) {
                // n=14: No batches, combine staging with q[1,2] into q[0]
                addRC3X(forward_phase, controls[staging], controls[1], controls[2], controls[0]);
                addX(forward_phase, controls[0]);
            } else {
                // Fold earlier batch pairs right-to-left using reserved gap wires
                // q[3], q[6], ... so n=20, 26, ... stays regular.
                int current_summary = staging;
                for (int pair = static_cast<int>(batch_intermediates.size()) - 2; pair >= 0; pair -= 2) {
                    int next_summary = batch_intermediates[pair] - 1;
                    addRC3X(forward_phase, controls[batch_intermediates[pair]],
                            controls[batch_intermediates[pair + 1]],
                            controls[current_summary], controls[next_summary]);
                    addX(forward_phase, controls[next_summary]);
                    current_summary = next_summary;
                }

                addRC3X(forward_phase, controls[1], controls[2], controls[current_summary], controls[0]);
                addX(forward_phase, controls[0]);
            }
        }
    }

    // Step 3: Add forward phase to circuit
    qc.addQCircuit(forward_phase);

    // Step 4: Apply CCX to target using q[0] and base ancilla
    addCCX(qc, controls[0], ancilla, target);

    // Step 5: Add backward phase (exact inverse of forward)
    qc.addQCircuit(forward_phase.inverse());

    // Step 6: Uncompute base RC3X
    qc.addQCircuit(base_phase.inverse());

    return qc;
}

// Decomposes an n-controlled X using ceil((n-2)/2) clean ancillas and a tree of RC3X gates:
// triples are combined level by level in parallel, so the T-depth grows with log(n).
QCircuit decompose_CNX_Tree_RC3X(const DecompositionConfiguration &config, const QubitSet &controls,
                                 Qubit target, const QubitSet &clean_ancilla) {
    auto qc = QCircuit{};
    auto n = controls.size();

    // Validation
    if (n < 3) {
        throw QcoreException(
            "[decompose_CNX_Tree_RC3X] Requires at least 3 controls, got " +
            std::to_string(n)
        );
    }
    const size_t ancilla_need = (n - 1) / 2;  // ceil((n-2)/2)
    if (clean_ancilla.size() < ancilla_need) {
        throw QcoreException(
            "[decompose_CNX_Tree_RC3X] Requires " + std::to_string(ancilla_need) +
            " clean ancilla, got " + std::to_string(clean_ancilla.size())
        );
    }

    // Set qubits for the circuit
    QubitSet all_qubits = controls;
    all_qubits.insert(all_qubits.end(), clean_ancilla.begin(), clean_ancilla.begin() + ancilla_need);
    all_qubits.push_back(target);
    qc.setQubits(all_qubits);

    // Helper lambdas
    auto addRC3X = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit c3, Qubit t) {
        circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {c1,c2,c3}, t, {}, {}));
    };
    auto addRCCX = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit t) {
        circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {c1,c2}, t, {}, {}));
    };
    auto addCCX = [config](QCircuit &circ, Qubit c1, Qubit c2, Qubit t) {
        circ.addQCircuit(decompose_resolve_Toffoli(config, {c1,c2}, t, {}, {}));
    };

    // Compute phase: combine the layer into fresh clean ancillas, triples with RC3X and at most
    // one pair with RCCX, until two qubits hold the AND of all controls.
    QCircuit compute_phase;
    QubitSet layer = controls;
    size_t next_ancilla = 0;
    while (layer.size() > 2) {
        size_t reduce = layer.size() - 2;
        QubitSet next_layer;
        size_t idx = 0;
        while (reduce >= 2 && idx + 3 <= layer.size()) {
            Qubit a = clean_ancilla[next_ancilla++];
            addRC3X(compute_phase, layer[idx], layer[idx + 1], layer[idx + 2], a);
            next_layer.push_back(a);
            idx += 3;
            reduce -= 2;
        }
        if (reduce == 1 && idx + 2 <= layer.size()) {
            Qubit a = clean_ancilla[next_ancilla++];
            addRCCX(compute_phase, layer[idx], layer[idx + 1], a);
            next_layer.push_back(a);
            idx += 2;
        }
        // Leftover qubits pass through to the next level
        next_layer.insert(next_layer.end(), layer.begin() + idx, layer.end());
        layer = next_layer;
    }

    qc.addQCircuit(compute_phase);

    // The qubit computed last goes second, where the CCX has the fewest T gates
    addCCX(qc, layer[1], layer[0], target);

    qc.addQCircuit(compute_phase.inverse());

    return qc;
}

QCircuit decompose_CNX_Logarithmic_Mixed(const DecompositionConfiguration &config, const QubitSet &controls,
                                         Qubit target, Qubit ancilla1, Qubit ancilla2, std::set<int> rc3x_steps) {
    auto qc = QCircuit{};
    auto n = controls.size();

    if (ancilla1 == ancilla2) {
        throw QcoreException("[decompose_CNX_Logarithmic_Mixed] Requires 2 distinct ancilla.");
    }
    if (n < 3) {
        throw QcoreException("[decompose_CNX_Logarithmic_Mixed] Requires at least 3 controls, got " + std::to_string(n));
    }

    // Set qubits for the circuit
    QubitSet all_qubits = controls;
    all_qubits.push_back(ancilla1);
    all_qubits.push_back(ancilla2);
    all_qubits.push_back(target);
    qc.setQubits(all_qubits);

    auto addH = [](QCircuit &circ, Qubit t) { circ.addQGate(QGate(GateType::H, 1, {t})); };
    auto addX = [](QCircuit &circ, Qubit t) { circ.addQGate(QGate(GateType::X, 1, {t})); };
    auto addRCCX = [&config](QCircuit &circ, QubitSet s, Qubit t, Qubit a) {
        circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), s, t, {a}, {}));
        // circ.addQCircuit(decompose_CNX_Linear_RCCX(s, t, a));
    };
    auto addR3X = [&config](QCircuit &circ, QubitSet s, Qubit t, Qubit a) {
        circ.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), s, t, {a}, {}));
        // circ.addQCircuit(decompose_CNX_Linear_RC3X(s, t, a));
    };

    struct PumpStep {
        bool collect = false;
        std::optional<Qubit> ancilla = {};
        bool ladder = false;
        int size = 2;
    };

    std::vector<std::vector<PumpStep>> steps;
    auto add_step = [&](int n_step, int n_clean2_a0, int n_col2, int n_clean2, int n_explicit_ladder, int n_col2_target) {
        auto size = rc3x_steps.contains(n_step) ? 3 : 2;
        const auto clean2_a0 = PumpStep{.ancilla = ancilla1, .size = size};
        const auto col2 = PumpStep{.size = size};
        const auto clean2 = PumpStep{.collect = true, .size = size};
        const auto explicit_ladder = PumpStep{.collect = true, .ancilla = ancilla2, .ladder = true, .size = size};
        const auto col2_target = PumpStep{.collect = true, .ancilla = target, .size = size};

        std::vector<PumpStep> step;
        while (n_clean2_a0-- > 0) {
            step.push_back(clean2_a0);
        }
        while (n_col2-- > 0) {
            step.push_back(col2);
        }
        while (n_clean2-- > 0) {
            step.push_back(clean2);
        }
        while (n_explicit_ladder-- > 0) {
            step.push_back(explicit_ladder);
        }
        steps.push_back(step);
    };

    regsize_t caught = 4;
    regsize_t step = 0;
    add_step(step++, 1, 0, 0, 0, 0);
    add_step(step++, 0, 1, 0, 0, 0);
    while (caught <= controls.size()) {
        auto real_step = step - 1;
        add_step(step++, 0, std::pow(2, real_step - 1), std::pow(2, real_step), 0, 0);
        caught += pow(2, real_step);
    }
    auto step_max = step;
    while (step_max > 1) {
        auto real_step = step_max - 1;
        step_max--;
        add_step(step++, 0, std::pow(2, real_step - 1), 0, 0, 0);
    }
    add_step(step++, 0, 0, 0, 1, 0);
    add_step(step++, 0, 0, 0, 0, 1);

    QubitSet clean;
    QubitSet used;
    QubitSet unused = controls;
    QubitSet anc_used;
    int i = 0;
    for (auto step : steps) {
        QubitSet next_clean;
        QubitSet next_used;
        for (auto layer : step) {
            if (layer.ladder) {
                auto sources = used;
                sources.insert(sources.end(), unused.begin(), unused.end());

                auto target = sources.front();
                sources.erase(sources.begin());

                if (layer.size == 2) {
                    addRCCX(qc, sources, target, *layer.ancilla);
                } else if (layer.size == 3) {
                    addR3X(qc, sources, target, *layer.ancilla);
                } else {
				    // TODO: Wee need to support decomp into the first half if asDecomputationPair is set!
                    throw std::logic_error("invalid layer size for ladder");
                }

                used.clear();
                unused.clear();
                used.push_back(target);
                used.push_back(*layer.ancilla);
                anc_used.push_back(*layer.ancilla);
                continue;
            }

            QubitSet sources = {};
            if (layer.collect) {
                if (!layer.ancilla) {
                    auto start_idx = (used.size() > layer.size) ? (used.end() - layer.size) : used.begin();
                    sources.insert(sources.end(), start_idx, used.end());
                } else {
                    auto end_idx = (used.size() > layer.size) ? (used.begin() + layer.size) : used.end();
                    sources.insert(sources.end(), used.begin(), end_idx);
                }
            } else {
                auto end_idx = (unused.size() > layer.size) ? (unused.begin() + layer.size) : unused.end();
                sources.insert(sources.end(), unused.begin(), end_idx);
            }

            Qubit target = -1;
            if (layer.ancilla) {
                target = *layer.ancilla;
            } else {
                if (layer.collect) {
					// TODO what is the meaninig of '<' here? -> We should be comparing positions in the source list, not actual values!!
                    std::vector<Qubit> potential_targets;
                    std::copy_if(clean.begin(), clean.end(), std::back_inserter(potential_targets),
                                 [&sources](auto candidate) {
                                     return std::all_of(sources.begin(), sources.end(),
                                                        [candidate](auto source) { return candidate < source; });
                                 });

                    while (potential_targets.empty() && sources.size() > 1) {
                        sources.erase(sources.begin());
                        std::copy_if(clean.begin(), clean.end(), std::back_inserter(potential_targets),
                                     [&sources](auto candidate) {
                                         return std::all_of(sources.begin(), sources.end(),
                                                            [candidate](auto source) { return candidate < source; });
                                     });
                    }
                    if (potential_targets.empty()) {
                        continue;
                    }

                    target = potential_targets.front();
                } else {
                    if (clean.size() == 0) {
                        throw std::logic_error("not enough clean qubits");
                    }
                    target = clean.back();
                }
            }

            if (sources.size() <= 1) {
                continue;
            }

            // addH(qc, target);
            bool do_toggle = !layer.ancilla or (std::find(anc_used.begin(), anc_used.end(), target) != anc_used.end());
            if (do_toggle) {
                addX(qc, target);
            }
            qc.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), sources, target, {}, {}));
            // if (sources.size() == 2) {
            //   qc.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), QubitSet{sources[0], sources[1]}, target, {}, {}));
            //   // qc.addQCircuit(decompose_RCCX_Clifford_T_depth2(sources[0], sources[1], target)); // TODO
            // } else if (sources.size() == 3) {
            //   qc.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), QubitSet{sourcse[0], sources[1], sources[2]}, target, {}, {}));
            //   // qc.addQCircuit(decompose_RC3X_Clifford_T_depth4(sources[0], sources[1], sources[2], target)); // TODO
            // } else {
            //     throw std::logic_error("invalid number of source qubits: " + std::to_string(sources.size()));
            // }
            // addH(qc, target);

            clean.erase(std::remove_if(clean.begin(), clean.end(), [&target](auto cand) { return cand == target; }),
                        clean.end());
            used.erase(std::remove_if(used.begin(), used.end(),
                                      [&sources](auto cand) {
                                          return std::find(sources.begin(), sources.end(), cand) != sources.end();
                                      }),
                       used.end());
            unused.erase(std::remove_if(unused.begin(), unused.end(),
                                        [&sources](auto cand) {
                                            return std::find(sources.begin(), sources.end(), cand) != sources.end();
                                        }),
                         unused.end());

            anc_used.push_back(target);
            next_used.push_back(target);
            next_clean.insert(next_clean.end(), sources.begin(), sources.end());
        }
        clean.insert(clean.end(), next_clean.begin(), next_clean.end());
        used.insert(used.end(), next_used.begin(), next_used.end());
    }

    auto sources = used;
    if (sources.size() != 2) {
        throw std::logic_error("expected 2 final ancilla, got " + std::to_string(sources.size()));
    }
    auto uncomp = qc.inverse();
    // qc.addQCircuit(decompose_CCX_Clifford_T(sources[0], sources[1], target));
	qc.addQCircuit(decompose_resolve_Toffoli(config, QubitSet{sources[0], sources[1]}, target, {}, {}));
    qc.addQCircuit(uncomp);

    return qc;
}

QCircuit decompose_circuit_to_intermediate_RCCX(QCircuit &circuit, Qubit ancilla) {
	throw std::logic_error("decompose_circuit_to_clifford_t is disabled");

	#if 0 // TODO Re-enable once we have a method to handle RC3X-Gates correctly
    // Set up qubits - use original circuit's qubits plus ancilla
    regsize_t max_qubit = circuit.getQregSize();
    if (ancilla >= max_qubit) {
        max_qubit = ancilla + 1;
    }
    auto qc = QCircuit(max_qubit, circuit.getCregSize());

    // Iterate through all gates
    for (const auto& gate_ptr : circuit.getGates()) {
        QGate gate = *gate_ptr;  // Make a copy to allow non-const access
        GateType type = gate.getType();

        if (type == GateType::MCX) {
            // Multi-control X gate - decompose it
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];

            if (controls.size() >= 3) {
                // Use linear decomposition
                QCircuit decomposed = decompose_CNX_Linear_RCCX(controls, target, ancilla);
                qc.addQCircuit(decomposed);
            } else if (controls.size() == 2) {
                // It's a CCX, add directly
                qc.addQGate(QGate(GateType::CCX, 3, (ControlSet){controls[0], controls[1]}, {target}));
            } else if (controls.size() == 1) {
                // It's a CX, add directly
                qc.addQGate(QGate(GateType::CX, 2, (ControlSet){controls[0]}, {target}));
            } else {
                // Just X gate
                qc.addQGate(QGate(GateType::X, 1, {target}));
            }
        } else if (type == GateType::CCX) {
            // Standard Toffoli - keep as CCX (already in intermediate form)
            qc.addQGate(gate);
        } else {
            // All other gates - pass through unchanged
            qc.addQGate(gate);
        }
    }

    return qc;
	#endif
}

QCircuit decompose_circuit_to_clifford_t(QCircuit &circuit, Qubit ancilla) {
	throw std::logic_error("decompose_circuit_to_clifford_t is disabled");

	#if 0 // TODO Re-enable once we have a method to handle RC3X-Gates correctly

    // Step 1: Decompose MCX gates to intermediate form only if MCX gates are present
    bool has_mcx = false;
    for (const auto& gate_ptr : circuit.getGates()) {
        if ((*gate_ptr).getType() == GateType::MCX) {
            has_mcx = true;
            break;
        }
    }

    QCircuit intermediate;
    if (has_mcx) {
        intermediate = decompose_circuit_to_intermediate_RCCX(circuit, ancilla);
    }
    auto& source = has_mcx ? intermediate : circuit;

    // Step 2: Decompose all remaining multi-qubit gates to Clifford+T
    regsize_t max_qubit = source.getQregSize();
    auto qc = QCircuit(max_qubit, source.getCregSize());

    for (const auto& gate_ptr : source.getGates()) {
        QGate gate = *gate_ptr;
        GateType type = gate.getType();

        if (type == GateType::CCX) {
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];
            qc.addQCircuit(decompose_CCX_Clifford_T(controls[0], controls[1], target));
        } else if (type == GateType::RCCX) {
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];
            qc.addQCircuit(decompose_RCCX_Clifford_T(controls[0], controls[1], target));
        } else if (type == GateType::RC3X) {
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];
            qc.addQCircuit(decompose_RC3X_Clifford_T(controls[0], controls[1], controls[2], target));
        } else if (type == GateType::CCIZ) {
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];
            QCircuit cciz_decomposed = decompose_CCIZ_Clifford_T(controls[0], controls[1], target);

            // If the original CCIZ gate is conditional (from dynamic circuit),
            // make all decomposed gates conditional with the same condition
            if (gate.isClassical()) {
                ClassicalCondition condition = gate.getClassical();
                for (auto& decomposed_gate_ptr : cciz_decomposed.getGates()) {
                    decomposed_gate_ptr->setClassical(condition);
                }
            }

            qc.addQCircuit(cciz_decomposed);
        } else if (type == GateType::CSDG) {
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];
            QCircuit csdg_decomposed = decompose_CSDG_Clifford_T(controls[0], target);

            // If the original CSDG gate is conditional (from dynamic circuit),
            // make all decomposed gates conditional with the same condition
            if (gate.isClassical()) {
                ClassicalCondition condition = gate.getClassical();
                for (auto& decomposed_gate_ptr : csdg_decomposed.getGates()) {
                    decomposed_gate_ptr->setClassical(condition);
                }
            }

            qc.addQCircuit(csdg_decomposed);
        } else if (type == GateType::CZ) {
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];
            QCircuit cz_decomposed = decompose_CZ_Clifford_T(controls[0], target);

            // If the original CZ gate is conditional (from dynamic circuit),
            // make all decomposed gates conditional with the same condition
            if (gate.isClassical()) {
                ClassicalCondition condition = gate.getClassical();
                for (auto& decomposed_gate_ptr : cz_decomposed.getGates()) {
                    decomposed_gate_ptr->setClassical(condition);
                }
            }

            qc.addQCircuit(cz_decomposed);
        } else {
            qc.addQGate(gate);
        }
    }

    return qc;
	#endif
}

QCircuit decompose_circuit_to_intermediate_RC3X(QCircuit &circuit, Qubit ancilla) {
	throw std::logic_error("decompose_circuit_to_intermediate_RC3X is disabled");
    // Set up qubits - use original circuit's qubits plus ancilla
#if 0
    regsize_t max_qubit = circuit.getQregSize();
    if (ancilla >= max_qubit) {
        max_qubit = ancilla + 1;
    }
    auto qc = QCircuit(max_qubit, circuit.getCregSize());

    // Iterate through all gates
    for (const auto& gate_ptr : circuit.getGates()) {
        QGate gate = *gate_ptr;
        GateType type = gate.getType();

        if (type == GateType::MCX) {
            QubitSet controls = gate.getControls();
            Qubit target = gate.getTargets()[0];

            if (controls.size() >= 3) {
                // Use RC3X-based linear decomposition (triplet encoding)
                QCircuit decomposed = decompose_CNX_Linear_RC3X(controls, target, ancilla);
                qc.addQCircuit(decomposed);
            } else if (controls.size() == 2) {
                qc.addQGate(QGate(GateType::CCX, 3, (ControlSet){controls[0], controls[1]}, {target}));
            } else if (controls.size() == 1) {
                qc.addQGate(QGate(GateType::CX, 2, (ControlSet){controls[0]}, {target}));
            } else {
                qc.addQGate(QGate(GateType::X, 1, {target}));
            }
        } else if (type == GateType::CCX) {
            qc.addQGate(gate);
        } else {
            qc.addQGate(gate);
        }
    }

    return qc;
#endif
}

QCircuit decompose_MCT_Clifford_T(const DecompositionConfiguration &config, const QubitSet &controls, Qubit target, const QubitSet &dirty, const QubitSet &clean) {
    auto qc = QCircuit{};
    /*printf("C=%d\nControls: { ",controls.size());
    for(auto q:controls)
        printf("%d ", q);
    printf("}, target: %d, clean: { ", target);
    for(auto q:clean)
        printf("%d ", q);
    printf("}, dirty: { ");
    for(auto q:dirty)
        printf("%d ", q);
    printf("}\n");*/
    if (controls.size() == 2) {
        qc = decompose_CCX_Clifford_T(controls[0], controls[1], target);
    } 
    else {
        qcore::regsize_t ancilla_need = ((controls.size() - 2) / 2) + ((controls.size() - 2) % 2);
        if (clean.size() + dirty.size() < ancilla_need) {
            throw QcoreException("[decompose_MCT_Clifford_T] decomposition error msg: ancilla required (" + std::to_string(ancilla_need) + ") is higher than available (" + std::to_string(clean.size() + dirty.size()) + ").");
        }
  
        QubitSet quibts = mergeQubits({controls, {target}, dirty, clean});
        qc.setQubits(quibts);
        
  
        //decompose using clean ancilla 
        auto p_caq = controls[0];
        auto controls_covered = 1;
        if (!clean.empty()){
            p_caq = clean[0];
            controls_covered = 0;
        }
        for (auto & caq: clean){
            if (controls.size() == 3 && controls_covered == 0){
                // qc.addQCircuit(decompose_RCCX_Clifford_T(controls[0], controls[1], caq));
				qc.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {controls[0], controls[1]}, {caq}, {}, {}));
                controls_covered += 2; 
            }
            else if (controls.size() > 3 && controls_covered == 0){
				// qc.addQCircuit(decompose_RC3X_Clifford_T(controls[0], controls[1], controls[2], caq, false));
				qc.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {controls[0], controls[1], controls[2]}, {caq}, {}, {}));
                controls_covered += 3;
            }
            else if ((controls.size() - controls_covered) == 2) {
                // qc.addQCircuit(decompose_RCCX_Clifford_T(p_caq, controls[controls_covered], caq));
				qc.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {p_caq, controls[controls_covered]}, {caq}, {}, {}));
                controls_covered += 1;
            }
            else if ((controls.size() - controls_covered) > 2) {
			    // qc.addQCircuit(decompose_RC3X_Clifford_T(p_caq, controls[controls_covered], controls[controls_covered + 1], caq, false));
				qc.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {p_caq, controls[controls_covered], controls[controls_covered + 1]}, {caq}, {}, {}));
                controls_covered += 2;
            }
            
            p_caq = caq;
            
            if ((controls.size() - controls_covered) == 1){
                break;
            }
        }
        auto qc_temp = qc; 
        //decompose using dirty ancilla
        if ((controls.size() - controls_covered) > 1){
            auto p_daq = dirty[0];
            auto qc2 = QCircuit{};
            auto qc3 = QCircuit{};
            auto qc_temp2 = QCircuit{};
            for (auto &daq: dirty){
                if (daq == dirty[0]){
                    // qc3.addQCircuit(decompose_SSRCCX_Clifford_T(controls[controls_covered], daq, target));
					qc2.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {controls[controls_covered], daq}, {target}, {}, {}));
                    controls_covered += 1; 
                }
                else if ((controls.size() - controls_covered) > 3) {
                    // qc2.addQCircuit(decompose_SRC3X_Clifford_T(daq, controls[controls_covered], controls[controls_covered + 1], p_daq));
					qc2.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {daq, controls[controls_covered], controls[controls_covered + 1]}, {p_daq}, {}, {}));
                    controls_covered += 2;
                }
                else if ((controls.size() - controls_covered) > 2) {
                    // qc2.addQCircuit(decompose_SRCCX_Clifford_T(daq, controls[controls_covered], p_daq));
					qc2.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {daq, controls[controls_covered]}, {p_daq}, {}, {}));
                    controls_covered += 1;
                }
                p_daq = daq;  
                if ((controls.size() - controls_covered) <= 2){
                    break;
                }    
                 
            }
            if ((controls.size() - controls_covered) == 1 ){
                qc_temp2 = qc2;
                // qc2.addQCircuit(decompose_RCCX_Clifford_T(p_caq, controls[controls_covered], p_daq));
				qc2.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {p_caq, controls[controls_covered]}, {p_daq}, {}, {}));
            }
            else if ((controls.size() - controls_covered) == 2) {
                qc_temp2 = qc2;
                // qc2.addQCircuit(decompose_RC3X_Clifford_T(p_caq, controls[controls_covered] , controls[controls_covered + 1], p_daq, false));
				qc2.addQCircuit(decompose_resolve_Toffoli(config.withRelativePhase(false), {p_caq, controls[controls_covered], controls[controls_covered + 1]}, {p_daq}, {}, {}));
            }
            else{
                throw QcoreException("[toString] Insufficient dirty ancilla, decomposition failed.");
            }
            qc2.addQCircuit(qc_temp2.inverse());
            qc.addQCircuit(qc3);
            qc.addQCircuit(qc2);
            qc.addQCircuit(qc3.inverse());
            qc.addQCircuit(qc2.inverse());                                    
        }
        else{
            if ((controls.size() - controls_covered) == 1) {
                // qc.addQCircuit(decompose_CCX_Clifford_T(p_caq, controls[controls_covered], target));
				qc.addQCircuit(decompose_resolve_Toffoli(config, {p_caq, controls[controls_covered]}, {target}, {}, {}));
            }
            else{
                throw QcoreException("[toString] Insufficient clean ancilla, decomposition failed.");
            }
        }
        qc.addQCircuit(qc_temp.inverse());
    }
    return qc;
}

}  // namespace qcore

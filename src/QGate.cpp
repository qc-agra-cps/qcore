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
 *  @file   QGate.cpp
 *  @brief  Instance Description for Quantum Gate
 *  @author Abhoy Kole 
 *  @date   19.12.2022 
 ***********************************************************/

#include "QGate.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace qcore {

std::uint64_t QGate::gate_count = 0;

bool QGateCompare::operator()(const QGate &lhs, const QGate &rhs) {
    return lhs.getId() < rhs.getId();
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }
    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = RotationMap{};
    this->controls = QubitSet{};
    this->targets = targets;
    this->cbits = CbitSet{};
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const CbitSet &cbits, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }

    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = RotationMap{};
    this->controls = QubitSet{};
    this->targets = targets;
    this->cbits = cbits;
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const RotationMap &angles, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }
    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = angles;
    this->controls = QubitSet{};
    this->targets = targets;
    this->cbits = CbitSet{};
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const CbitSet &cbits, const RotationMap &angles, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }
    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = angles;
    this->controls = QubitSet{};
    this->targets = targets;
    this->cbits = cbits;
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const ControlSet &controls, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }
    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = RotationMap{};
    this->controls = controls;
    this->targets = targets;
    this->cbits = CbitSet{};
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const CbitSet &cbits, const ControlSet &controls, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }
    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = RotationMap{};
    this->controls = controls;
    this->targets = targets;
    this->cbits = cbits;
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const RotationMap &angles, const ControlSet &controls, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }
    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = angles;
    this->controls = controls;
    this->targets = targets;
    this->cbits = CbitSet{};
}

QGate::QGate(const gate_t &gate_type, gsize_t gate_size, const CbitSet &cbits, const RotationMap &angles, const ControlSet &controls, const TargetSet &targets) {
    if (controls.size() + targets.size() != gate_size) {
        throw QcoreException("[QGate] instantiation error msg: Gate size mismatch");
    }
    this->classical = ClassicalCondition{};
    this->gate_id = ++gate_count;
    this->gate_type = gate_type;
    this->gate_size = gate_size;
    this->angles = angles;
    this->controls = controls;
    this->targets = targets;
    this->cbits = cbits;
}

// copy gate
QGate::QGate(const QGate &g) {
    this->classical = g.classical;
    this->gate_id = g.gate_id;
    this->gate_type = g.gate_type;
    this->gate_size = g.gate_size;
    this->angles = g.angles;
    this->controls = g.controls;
    this->targets = g.targets;
    this->cbits = g.cbits;
    this->custom_gate_name = g.custom_gate_name;
    this->call_params = g.call_params;
    this->subcircuit = g.subcircuit;
    this->assertion = g.assertion;
}

QGate QGate::inverse(){
    auto gate = QGate{*this}; 
    gate.gate_type = inverseGateType(gate.gate_type);
    //Rotations and other parameterized gates need to be specified
    if(gate.angles.count(THETA)) {
        gate.angles[THETA] = "-("+gate.angles[THETA]+")";
    }
    if(gate.angles.count(PHI)) {
        gate.angles[PHI] = "-("+gate.angles[PHI]+")";
    }
    if(gate.angles.count(LAMBDA)) {
        gate.angles[LAMBDA] = "-("+gate.angles[LAMBDA]+")";
    }
    if(gate.angles.count(GAMMA)) {
        gate.angles[GAMMA] = "-("+gate.angles[GAMMA]+")";
    }
    return gate;
}

QGate &QGate::operator=(const QGate &g) {
    this->classical = g.classical;
    this->gate_id = g.gate_id;
    this->gate_type = g.gate_type;
    this->gate_size = g.gate_size;
    this->angles = g.angles;
    this->controls = g.controls;
    this->targets = g.targets;
    this->cbits = g.cbits;
    this->custom_gate_name = g.custom_gate_name;
    this->call_params = g.call_params;
    this->subcircuit = g.subcircuit;
    return *this;
}

std::string QGate::operator[](const FileFormat &format) {
    return this->toString(format);
}

std::string QGate::toString(const FileFormat &format, const std::vector<std::string>& qargs) {
    if (format == FileFormat::OpenQASM) {
        return toOpenQASM(qargs);
    } else {
        throw QcoreException("[toString] format " + qcore::toString(format) + " not recognized");
    }
}

void QGate::updateBits(const ControlSet &controls, const TargetSet &targets) {
    this->controls = controls;
    this->targets = targets;
}

void QGate::updateBits(const CbitSet &cbits, const ControlSet &controls, const TargetSet &targets) {
    this->cbits = cbits;
    this->controls = controls;
    this->targets = targets;
}

void QGate::updateBits(const TargetSet &targets) {
    this->targets = targets;
}

void QGate::updateBits(const CbitSet &cbits, const TargetSet &targets) {
    this->cbits = cbits;
    this->targets = targets;
}

void QGate::setCbits(const CbitSet &cbits) {
    this->cbits = cbits;
}

}  // namespace qcore

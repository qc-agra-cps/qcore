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
 *  @file   QCircuit.cpp
 *  @brief  Instance Description for Quantum Circuit
 *  @author Abhoy Kole 
 *  @date   02.02.2022 
 ***********************************************************/


#include "QCircuit.hpp"
#include "GateLibrary.hpp"
#include <algorithm>

namespace qcore {

QCircuit::~QCircuit() {}

QCircuit& QCircuit::operator=(const QCircuit& ckt) {
    clone(ckt);
    return *this;
}

QCircuit::QCircuit(const QCircuit& ckt) {
    clone(ckt);
}

QCircuit::QCircuit(regsize_t qreg, regsize_t creg){
    init();
    this->qreg = qreg;
    this->creg = creg;
    this->qubits.resize(qreg);
    for (Qubit i = 0; i < qreg; i++) {
        this->qubits[i] = i;
    }
    this->cbits.resize(creg);
    for (Cbit i = 0; i < creg; i++) {
        this->cbits[i] = i;
    }
}

QCircuit::QCircuit() {
    init();
}

void QCircuit::clone(const QCircuit& ckt) {
    init();

    this->qreg = ckt.qreg;
    this->creg = ckt.creg;
    this->max_gate_size = ckt.max_gate_size;
    this->qubits = ckt.qubits;
    this->cbits = ckt.cbits;
    this->depth = ckt.depth;
    this->properties = ckt.properties;
    this->source_format_ = ckt.source_format_;
    this->gate_definitions = ckt.gate_definitions;
    this->gate_def_text = ckt.gate_def_text;
    this->gate_library_ = ckt.gate_library_;
    this->max_ancilla = ckt.max_ancilla;

    for (ConstantQGateIterator g = ckt.gates.begin(); g != ckt.gates.end(); ++g) {
        this->gates.push_back(QGatePtr(new QGate(**g)));
    }
}

void QCircuit::cloneEmpty(const QCircuit& ckt) {
    init();

    this->qreg = ckt.qreg;
    this->creg = ckt.creg;
    this->max_gate_size = ckt.max_gate_size;
    this->qubits = ckt.qubits;
    this->cbits = ckt.cbits;
    this->depth = ckt.depth;
    this->properties = ckt.properties;
    this->source_format_ = ckt.source_format_;
    this->gate_definitions = ckt.gate_definitions;
    this->gate_def_text = ckt.gate_def_text;
    this->gate_library_ = ckt.gate_library_;
    this->max_ancilla = ckt.max_ancilla;
}

void QCircuit::init() {
    this->qreg = 0;
    this->creg = 0;
    this->max_gate_size = 0;
    this->qubits = QubitSet{};
    this->cbits = CbitSet{};
    this->gates = QGateSet{};
    this->depth = QubitDepthMap{};
    this->properties = PropertiesMap{};
    this->gate_definitions = {};
    this->gate_def_text = {};
    this->gate_library_ = nullptr;
    this->max_ancilla = 0;

}

std::string QCircuit::toString(const FileFormat& format) {
    auto os = std::ostringstream();
    if (format == FileFormat::OpenQASM3) {
        this->writeQASM3(os);
    } else if (format == FileFormat::OpenQASM) {
        this->writeQASM(os);
    } else {
        throw QcoreException("[toString] format " + qcore::toString(format) + " not recognized");
    }
    return os.str();
}

std::string QCircuit::operator[](const FileFormat& format) {
    return this->toString(format);
}

depth_t QCircuit::getDepth() {
    depth_t max_depth = 0;
    for (auto d : this->depth) {
        if (d.second > max_depth) {
            max_depth = d.second;
        }
    }
    return max_depth;
}

Qubit QCircuit::addQubit(Qubit& qubit) {
    this->qubits.push_back(qubit);
    return qubit;
}

Cbit QCircuit::addCbit(Cbit& cbit) {
    this->cbits.push_back(cbit);
    return cbit;
}

/*void QCircuit::addQGate(QGate& gate){
    this->gates.push_back(std::make_unique<QGate>(gate));
}*/

void QCircuit::addQGate(QGate gate){
    this->gates.push_back(std::make_unique<QGate>(gate));
}

void QCircuit::addQCircuit(QCircuit qc, QubitSet qubits){
    
    addQCircuit(qc);

    for (const auto& q : qc.getCbits()) {
        this->cbits.push_back(q);
    }
    std::sort(this->cbits.begin(), this->cbits.end());
    this->cbits.erase(std::unique(this->cbits.begin(), this->cbits.end()), this->cbits.end());
    if (this->cbits.empty()) {
		this->creg = 0;        
	} else {
		this->creg = *std::max_element(this->cbits.begin(), this->cbits.end()) + 1;
	}

    if (qubits.size() != 0){
        moveQubits(this->qubits, qubits);
    }
}

void QCircuit::addQCircuit(QCircuit qc){
    moveQGates(this->gates, qc.gates);
}

void QCircuit::addQCircuit(const QCircuit qc, int s, int e){
    for (ConstantQGateIterator g = qc.gates.begin() + s; g + e != qc.gates.end(); ++g) {
        this->gates.push_back(QGatePtr(new QGate(**g)));
    }
}

void QCircuit::addQCircuit(QCircuit qc, ClassicalCondition cond) {
    for (auto &gate : qc.getGates()) {
        if (gate->isClassical()) {
            throw QcoreException("[addQCircuit] adding a classical condition to a conditioned circuit.");
        }
        gate->setClassical(cond);
        // gate->setClassical(true);
    }
    this->addQCircuit(qc);
}

void QCircuit::setGateLibrary(std::shared_ptr<GateLibrary> lib) {
    this->gate_library_ = std::move(lib);
}

void QCircuit::defineGate(const std::string& name, std::shared_ptr<QCircuit> body) {
    this->gate_definitions[name] = std::move(body);
}

void QCircuit::addSubcircuitGate(const std::string& name, const QubitSet& qubits, bool inverse, const ClassicalCondition& cond) {
    // Look up in per-instance definitions first
    std::shared_ptr<QCircuit> body;
    auto it = this->gate_definitions.find(name);
    if (it != this->gate_definitions.end()) {
        body = it->second;
    } else if (this->gate_library_) {
        body = this->gate_library_->findCompiledGate(name);
    }
    if (!body) {
	  std::cout << "defined gates:\n";
	  for(auto const &d:this->gate_definitions) { std::cout << "defined: [" << d.first << "]\n"; }
        throw QcoreException("[addSubcircuitGate] undefined gate: " + name);
    }

    std::string use_name = name;
    // inverse if necessary
    if(inverse) {
        body = std::make_shared<QCircuit>(body->inverse());
        use_name += "_inverse";
        defineGate(use_name, body);
        auto txtdef_it = this->gate_def_text.find(name);
        if(txtdef_it == this->gate_def_text.end()) {
            // throw QcoreException("[addSubcircuitGate] undefined gate: " + name);
			defineGateTxt(use_name, {.name = "todo"});
        } else {
			defineGateTxt(use_name, txtdef_it->second);
		}
    }
    gsize_t gsize = static_cast<gsize_t>(qubits.size());
    TargetSet targets(qubits.begin(), qubits.end());
    auto gatePtr = QGatePtr(new QGate(GateType::SUBCIRCUIT, gsize, ControlSet{}, targets));
    gatePtr->setCustomGateName(use_name);
    gatePtr->setSubcircuit(body);

    if (cond.isActive()) {
        gatePtr->setClassical(cond);
    }
    if (this->max_gate_size < gsize) {
        this->max_gate_size = gsize;
    }
    if (this->properties.find(GateType::SUBCIRCUIT) == this->properties.end()) {
        this->properties[GateType::SUBCIRCUIT] = 1;
    } else {
        this->properties[GateType::SUBCIRCUIT] += 1;
    }

    this->gates.push_back(std::move(gatePtr));
}

void QCircuit::setQubits(const QubitSet& qubits) {
    for (const auto& q : qubits) {
        this->qubits.push_back(q);
    }
    //removing duplicates
    std::sort(this->qubits.begin(), this->qubits.end());
    this->qubits.erase(std::unique(this->qubits.begin(), this->qubits.end()), this->qubits.end());
    if (this->qubits.empty()) {
		this->qreg = 0;        
	} else {
		this->qreg = *std::max_element(this->qubits.begin(), this->qubits.end()) + 1;
	}
}
QCircuit QCircuit::inverse(){
    auto qc = QCircuit{};
    qc.clone(*this);
    qc.gates.clear();
    qc.gates.reserve(this->gates.size());
    for (auto i = this->gates.rbegin(); i != this->gates.rend(); ++i){
        qc.addQGate(static_cast<QGate&>(**i).inverse());
    }
    return qc;
}

FileFormat QCircuit::writeQCircuit(const std::string& filename) {
    const std::size_t dot = filename.find_last_of('.');

    std::string extension = filename.substr(dot + 1);

    std::transform(extension.cbegin(), extension.cend(), extension.begin(), [](unsigned char ch) { return ::tolower(ch); });

    if (extension == "qasm") {
        auto ofs = std::ofstream(filename);

        if (!ofs.good()) {
            throw QcoreException("[writeQCircuit] unable to open file " + filename);
        }

        if (source_format_ == FileFormat::OpenQASM3)
            writeQASM3(ofs);
        else
            writeQASM(ofs);

        ofs.close();

        return source_format_;
    }

    /*
    else if (extension == "real") {
        readReal(filename, FileFormat::Real);
    }*/

    else {
        throw QcoreException("[readQCircuit] extension " + extension + " not recognized");
    }

}

FileFormat QCircuit::readQCircuit(const std::string& filename) {
    const std::size_t dot = filename.find_last_of('.');

    std::string extension = filename.substr(dot + 1);

    std::transform(extension.cbegin(), extension.cend(), extension.begin(), [](unsigned char ch) { return ::tolower(ch); });

    if (extension == "qasm") {
        auto ifs = std::ifstream(filename);

        if (!ifs.good()) {
            throw QcoreException("[readQCircuit] unable to open file " + filename);
        }

        // Detect QASM version from the first line and dispatch to the right parser
        std::string first_line;
        std::getline(ifs, first_line);
        ifs.seekg(0);

        bool use_qasm3 = false;
        const std::string qasm_key = "OPENQASM";
        auto kpos = first_line.find(qasm_key);
        if (kpos != std::string::npos) {
            auto start = first_line.find_first_of("0123456789", kpos + qasm_key.size());
            if (start != std::string::npos && std::stod(first_line.substr(start)) >= 3.0)
                use_qasm3 = true;
        }

        if (use_qasm3) {
            readQASM3(ifs);
            source_format_ = FileFormat::OpenQASM3;
        } else {
            readQASM(ifs);
            source_format_ = FileFormat::OpenQASM;
        }

        ifs.close();

        return source_format_;
    }

    /*
    else if (extension == "real") {
        readReal(filename, FileFormat::Real);
    }*/

    else {
        throw QcoreException("[readQCircuit] extension " + extension + " not recognized");
    }
}

/* Interchange the position of i-th qubit with j-th qubit*/
void exchangeQubits(QubitSet& qubits, int i, int j) {
    Qubit qubit = qubits[i];
    qubits[i] = qubits[j];
    qubits[j] = qubit;
}

QubitSet mergeQubits(const std::vector<QubitSet>& q_sets) {
    size_t total_size = 0;
    for (const auto& q_set : q_sets) {
        total_size += q_set.size();
    }
    QubitSet qubits{};
    qubits.reserve(total_size);
    for (const auto& q_set : q_sets) {
        qubits.insert(qubits.end(), q_set.begin(), q_set.end());
    }
    //removing duplicates
    std::sort(qubits.begin(), qubits.end());
    qubits.erase(std::unique(qubits.begin(), qubits.end()), qubits.end());
    //auto it = std::unique(qubits.begin(), qubits.end());
    
    /*
    // checking for duplicate qubits
    if (it != qubits.end()) {
        std::string duplicates = std::to_string(*it);
        while (++it != qubits.end()) {
            duplicates += ", " + std::to_string(*it);
        }
        throw QcoreException("[merge qubits] q:" + duplicates + " msg: found duplicate qubits ");
    }*/
    return qubits;
}

void moveQubits(QubitSet &dest, QubitSet &src){
    dest.reserve(dest.size() + src.size());
    std::move(src.begin(), src.end(), std::back_inserter(dest));

    //removing duplicates
    std::sort(dest.begin(), dest.end());
    dest.erase(std::unique(dest.begin(), dest.end()), dest.end());
}

void moveQGates(QGateSet& dest, QGateSet& src) {
    dest.reserve(dest.size() + src.size());
    std::move(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()), std::back_inserter(dest));
}

// ───────────── Add a gate to the circuit ─────────────
void addGate(gate_t type, gsize_t size, const CbitSet& cbits,
                 const RotationMap& angles, const ControlSet& controls,
                 const TargetSet& targets, const ClassicalCondition& cond,
                 AssertionInfo& assertion, QCircuit &circuit) {
    auto gatePtr = QGatePtr(new QGate(type, size, cbits, angles, controls, targets));


    if (cond.isActive()) {
        gatePtr->setClassical(cond);
    }

    // update circuit stats
    if (circuit.getMaxGateSize() < size) {
        circuit.setMaxGateSize(size);
    }

    if (circuit.getProperties().find(type) == circuit.getProperties().end()) {
        circuit.setProperty(type, 1);
    } else {
        circuit.setProperty(type, circuit.getProperty(type)+1);
    }

    gatePtr->setAssertionInfo(assertion);

    circuit.addGate(gatePtr);
}
}  // namespace qcore

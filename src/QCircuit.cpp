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
    this->qreg = ckt.creg;
    this->max_gate_size = ckt.max_gate_size;
    this->qubits = ckt.qubits;
    this->cbits = ckt.cbits;
    this->depth = ckt.depth;
    this->properties = ckt.properties;

    for (ConstantQGateIterator g = ckt.gates.begin(); g != ckt.gates.end(); ++g) {
        this->gates.push_back(QGatePtr(new QGate(**g)));
    }
}

void QCircuit::init() {
    this->qreg = 0;
    this->qreg = 0;
    this->max_gate_size = 0;
    this->qubits = QubitSet{};
    this->cbits = CbitSet{};
    this->gates = QGateSet{};
    this->depth = QubitDepthMap{};
    this->properties = PropertiesMap{};
}

std::string QCircuit::toString(const FileFormat& format) {
    if (format == FileFormat::OpenQASM) {
        auto os = std::ostringstream();
        this->writeQASM(os);
        return os.str();
    } else {
        throw QcoreException("[toString] format " + qcore::toString(format) + " not recognized");
    }
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

void QCircuit::setQubits(const QubitSet& qubits) {
    for (const auto& q : qubits) {
        this->qubits.push_back(q);
    }
    //removing duplicates
    std::sort(this->qubits.begin(), this->qubits.end());
    this->qubits.erase(std::unique(this->qubits.begin(), this->qubits.end()), this->qubits.end());
    this->qreg = this->qubits.size();
}
QCircuit QCircuit::inverse(){
    auto qc = QCircuit{};
    qc.setQubits(this->qubits);
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
        
        writeQASM(ofs);

        ofs.close();

        return FileFormat::OpenQASM;
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

        readQASM(ifs);
        
        ifs.close();

        return FileFormat::OpenQASM;
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
}  // namespace qcore

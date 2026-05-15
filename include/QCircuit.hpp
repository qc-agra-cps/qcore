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
 *  @file   QCircuit.hpp
 *  @brief  Specification of Quantum Circuit
 *  @author Abhoy Kole
 *  @date   02.02.2022
 ***********************************************************/

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

#include "QGate.hpp"

namespace qcore {

void exchangeQubits(QubitSet& qubits, int i, int j);

class QCircuit {
   private:
    regsize_t qreg;
    regsize_t creg;
    QubitSet qubits;
    CbitSet cbits;
    gsize_t max_gate_size;
    QGateSet gates;

    QubitDepthMap depth;
    PropertiesMap properties;

   public:
    QCircuit();

    QCircuit(const QCircuit& ckt);  //= delete;

    QCircuit(regsize_t qreg, regsize_t creg);

    QCircuit& operator=(const QCircuit& ckt);  // = delete;

    ~QCircuit();  // = default;

    void init();

    void clone(const QCircuit& ckt);

    FileFormat readQCircuit(const std::string& filename);

    FileFormat writeQCircuit(const std::string& filename);

    void readQASM(std::istream& is);

    void writeQASM(std::ostream& os);

    void readQASM3(std::istream& is);

    void writeQASM3(std::ostream& os);

    std::string toString(const FileFormat& format);

    std::string operator[](const FileFormat& format);

    Qubit addQubit(Qubit& qubit);

    //void addQGate(QGate& gate);

    void addQGate(QGate gate);

    /** @brief Merging two quantum circuits and their qubits
     *
     *
     *  @param qc The quantum circuit to be appended
     *  @param qubits The set of qubits to be merged (Default = empty set)
     */
    void addQCircuit(QCircuit qc, QubitSet qubits);

    /** @brief Merging two quantum circuits
     *
     *
     *  @param qc The quantum circuit to be appended
     */
    void addQCircuit(QCircuit qc);

    /** @brief Merging two quantum circuits
     *
     *
     *  @param qc The quantum circuit to be appended
     *  @param s The number of gates to be avoided from the begining  
     *  @param e The number of gates to be avoided from the end
     */
    void addQCircuit(const QCircuit qc, int s, int e);

    /** @brief Finding the inverse of the current quantum circuit
     *
     *
     *  @return The inverse quantum circuit
     */
    QCircuit inverse();

    Cbit addCbit(Cbit& cbit);

    void setQubits(const QubitSet& qubits); 

    inline void setQreg(register_t qreg) {
        this->qreg = qreg;
    }

    inline QGateSet& getGates() {
        return this->gates;
    }

    inline QubitSet& getQubits() {
        return this->qubits;
    }

    inline CbitSet& getCbits() {
        return this->cbits;
    }

    inline regsize_t& getQregSize() {
        return this->qreg;
    }

    inline regsize_t& getCregSize() {
        return this->creg;
    }

    inline gsize_t& getMaxGateSize() {
        return this->max_gate_size;
    }

    depth_t getDepth();

    inline PropertiesMap& getProperties() {
        return this->properties;
    }

    /*
    void readReal(std::istream& is);
    void addQGate(QGate&);

    void writeQCircuit(const std::string& filename, FileFormat format);
    void writeQASM(std::string, unsigned);
    void writeReal(std::string file);

    void displayQASM(std::ostream& os, unsigned size);
    void displayQisKit(std::ostream& os, unsigned size);

    MeasuredQubitMap getMeasures();
    void setGates(QGateSet& gates);
    void setQubits(QubitSet& qubits);
    void setMeasures(MeasuredQubitMap& measures);
    unsigned twoQubitGateCount();
    unsigned getCNOTDepth();
    unsigned maxCNOTDepth();
    void reduceCNOTDepth(QGate& g);
    unsigned twoQubitGateCount(unsigned pos);
    void clear();*/
    friend void exchangeQubits(QubitSet& qubits, int i, int j);
};

/** @brief merging qubits from multiple sources eliminating duplicates.
 *
 *
 *  @param qbit_sets the set of qubit sets
 *  @return the set of qubits
 */
QubitSet mergeQubits(const std::vector<QubitSet> &qbit_sets);

/** @brief moving qubits from sources to destination eliminating duplicates.
 *
 *
 *  @param dest the destination set of qubits 
 *  @param src the source set of qubits
 */
void moveQubits(QubitSet &dest, QubitSet &src);

/** @brief moving quantum gates from source to destination.
 *
 *
 *  @param dest the destination set of quantum gate 
 *  @param src the source set of quantum gates
 */
void moveQGates(QGateSet &dest, QGateSet &src);

}  // namespace qcore

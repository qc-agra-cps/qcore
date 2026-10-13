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
#include <utility>

#include "Definition.hpp"
#include "QGate.hpp"
#include "GateLibrary.hpp"

namespace qcore {

class GateLibrary;

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
    std::map<std::string, std::shared_ptr<QCircuit>> gate_definitions;
    std::map<std::string, GateDefinition> gate_def_text;
    std::shared_ptr<GateLibrary> gate_library_;
    FileFormat source_format_ = FileFormat::OpenQASM;
    regsize_t max_ancilla;

   public:
    QCircuit();

    QCircuit(const QCircuit& ckt);  //= delete;

    QCircuit(regsize_t qreg, regsize_t creg);

    QCircuit& operator=(const QCircuit& ckt);  // = delete;

    ~QCircuit();  // = default;

    void init();

    void clone(const QCircuit& ckt);
    void cloneEmpty(const QCircuit& ckt);

    FileFormat readQCircuitFromString(const std::string &string) {
        std::stringstream ss(string);
        readQASM3(ss);
        return FileFormat::OpenQASM3;
    };

    std::string writeQCircuitToString() {
        std::stringstream ss;
        writeQASM3(ss);
        return ss.str();
    };

    FileFormat writeQCircuitToString(const std::string& string);

    FileFormat readQCircuit(const std::string& filename);

    FileFormat writeQCircuit(const std::string& filename);

    FileFormat getSourceFormat() const { return source_format_; }
    void setSourceFormat(FileFormat fmt) { source_format_ = fmt; }

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

	void addQCircuit(QCircuit qc, ClassicalCondition cond);

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

    inline void setQreg(regsize_t qreg) {
        this->qreg = qreg;
    }

    inline void setCreg(regsize_t creg) {
        this->creg = creg;
    }

    inline QGateSet& getGates() {
        return this->gates;
    }

    inline const QGateSet& getGates() const {
        return this->gates;
    }

    inline void setGates(QGateSet &gates) {
        this->gates = std::move(gates);
    }

    inline void addGate(std::unique_ptr<QGate> &gate) {
        this->gates.push_back(std::move(gate));
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

    inline void setMaxGateSize(gsize_t max_gate_size) {
        this->max_gate_size = max_gate_size;
    }

    depth_t getDepth();

    inline PropertiesMap& getProperties() {
        return this->properties;
    }

    inline void setProperty(GateType type, gcount_t prop) {
        this->properties[type] = prop;
    }

    inline gcount_t getProperty(GateType type) {
        return this->properties[type];
    }

    inline void setMaxNumAncilla(regsize_t max_ancilla) {
        this->max_ancilla = max_ancilla;
    }

    inline regsize_t getMaxNumAncilla() const {
        return max_ancilla;
    }

    /** @brief Register a named subcircuit definition for later use
     *  @param name The gate name
     *  @param body The subcircuit body (using local qubit indices 0..n-1)
     */
    void defineGate(const std::string& name, std::shared_ptr<QCircuit> body);

    /** @brief Add a SUBCIRCUIT gate that invokes a previously defined gate
     *  @param name The gate name (must have been defined via defineGate)
     *  @param qubits The actual qubits this gate operates on
     *  @param inverse Invert the custom circuit?
     */
    void addSubcircuitGate(const std::string& name, const QubitSet& qubits, bool inverse, const ClassicalCondition &cond);

    /** @brief Get the map of defined gate names to their body circuits */
    inline const std::map<std::string, std::shared_ptr<QCircuit>>& getGateDefinitions() const {
        return this->gate_definitions;
    }

    /** @brief Register a named subcircuit definition for later use
     *  @param name The gate name
     *  @param body The textual definition
     */
    void defineGateTxt(const std::string& name, GateDefinition body) {
        this->gate_def_text[name] = std::move(body);
    }

    /** @brief Get the map of defined gate names to their body circuits */
    inline const std::map<std::string, GateDefinition>& getGateDefTxts() const {
        return this->gate_def_text;
    }

    /** @brief Set a shared gate library for predefined custom gates */
    void setGateLibrary(std::shared_ptr<GateLibrary> lib);

    /** @brief Get the shared gate library. The returned library may be shared by many circuits */
    inline std::shared_ptr<const GateLibrary> getGateLibrary() const {
        return this->gate_library_;
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

void addGate(gate_t type, gsize_t size, const CbitSet& cbits,
              const RotationMap& angles, const ControlSet& controls,
              const TargetSet& targets, const ClassicalCondition& cond,
              AssertionInfo& assertion, QCircuit &circuit);

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

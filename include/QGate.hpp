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
 *  @file   QGate.hpp
 *  @brief  Specification of Quantum Gate
 *  @author Abhoy Kole 
 *  @date   19.12.2022 
 ***********************************************************/

#pragma once

#include "Definition.hpp"
#include "GateType.hpp"

namespace qcore {

// Forward declaration for subcircuit support
class QCircuit;

/**
 * @brief To compare two quantum gates for distinguising them
 * 
 */
struct QGateCompare {
    bool operator()(const QGate &lhs, const QGate &rhs);
};


/**
 * @brief Class for representing quantum gate
 * 
 * @details Type for Quantum Gate composing
 *     1. set of control qubits
 *     2. set of target qubits
 *     3. type of gate
 *     4. rotation angle, theta, phi, lambda
 */
class QGate {
   private:
    static std::uint64_t gate_count;
    gateid_t gate_id;
    gate_t gate_type;
    gsize_t gate_size;
    ControlSet controls{};
    TargetSet targets{};
    RotationMap angles{};
    CbitSet cbits{};
    ClassicalCondition classical{};
    std::string custom_gate_name{};
    std::vector<std::string> call_params{};
    std::shared_ptr<QCircuit> subcircuit{nullptr};
    AssertionInfo assertion;

   public:
   /**
    * @brief Construct a new quantum gate from an existing one
    * 
    * @param gate The existing quantum gate 
    */
    QGate(const QGate & gate);

    /**
     * @brief Create duplicate of an existing quantum gate  
     * 
     * @param gate The existing quantum gate
     * @return QGate& The refernece of created duplicate quantum gate
     */
    QGate &operator=(const QGate &gate);

    /**
     * @brief Construct a new QGate object
     * 
     * @param gate_type 
     * @param gate_size 
     * @param targets 
     */
    QGate(const gate_t &gate_type, gsize_t gate_size,
          const TargetSet &targets);

    QGate(const gate_t &gate_type, gsize_t gate_size,
          const CbitSet &cbits, const TargetSet &targets);

    QGate(const gate_t &gate_type, gsize_t gate_size,
          const RotationMap &angles, const TargetSet &targets);

    QGate(const gate_t &gate_type, gsize_t gate_size,
          const CbitSet &cbits, const RotationMap &angles, 
          const TargetSet &targets);

    QGate(const gate_t &gate_type, gsize_t gate_size,
          const ControlSet &controls, const TargetSet &targets);

    QGate(const gate_t &gate_type, gsize_t gate_size,
          const CbitSet &cbits, const ControlSet &controls, 
          const TargetSet &targets);

    QGate(const gate_t &gate_type, gsize_t gate_size,
          const RotationMap &angles, const ControlSet &controls,
          const TargetSet &targets);

    QGate(const gate_t &gate_type, gsize_t gate_size,
          const CbitSet &cbits, const RotationMap &angles, 
          const ControlSet &controls, const TargetSet &targets);

    /** @brief Finding the inverse of a given quantum gate
     *
     *
     *  @return The inverse quantum gate
     */
    QGate inverse();

    std::string toOpenQASM(const std::vector<std::string>& qargs = {});

    std::string toOpenQASM3();

    std::string toString(const FileFormat &format, const std::vector<std::string>& qargs = {});

    std::string operator[](const FileFormat &format);

    inline void setClassical(const unsigned value, const ConditionType &type=eq, const std::string &cbit="c") {
        this->classical = ClassicalCondition{type, cbit, value};
    }
    inline void setClassical(const ClassicalCondition &condition) {
        this->classical = condition;
    }

    inline ClassicalCondition getClassical() const { return this->classical; }

    inline is_classical isClassical() const { return this->classical.type != invalid; }

    inline gate_t getType() { return this->gate_type; }

    inline RotationMap &getAngle() { return this->angles; }

    inline gateid_t getId() const { return this->gate_id; }

    inline QubitSet &getControls() { return this->controls; }

    inline QubitSet &getTargets() { return this->targets; }

    inline CbitSet &getCbits() { return this->cbits; }

    inline const std::string& getCustomGateName() const { return this->custom_gate_name; }
    inline void setCustomGateName(const std::string& name) { this->custom_gate_name = name; }
    inline const std::vector<std::string>& getCallParams() const { return this->call_params; }
    inline void setCallParams(const std::vector<std::string>& params) { this->call_params = params; }
    inline std::shared_ptr<QCircuit> getSubcircuit() const { return this->subcircuit; }
    inline void setSubcircuit(std::shared_ptr<QCircuit> sc) { this->subcircuit = std::move(sc); }
    inline bool isSubcircuit() const { return this->gate_type == GateType::SUBCIRCUIT; }

    void updateBits(const ControlSet &controls, const TargetSet &targets);

    void updateBits(const CbitSet &cbits, const ControlSet &controls,
                    const TargetSet &targets);

    void updateBits(const TargetSet &targets);

    void updateBits(const CbitSet &cbits, const TargetSet &targets);
    void setCbits(const CbitSet &cbits);

    void setAssertionInfo(AssertionInfo &info) {
        this->assertion = info;
    }

    AssertionInfo& getAssertionInfo() {
        return this->assertion;
    }

    bool isAssertion() {
        return this->assertion.isValid() && !this->assertion.isAssumption();
    }
    bool isAssumption() {
        return this->assertion.isValid() && this->assertion.isAssumption();
    }

};
}  // namespace qcore

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
 *  @file   ParseQASM.cpp
 *  @brief  Parser for QASM 2.0
 *  @author Abhoy Kole 
 *  @date   10.03.2022 
 ***********************************************************/
 

#include "Definition.hpp"
#include "GateType.hpp"
#include "QCircuit.hpp"
#include "QGate.hpp"

namespace qcore {
namespace {

constexpr const char* kWhitespace = " \t\r\n";

std::string trimWhitespace(const std::string& s) {
    const auto start = s.find_first_not_of(kWhitespace);
    if (start == std::string::npos) {
        return std::string{};
    }
    const auto end = s.find_last_not_of(kWhitespace);
    return s.substr(start, end - start + 1);
}

Qubit parseQubitIndex(const std::string& token, int line) {
    const auto open = token.find('[');
    const auto close = token.find(']', open == std::string::npos ? 0 : open + 1);
    if (open == std::string::npos || close == std::string::npos || close <= open + 1) {
        throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Failed to parse qubit " + token);
    }
    const auto index_str = token.substr(open + 1, close - open - 1);
    return static_cast<Qubit>(std::stoull(index_str));
}

bool parseMultiControlledXGate(const std::string& qgate,
                               QubitIdMap& var_indices,
                               gate_t& gate_type,
                               size_t& gate_size,
                               ControlSet& controls,
                               TargetSet& targets,
                               int line) {
    auto trimmed = trimWhitespace(qgate);
    if (trimmed.empty()) {
        return false;
    }

    const auto name_end = trimmed.find_first_of(kWhitespace);
    if (name_end == std::string::npos) {
        return false;
    }

    const auto gate_name = trimmed.substr(0, name_end);
    if (gate_name != "mcx" && gate_name != "cnx") {
        return false;
    }

    gate_type = GateType::MCX;

    auto rest = trimWhitespace(trimmed.substr(name_end + 1));
    if (!rest.empty() && rest.back() == ';') {
        rest.pop_back();
    }
    rest = trimWhitespace(rest);
    if (rest.empty()) {
        throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Failed to read gate");
    }

    QubitSet qubits{};
    std::size_t pos = 0;
    while (pos < rest.size()) {
        auto comma = rest.find(',', pos);
        if (comma == std::string::npos) {
            comma = rest.size();
        }
        auto token = trimWhitespace(rest.substr(pos, comma - pos));
        if (!token.empty()) {
            if (var_indices.find(token) == var_indices.end()) {
                var_indices[token] = parseQubitIndex(token, line);
            }
            qubits.push_back(var_indices[token]);
        }
        pos = comma + 1;
    }

    if (qubits.size() < 2) {
        throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Multi-controlled gate requires at least 2 qubits");
    }

    gate_size = qubits.size();
    controls.assign(qubits.begin(), qubits.end() - 1);
    targets.assign(qubits.end() - 1, qubits.end());
    return true;
}

}  // namespace

void QCircuit::readQASM(std::istream& is) {
    QubitIdMap var_indices{};

    CbitIdMap var_cindices{};

    enum HeaderKey : std::uint8_t {  // Header keywords
        NONE,
        OPENQASM,
        INCLUDE,
        QREG,
        CREG,
        KEYCOUNT
    };

    static std::map<std::string, HeaderKey> header_map{
        {"None", HeaderKey::NONE},
        {"OPENQASM", HeaderKey::OPENQASM},
        {"include", HeaderKey::INCLUDE},
        {"qreg", HeaderKey::QREG},
        {"creg", HeaderKey::CREG}};

    static std::map<std::string, GateType> gateType_map{
        {"0", GateType::I},
        {"id", GateType::I},
        {"x", GateType::X},
        {"cx", GateType::CX},
        {"ccx", GateType::CCX},
        {"cnx", GateType::MCX},  // c3x, c4x, ..., cnx
        {"mcx", GateType::MCX}, // Note: keep both (mcx)
        {"rx", GateType::RX},
        {"crx", GateType::CRX},
        {"rxx", GateType::RXX},
        {"rccx", GateType::RCCX},
        {"rc3x", GateType::RC3X},
        {"sx", GateType::SX},
        {"csx", GateType::CSX},
        {"sxdg", GateType::SXDG},
        {"csxdg", GateType::CSXDG},
        {"y", GateType::Y},
        {"cy", GateType::CY},
        {"ry", GateType::RY},
        {"cry", GateType::CRY},
        {"z", GateType::Z},
        {"cz", GateType::CZ},
        {"p", GateType::P},
        {"cp", GateType::CP},
        {"s", GateType::S},
        {"cs", GateType::CS},
        {"sdg", GateType::SDG},
        {"csdg", GateType::CSDG},
        {"t", GateType::T},
        {"ct", GateType::CT},
        {"tdg", GateType::TDG},
        {"ctdg", GateType::CTDG},
        {"rz", GateType::RZ},
        {"crz", GateType::CRZ},
        {"rzz", GateType::RZZ},
        {"h", GateType::H},
        {"ch", GateType::CH},
        {"u1", GateType::U1},
        {"cu1", GateType::CU1},
        {"u2", GateType::U2},
        {"cu2", GateType::CU2},
        {"u3", GateType::U3},
        {"cu3", GateType::CU3},
        {"u", GateType::U},
        {"cu", GateType::CU},
        {"swap", GateType::SWAP},
        {"cswap", GateType::CSWAP},
        {"iswap", GateType::ISWAP},
        {"reset", GateType::RESET},
        {"measure", GateType::MEASURE},
        {"if", GateType::IF},
        {"barrier", GateType::BARRIER}

    };

    enum GateGroup : std::uint8_t {  // Gate Classification based on number of rotation parameters
        ZERO,
        ONE,
        TWO,
        THREE,
        FOUR,
        MEASURE,
        GROUPCOUNT
    };

    static std::map<GateGroup, std::regex> gateGroup_map{
        {GateGroup::ZERO, std::regex("^\\s*([a-z]*[0-9]*)\\s*([A-Za-z][A-Za-z0-9]*\\[[0-9]+\\].*);\\s*$")},
        {GateGroup::ONE, std::regex("^\\s*([a-z]*[0-9]*)\\(\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*\\)(.*);\\s*$")},
        {GateGroup::TWO, std::regex("^\\s*([a-z]*[0-9]*)\\(\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*,\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*\\)(.*);\\s*$")},
        {GateGroup::THREE, std::regex("^\\s*([a-z]*[0-9]*)\\(\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*,\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*,\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*\\)(.*);\\s*$")},
        {GateGroup::FOUR, std::regex("^\\s*([a-z]*[0-9]*)\\(\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*,\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*,\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*,\\s*(pi\\s*/?\\s*[0-9]*?|[0-9]*\\.[0-9]+)\\s*\\)(.*);\\s*$")},
        {GateGroup::MEASURE, std::regex("^\\s*(measure)\\s*([A-Za-z][A-Za-z0-9]*\\[[0-9]+\\])\\s*->\\s*[A-Za-z][A-Za-z0-9]*(\\[[0-9]+\\])\\s*;\\s*$")}};

    std::smatch gateTokens;
    std::string nextToken;
    int line = 0;
    while (!is.eof()) {
        line++;

        if (!static_cast<bool>(is >> nextToken)) {
            throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Failed to read content");
        }

        else if (nextToken.front() == '#' || nextToken.front() == '/') {
            is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;  // skip comments lines
        }

        else if (header_map[nextToken] == HeaderKey::OPENQASM) {
            if (!static_cast<bool>(std::getline(is, nextToken))) {
                throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Failed to read gate");
            }

            // bug fixed by lokr: '*' to '+'
            version_t version = std::stod(std::regex_replace(nextToken, std::regex("^.*([0-9]+\\.[0-9]+).*$"), "$1"));

            if (version > 2.0) {
                throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Unsupported QASM version ");
            }

            continue;  // version
        }

        else if (header_map[nextToken] == HeaderKey::INCLUDE) {
            is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;  // skip include file
        }

        else if (header_map[nextToken] == HeaderKey::QREG) {
            if (!static_cast<bool>(std::getline(is, nextToken))) {
                throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Failed to read gate");
            }

            this->qreg = std::stoull(std::regex_replace(nextToken, std::regex("^.*\\[([0-9]*)\\].*$"), "$1"));

            continue;  // quantum register length
        }

        else if (header_map[nextToken] == HeaderKey::CREG) {
            if (!static_cast<bool>(std::getline(is, nextToken))) {
                throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Failed to read gate");
            }

            this->creg = std::stoull(std::regex_replace(nextToken, std::regex("^.*\\[([0-9]*)\\].*$"), "$1"));

            continue;  // classical register length
        }

        else {  // read quantum gates

            std::string qgate{};
            is_classical classical = false;
            expression_t expression{};
            if (gateType_map[ltrim(rtrim(nextToken))] == GateType::IF) {
                classical = true;
            }

            if (!classical) {
                qgate += nextToken;
            }

            if (!static_cast<bool>(std::getline(is, nextToken))) {
                throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Failed to read gate");
            }
            qgate += " " + nextToken;

            if (classical && std::regex_match(qgate, gateTokens, std::regex("^\\s*\\(\\s*[A-Za-z][A-Za-z0-9]*\\s*==\\s*([0-9]+)\\s*\\).*$"))) {
                expression = gateTokens[1];
                qgate = std::regex_replace(qgate, std::regex("^\\s*\\((\\s*[A-Za-z][A-Za-z0-9]*\\s*==\\s*[0-9]+\\s*)\\)(.*)$"), "$2");
            }

            gate_t gate_type;
            size_t gate_size;
            ControlSet controls{};
            TargetSet targets{};
            RotationMap angles{};
            CbitSet cbits{};

            if (parseMultiControlledXGate(qgate, var_indices, gate_type, gate_size, controls, targets, line)) {
                // Parsed a potentially large MCX without regex.
            }
            else if (std::regex_match(qgate, gateTokens, gateGroup_map[GateGroup::MEASURE])) {
                gate_type = gateType_map[ltrim(rtrim(gateTokens[1]))];

                QubitSet qubits{};
                if (var_indices.find(gateTokens[2]) == var_indices.end()) {                    
                    Qubit q;
                    std::istringstream iss(std::regex_replace(gateTokens[2].str(), std::regex("^\\s*[A-Za-z][A-Za-z0-9]*\\[([0-9]+)\\]\\s*$"), "$1"));
                    iss >> q;
                    var_indices[gateTokens[2]] = q;                    
                }
                qubits.push_back(var_indices[gateTokens[2]]);

                CbitSet gcbits{};
                if (var_cindices.find(gateTokens[3]) == var_cindices.end()) {
                    Cbit c;
                    std::istringstream iss(std::regex_replace(gateTokens[3].str(), std::regex("^\\s*\\[([0-9]+)\\]\\s*$"), "$1"));
                    iss >> c;
                    var_cindices[gateTokens[3]] = c;
                }
                gcbits.push_back(var_cindices[gateTokens[3]]);

                gate_size = qubits.size();

                targets.insert(targets.begin(), qubits.begin(), qubits.end());
                cbits.insert(cbits.begin(), gcbits.begin(), gcbits.end());

            }

            else if (std::regex_match(qgate, gateTokens, gateGroup_map[GateGroup::ZERO])) {
                gate_type = gateType_map[ltrim(rtrim(gateTokens[1]))];

                if (gate_type == GateType::NONE) {
                    throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Unrecognized gate type " + ltrim(rtrim(gateTokens[1])));
                }

                QubitSet qubits{};
                std::string token;
                std::istringstream tokenStream(gateTokens[2]);
                while (std::getline(tokenStream, token, ',')) {
                    token = ltrim(rtrim(token));
                    if (var_indices.find(token) == var_indices.end()) {
                        Qubit q;
                        std::istringstream iss(std::regex_replace(token, std::regex("^\\s*[A-Za-z][A-Za-z0-9]*\\[([0-9]+)\\]\\s*$"), "$1"));
                        iss >> q;
                        var_indices[token] = q;
                    }
                    qubits.push_back(var_indices[token]);
                }

                gate_size = qubits.size();

                if (gate_type != GateType::CSWAP && gate_type != GateType::SWAP && gate_type != GateType::ISWAP &&
                    gate_type != GateType::RXX && gate_type != GateType::RZZ && gate_type != GateType::BARRIER) {
                    controls.insert(controls.begin(), qubits.begin(), qubits.end() - 1);
                    targets.insert(targets.begin(), qubits.end() - 1, qubits.end());
                }

                else if (gate_type == GateType::CSWAP) {
                    controls.insert(controls.begin(), qubits.begin(), qubits.begin() + 1);
                    targets.insert(targets.begin(), qubits.begin() + 1, qubits.end());
                }

                else {
                    targets.insert(targets.begin(), qubits.begin(), qubits.end());
                }

            }

            else if (std::regex_match(qgate, gateTokens, gateGroup_map[GateGroup::ONE])) {
                gate_type = gateType_map[ltrim(rtrim(gateTokens[1]))];

                if (gate_type == GateType::NONE) {
                    throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Unrecognized gate type " + ltrim(rtrim(gateTokens[1])));
                }

                else if (gate_type == GateType::U1) {
                    angles[RotationType::LAMBDA] = ltrim(rtrim(gateTokens[2]));
                }

                else {
                    angles[RotationType::THETA] = ltrim(rtrim(gateTokens[2]));
                }

                QubitSet qubits{};
                std::string token;
                std::istringstream tokenStream(gateTokens[3]);
                while (std::getline(tokenStream, token, ',')) {
                    token = ltrim(rtrim(token));
                    if (var_indices.find(token) == var_indices.end()) {
                        Qubit q;
                        std::istringstream iss(std::regex_replace(token, std::regex("^\\s*[A-Za-z][A-Za-z0-9]*\\[([0-9]+)\\]\\s*$"), "$1"));
                        iss >> q;
                        var_indices[token] = q;
                    }
                    qubits.push_back(var_indices[token]);
                }

                gate_size = qubits.size();

                if (gate_type == GateType::RXX || gate_type == GateType::RZZ) {
                    targets.insert(targets.begin(), qubits.begin(), qubits.end());
                }

                else {
                    controls.insert(controls.begin(), qubits.begin(), qubits.end() - 1);
                    targets.insert(targets.begin(), qubits.end() - 1, qubits.end());
                }

            }

            else if (std::regex_match(qgate, gateTokens, gateGroup_map[GateGroup::TWO])) {
                gate_type = gateType_map[ltrim(rtrim(gateTokens[1]))];

                if (gate_type == GateType::NONE) {
                    throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Unrecognized gate type " + ltrim(rtrim(gateTokens[1])));
                }

                else if (gate_type == GateType::U2) {
                    angles[RotationType::PHI] = ltrim(rtrim(gateTokens[2]));
                    angles[RotationType::LAMBDA] = ltrim(rtrim(gateTokens[3]));
                }

                else {
                    angles[RotationType::THETA] = ltrim(rtrim(gateTokens[2]));
                    angles[RotationType::PHI] = ltrim(rtrim(gateTokens[3]));
                }

                QubitSet qubits{};
                std::string token;
                std::istringstream tokenStream(gateTokens[4]);
                while (std::getline(tokenStream, token, ',')) {
                    token = ltrim(rtrim(token));
                    if (var_indices.find(token) == var_indices.end()) {
                        Qubit q;
                        std::istringstream iss(std::regex_replace(token, std::regex("^\\s*[A-Za-z][A-Za-z0-9]*\\[([0-9]+)\\]\\s*$"), "$1"));
                        iss >> q;
                        var_indices[token] = q;
                    }
                    qubits.push_back(var_indices[token]);
                }

                gate_size = qubits.size();

                controls.insert(controls.begin(), qubits.begin(), qubits.end() - 1);
                targets.insert(targets.begin(), qubits.end() - 1, qubits.end());

            }

            else if (std::regex_match(qgate, gateTokens, gateGroup_map[GateGroup::THREE])) {
                gate_type = gateType_map[ltrim(rtrim(gateTokens[1]))];

                if (gate_type == GateType::NONE) {
                    throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Unrecognized gate type " + ltrim(rtrim(gateTokens[1])));
                }

                angles[RotationType::THETA] = ltrim(rtrim(gateTokens[2]));
                angles[RotationType::PHI] = ltrim(rtrim(gateTokens[3]));
                angles[RotationType::LAMBDA] = ltrim(rtrim(gateTokens[4]));

                QubitSet qubits{};
                std::string token;
                std::istringstream tokenStream(gateTokens[5]);
                while (std::getline(tokenStream, token, ',')) {
                    token = ltrim(rtrim(token));
                    if (var_indices.find(token) == var_indices.end()) {
                        Qubit q;
                        std::istringstream iss(std::regex_replace(token, std::regex("^\\s*[A-Za-z][A-Za-z0-9]*\\[([0-9]+)\\]\\s*$"), "$1"));
                        iss >> q;
                        var_indices[token] = q;
                    }
                    qubits.push_back(var_indices[token]);
                }

                gate_size = qubits.size();

                controls.insert(controls.begin(), qubits.begin(), qubits.end() - 1);
                targets.insert(targets.begin(), qubits.end() - 1, qubits.end());

            }

            else if (std::regex_match(qgate, gateTokens, gateGroup_map[GateGroup::FOUR])) {
                gate_type = gateType_map[ltrim(rtrim(gateTokens[1]))];

                if (gate_type == GateType::NONE) {
                    throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Unrecognized gate type " + ltrim(rtrim(gateTokens[1])));
                }

                angles[RotationType::THETA] = ltrim(rtrim(gateTokens[2]));
                angles[RotationType::PHI] = ltrim(rtrim(gateTokens[3]));
                angles[RotationType::LAMBDA] = ltrim(rtrim(gateTokens[4]));
                angles[RotationType::GAMMA] = ltrim(rtrim(gateTokens[5]));

                QubitSet qubits{};
                std::string token;
                std::istringstream tokenStream(gateTokens[6]);
                while (std::getline(tokenStream, token, ',')) {
                    token = ltrim(rtrim(token));
                    if (var_indices.find(token) == var_indices.end()) {
                        Qubit q;
                        std::istringstream iss(std::regex_replace(token, std::regex("^\\s*[A-Za-z][A-Za-z0-9]*\\[([0-9]+)\\]\\s*$"), "$1"));
                        iss >> q;
                        var_indices[token] = q;
                    }
                    qubits.push_back(var_indices[token]);
                }

                gate_size = qubits.size();

                controls.insert(controls.begin(), qubits.begin(), qubits.end() - 1);
                targets.insert(targets.begin(), qubits.end() - 1, qubits.end());

            }

            else {
                throw QcoreException("[readQASM] l:" + std::to_string(line) + " msg: Unable to process");
            }

            auto gatePtr = QGatePtr(new QGate(gate_type, gate_size, cbits, angles, controls, targets));

            gatePtr->setIsClassical(classical);
            if (classical) {
                gatePtr->setExpression(expression);
            }

            this->gates.push_back(std::move(gatePtr));

            if (this->max_gate_size < gate_size) {
                this->max_gate_size = gate_size;
            }

            if (this->properties.find(gate_type) == this->properties.end()) {
                this->properties[gate_type] = 1;
            } else {
                this->properties[gate_type] += 1;
            }

            QubitSet qubits(controls);
            std::copy(targets.begin(), targets.end(), std::back_inserter(qubits));
            for (auto qubit : qubits) {
                if (this->depth.find(qubit) == this->depth.end()) {
                    this->depth[qubit] = 1;
                } else {
                    this->depth[qubit] += 1;
                }
            }
        }

        std::transform(var_indices.begin(), var_indices.end(), std::back_inserter(this->qubits), [](QIdMapElement q) { return q.second; });
        std::transform(var_cindices.begin(), var_cindices.end(), std::back_inserter(this->cbits), [](CIdMapElement c) { return c.second; });
    }
}

void QCircuit::writeQASM(std::ostream& os) {
        
        os <<  //Header part
        "OPENQASM 2.0;\n"
        "include \"qelib1.inc\";\n\n"
        "qreg q[" +
        std::to_string(this->qreg) +
        "];\n"
        "creg c[" +
        std::to_string(this->creg) + "];";

    for (ConstantQGateIterator g = this->gates.begin(); g != this->gates.end(); ++g) {
        //Quantum Gates 
        os << "\n" + (*g)->toString(FileFormat::OpenQASM);
    }

}

std::string QGate::toOpenQASM() {
    std::string gateString{};

    if (this->flag) {
        gateString += "if ( c == " + this->expression + " ) ";
    }

    gateString += qcore::toString(this->gate_type);

    if (this->angles.size() != 0) {
        gateString += "( ";

        if (this->angles.find(RotationType::THETA) != this->angles.end()) {
            gateString += this->angles[RotationType::THETA];
            if (this->angles.find(RotationType::PHI) != this->angles.end()) {
                gateString += ", ";
            }
        }

        if (this->angles.find(RotationType::PHI) != this->angles.end()) {
            gateString += this->angles[RotationType::PHI];
            if (this->angles.find(RotationType::LAMBDA) != this->angles.end()) {
                gateString += ", ";
            }
        }

        if (this->angles.find(RotationType::LAMBDA) != this->angles.end()) {
            gateString += this->angles[RotationType::LAMBDA];
            if (this->angles.find(RotationType::GAMMA) != this->angles.end()) {
                gateString += ", ";
            }
        }

        if (this->angles.find(RotationType::GAMMA) != this->angles.end()) {
            gateString += this->angles[RotationType::GAMMA];
        }
        gateString += " )";
    }

    for (ConstantQubitIterator q = this->controls.cbegin(); q != this->controls.cend(); q++) {
        gateString += " q[" + std::to_string(*q) + "],";
    }

    for (ConstantQubitIterator q = this->targets.cbegin(); q != this->targets.cend(); q++) {
        gateString += " q[" + std::to_string(*q) + "]";

        if (q != this->targets.cend() - 1) {
            gateString += ",";
        }

        else if (this->gate_type != GateType::MEASURE) {
            gateString += ";";
        }
    }

    for (ConstantCbitIterator c = this->cbits.cbegin(); c != this->cbits.cend(); c++) {
        gateString += " -> c[" + std::to_string(*c) + "]"; 
    }

    if (this->gate_type == GateType::MEASURE) {
        gateString += ";";
    }

    return gateString;
}

}  // namespace qcore

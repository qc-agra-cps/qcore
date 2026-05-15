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
 *  @file   Definition.hpp
 *  @brief  Reversible and Quantum GATE Instance Description
 *  @author Abhoy Kole 
 *  @date   18.02.2023
 ***********************************************************/

#pragma once

#include <bitset>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <string>
#include <map>
#include <tuple>

namespace qcore {
// Type for quantum bit
using Qubit = std::uint64_t;
using line_t = Qubit;  // for backward compitable

// register size
using regsize_t = std::uint64_t;
using gsize_t = std::uint32_t;

// Type for classical bit
using Cbit = std::uint32_t;

// Type for unique gate identifier
using gateid_t = std::uint64_t;

// Type for storing angle of rotation
using angle_t = std::string;

// Type for holding gate count
using gcount_t = std::uint64_t;

// Quantum bit Collection
using QubitSet = std::vector<Qubit>;
using ControlSet = QubitSet;
using TargetSet = QubitSet;

// Classical bit collection
using CbitSet = std::vector<Cbit>;

// forward declaration of quantum Gate
class QGate;
using QGatePtr = std::unique_ptr<QGate>;
using QGateSet = std::vector<std::unique_ptr<QGate>>;

// Quantum to Classical Mapping for Result
using MeasuredQubitMap = std::map<Qubit, Cbit>;

// qubit names
using QubitLabelMap = std::map<Qubit, std::string>;
using QubitIdMap = std::map<std::string, Qubit>;
using QIdMapElement = std::pair<std::string, Qubit>;

// cbit names
using CbitLabelMap = std::map<Cbit, std::string>;
using CbitIdMap = std::map<std::string, Cbit>;
using CIdMapElement = std::pair<std::string, Cbit>;

// per qubit gate depth
using depth_t = std::uint64_t;
using QubitDepthMap = std::map<line_t, depth_t>;

// Layout mapping
using PermutationMap = std::map<Qubit, Qubit>;

using QubitIterator = std::vector<Qubit>::iterator;
using CbitIterator = std::vector<Cbit>::iterator;
using ConstantQubitIterator = std::vector<Qubit>::const_iterator;
using ConstantCbitIterator = std::vector<Cbit>::const_iterator;
using QubitReverseIterator = std::vector<Qubit>::reverse_iterator;
using CbitReverseIterator = std::vector<Cbit>::reverse_iterator;
using QGateIterator = QGateSet::iterator;
using ConstantQGateIterator = QGateSet::const_iterator;
using QGateReverseIterator = QGateSet::reverse_iterator;
using MeasuredQubitIterator = MeasuredQubitMap::iterator;
using ConstantMeasuredQubitIterator = MeasuredQubitMap::const_iterator;
using QubitLabelIterator = QubitLabelMap::iterator;
using ConstantQubitLabelIterator = QubitLabelMap::const_iterator;
using QubitIdIterator = QubitIdMap::iterator;
using ConstantQubitIdIterator = QubitIdMap::const_iterator;
using CbitLabelIterator = CbitLabelMap::iterator;
using ConstantCbitLabelIterator = CbitLabelMap::const_iterator;
using CbitIdIterator = CbitIdMap::iterator;
using ConstantCbitIdIterator = CbitIdMap::const_iterator;
using QubitDepthIterator = QubitDepthMap::iterator;
using ConstantQubitDepthIterator = QubitDepthMap::const_iterator;
using PermutationIterator = PermutationMap::iterator;
using ConstantPermutationIterator = PermutationMap::const_iterator;

// floating-point type used throughout the library
using fp = double;

// PI	3.142857143
static constexpr fp PI = static_cast<fp>(3.141592653589793238462643383279502884197169399375105820974L);

class QcoreException : public std::invalid_argument {
    std::string msg;

   public:
    explicit QcoreException(std::string m) : std::invalid_argument("QCORE Exception"), msg(std::move(m)) {}

    [[nodiscard]] const char *what() const noexcept override {
        return msg.c_str();
    }
};

// supported file formats
enum class FileFormat {
    Real,
    OpenQASM,
    FormatCount
};

using format_t = FileFormat;

using version_t = double;

// types for ZX diagram
/**
 * @brief Enum class representing the different types of vertices in a ZX-diagram
 */
enum class VertexType {
    BOUNDARY = 0,
    Z = 1,
    X = 2,
    CLASSICAL_Z = 3,
    CLASSICAL_X = 4,
    DISCARD = 5,
};

inline std::string vertex_type_to_string(const VertexType& t) {
    switch (t) {
        case VertexType::BOUNDARY:
            return "BOUNDARY";
        case VertexType::Z:
            return "Z";
        case VertexType::X:
            return "X";
        case VertexType::CLASSICAL_Z:
            return "CLASSICAL_Z";
        case VertexType::CLASSICAL_X:
            return "CLASSICAL_X";
        case VertexType::DISCARD:
            return "DISCARD";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Enum class representing the different types of edges in a ZX-diagram
 */
enum class EdgeType {
    NORMAL = 0,
    HADAMARD = 1,
    CLASSICAL_NORMAL = 2,
    CLASSICAL_HADAMARD = 3,
};

inline std::string edge_type_to_string(const EdgeType& t) {
    switch (t) {
        case EdgeType::NORMAL:
            return "NORMAL";
        case EdgeType::HADAMARD:
            return "HADAMARD";
        case EdgeType::CLASSICAL_NORMAL:
            return "CLASSICAL_NORMAL";
        case EdgeType::CLASSICAL_HADAMARD:
            return "CLASSICAL_HADAMARD";
        default:
            return "UNKNOWN";
    }
}

using VertexId = std::size_t;
using AdjacencyList = std::unordered_map<VertexId, std::vector<std::pair<VertexId, EdgeType>>>;
using Column = int;


// inline_functions
inline std::string toString(const FileFormat &format) {
    switch (format) {
        case FileFormat::OpenQASM:
            return "OPENQASM";
        case FileFormat::Real:
            return "REAL";
        default:
            throw std::invalid_argument("Invalid RotationType!");
    }
}

inline std::string ltrim(const std::string &s) {
    return std::regex_replace(s, std::regex("^\\s+"), std::string(""));
}

inline std::string rtrim(const std::string &s) {
    return std::regex_replace(s, std::regex("\\s+$"), std::string(""));
}

inline double round_sig(double num, int sig) {
    return static_cast<double>(static_cast<int>(num * pow(10, sig))) / pow(10, sig);
}

}  // namespace qcore


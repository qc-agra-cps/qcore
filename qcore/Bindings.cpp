/*
 * This file is part of QCORE library which is released under the <LICENSE> license.
 * See file README.md or go to https://<LICENSE>/research/quantum/ for more information.
 */


#include <cstdint>
#include <functional>
#include <ios>
#include <pybind11_json/pybind11_json.hpp>
#include <nlohmann/json.hpp>
#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/native_enum.h>

#include "QCircuit.hpp"
#include "QGate.hpp"
#include "GateType.hpp"
#include "decompose/Clifford_T.hpp"
#include "decompose/assertions.h"
#include "decompose/Dynamic.hpp"


#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)


namespace py = pybind11;
namespace nl = nlohmann;
using namespace pybind11::literals;

// Read from a QASM string, auto-detecting version (QASM2 or QASM3).
static void readQASMAuto(qcore::QCircuit* ckt, std::istream& is) {
    std::string first_line;
    std::getline(is, first_line);
    is.seekg(0);
    if (first_line.find("OPENQASM 3") != std::string::npos) {
        ckt->readQASM3(is);
        ckt->setSourceFormat(qcore::FileFormat::OpenQASM3);
    } else {
        ckt->readQASM(is);
        ckt->setSourceFormat(qcore::FileFormat::OpenQASM);
    }
}

nl::json read_From_File(const std::string &filename){
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        qcore::format_t format = ckt->readQCircuit(filename);
        j_string = ckt->toString(format);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }

    return j_string;
}

nl::json readQASM(const nl::json &jqasm){
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        auto is = std::istringstream (jqasm.get<std::string>());
        ckt->readQASM(is);
        j_string = ckt->toString(qcore::FileFormat::OpenQASM);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }

    return j_string;
}

nl::json circuitProperties(const nl::json &jqasm) {
    nl::json properties {};
    auto ckt = new qcore::QCircuit();
    try{
        auto is = std::istringstream (jqasm.get<std::string>());
        readQASMAuto(ckt, is);
        properties["QuantumRegSize"] = ckt->getQregSize();
        properties["ClassicalRegSize"] = ckt->getCregSize();
        nl::json gateSummary {};
        for (auto element : ckt->getProperties()){
            gateSummary[qcore::toString(element.first)] = element.second;
        }
        properties["GateSummary"]  = gateSummary;
        properties["CircuitDepth"]  = ckt->getDepth();

    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }

    return properties;
}

std::string dynamicCircuitProperties(const nl::json &jqasm) {
    nl::json result {};
    auto ckt = new qcore::QCircuit();
    try {
        auto is = std::istringstream(jqasm.get<std::string>());
        readQASMAuto(ckt, is);

        // Basic circuit information
        result["QuantumRegSize"] = ckt->getQregSize();
        result["ClassicalRegSize"] = ckt->getCregSize();

        // Count measurements
        int measurement_count = 0;
        for (const auto& gate_ptr : ckt->getGates()) {
            if ((*gate_ptr).getType() == qcore::GateType::MEASURE) {
                measurement_count++;
            }
        }
        result["MeasurementCount"] = measurement_count;

        // Separate gates into unconditional and conditional groups
        std::map<qcore::GateType, qcore::gcount_t> common_gate_counts;
        std::map<std::string, std::map<qcore::GateType, qcore::gcount_t>> conditional_gate_counts;

        for (const auto& gate_ptr : ckt->getGates()) {
            qcore::QGate& gate = *gate_ptr;
            qcore::GateType type = gate.getType();

            if (gate.isClassical()) {
                // Conditional gate - group by expression
                std::string condition = gate.getClassical().toString();
                conditional_gate_counts[condition][type]++;
            } else {
                // Unconditional gate - always executes
                common_gate_counts[type]++;
            }
        }

        // Helper lambdas for T-depth computation
        auto isTGate = [](qcore::GateType t) {
            return t == qcore::GateType::T || t == qcore::GateType::TDG ||
                   t == qcore::GateType::CT || t == qcore::GateType::CTDG;
        };
        auto isNonUnitary = [](qcore::GateType t) {
            return t == qcore::GateType::MEASURE || t == qcore::GateType::IF ||
                   t == qcore::GateType::BARRIER || t == qcore::GateType::RESET;
        };

        // Build CommonGates properties
        nl::json common_properties;
        nl::json common_gate_summary {};
        for (const auto& [type, count] : common_gate_counts) {
            common_gate_summary[qcore::toString(type)] = count;
        }
        common_properties["GateSummary"] = common_gate_summary;

        // Compute T-depth for common gates (depth_pre: gates before first measurement)
        // Rule D: Measurement creates temporal barrier
        qcore::depth_t depth_pre = 0;
        {
            std::map<qcore::Qubit, qcore::depth_t> qubit_td;
            for (const auto& gate_ptr : ckt->getGates()) {
                qcore::QGate& gate = *gate_ptr;
                qcore::GateType type = gate.getType();

                // Stop at first measurement/barrier (temporal barrier - Rule D)
                if (isNonUnitary(type)) break;

                // Skip conditional gates (they belong to branches)
                if (gate.isClassical()) continue;

                // Rule A: T gates increment depth by 1
                // Rule B: Clifford gates don't change depth
                // Rule C: Multi-qubit gates synchronize qubits
                auto& controls = gate.getControls();
                auto& targets = gate.getTargets();
                qcore::depth_t max_td = 0;
                for (auto q : controls)
                    if (qubit_td.count(q) && qubit_td[q] > max_td) max_td = qubit_td[q];
                for (auto q : targets)
                    if (qubit_td.count(q) && qubit_td[q] > max_td) max_td = qubit_td[q];
                qcore::depth_t new_td = isTGate(type) ? max_td + 1 : max_td;
                for (auto q : controls) qubit_td[q] = new_td;
                for (auto q : targets) qubit_td[q] = new_td;
            }
            for (const auto& [q, td] : qubit_td)
                if (td > depth_pre) depth_pre = td;
            common_properties["TDepth"] = depth_pre;
        }

        result["CommonGates"] = common_properties;

        // Build properties for each conditional path
        nl::json paths;
        for (const auto& [condition, gate_counts] : conditional_gate_counts) {
            nl::json path_properties;

            // Gate summary for this path only
            nl::json path_gate_summary {};
            for (const auto& [type, count] : gate_counts) {
                path_gate_summary[qcore::toString(type)] = count;
            }
            path_properties["GateSummary"] = path_gate_summary;

            // Total gate summary (common + this path)
            std::map<qcore::GateType, qcore::gcount_t> total_counts = common_gate_counts;
            for (const auto& [type, count] : gate_counts) {
                total_counts[type] += count;
            }

            nl::json total_gate_summary {};
            for (const auto& [type, count] : total_counts) {
                total_gate_summary[qcore::toString(type)] = count;
            }
            path_properties["TotalGateSummary"] = total_gate_summary;

            // Compute total T-depth for this path: depth_pre + depth_path
            // Rule E: Each branch starts with fresh counters (all zeros)
            {
                std::map<qcore::Qubit, qcore::depth_t> qubit_td;  // Fresh counters!

                for (const auto& gate_ptr : ckt->getGates()) {
                    qcore::QGate& gate = *gate_ptr;
                    qcore::GateType type = gate.getType();

                    // Only process conditional gates for this specific path
                    if (!gate.isClassical()) continue;
                    if (gate.getClassical().toString() != condition) continue;

                    // Skip non-unitary gates
                    if (isNonUnitary(type)) continue;

                    // Apply Rules A, B, C to this branch's gates
                    auto& controls = gate.getControls();
                    auto& targets = gate.getTargets();
                    qcore::depth_t max_td = 0;
                    for (auto q : controls)
                        if (qubit_td.count(q) && qubit_td[q] > max_td) max_td = qubit_td[q];
                    for (auto q : targets)
                        if (qubit_td.count(q) && qubit_td[q] > max_td) max_td = qubit_td[q];
                    qcore::depth_t new_td = isTGate(type) ? max_td + 1 : max_td;
                    for (auto q : controls) qubit_td[q] = new_td;
                    for (auto q : targets) qubit_td[q] = new_td;
                }

                // Compute depth_path (max across all qubits in this branch)
                qcore::depth_t depth_path = 0;
                for (const auto& [q, td] : qubit_td)
                    if (td > depth_path) depth_path = td;

                // Total T-depth = depth_pre + depth_post (Rule D + Rule E)
                qcore::depth_t total_t_depth = depth_pre + depth_path;
                path_properties["TotalTDepth"] = total_t_depth;
            }

            // Create readable path name: normalize "c==1" or "c[0]==1" to "c[0]==<value>",
            // and bare values like "1" to "c[0]==1".
            auto eq_pos = condition.find("==");
            std::string value = (eq_pos != std::string::npos) ? condition.substr(eq_pos + 2) : condition;
            std::string path_name = "c[0]==" + value;
            paths[path_name] = path_properties;
        }

        result["Paths"] = paths;

    } catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }

    // Return pretty-printed JSON with 2-space indentation
    return result.dump(2);
}

nl::json writeQASM(const nl::json &jqasm, const std::string &filename) {
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        auto is = std::istringstream (jqasm.get<std::string>());
        ckt->readQASM(is);
        j_string = ckt->toString(qcore::FileFormat::OpenQASM);
        ckt->writeQCircuit(filename);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }


    return j_string;
}

// Decompose MCX to RCCX, CCX, X gates (pair encoding)
nl::json decomposeToIntermediateRCCX(const std::string &inputPath, const std::string &outputPath) {
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        ckt->readQCircuit(inputPath);
        qcore::Qubit ancilla = ckt->getQregSize();
        auto decomposed = qcore::decompose_circuit_to_intermediate_RCCX(*ckt, ancilla);
        decomposed.setSourceFormat(ckt->getSourceFormat());
        j_string = decomposed.toString(qcore::FileFormat::OpenQASM);
        decomposed.writeQCircuit(outputPath);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }
    return j_string;
}

nl::json decomposeToCliffordT(const std::string &inputPath, const std::string &outputPath) {
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        ckt->readQCircuit(inputPath);
        qcore::Qubit ancilla = ckt->getQregSize();
        auto decomposed = qcore::decompose_circuit_to_clifford_t(*ckt, ancilla);
        decomposed.setSourceFormat(ckt->getSourceFormat());
        j_string = decomposed.toString(qcore::FileFormat::OpenQASM);
        decomposed.writeQCircuit(outputPath);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }
    return j_string;
}

nl::json decomposeToIntermediateRC3X(const std::string &inputPath, const std::string &outputPath) {
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        ckt->readQCircuit(inputPath);
        qcore::Qubit ancilla = ckt->getQregSize();
        auto decomposed = qcore::decompose_circuit_to_intermediate_RC3X(*ckt, ancilla);
        decomposed.setSourceFormat(ckt->getSourceFormat());
        j_string = decomposed.toString(qcore::FileFormat::OpenQASM);
        decomposed.writeQCircuit(outputPath);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }
    return j_string;
}

// Decompose RCCX intermediate circuit to dynamic circuit with measurement
nl::json decomposeIntermediateRCCXToDynamic(const std::string &inputPath, const std::string &outputPath, int cbit = 0) {
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        ckt->readQCircuit(inputPath);
        qcore::Qubit ancilla = ckt->getQregSize() - 1;  // Last qubit is ancilla
        auto decomposed = qcore::decompose_intermediate_RCCX_to_dynamic(*ckt, ancilla, cbit);
        decomposed.setSourceFormat(ckt->getSourceFormat());
        j_string = decomposed.toString(qcore::FileFormat::OpenQASM);
        decomposed.writeQCircuit(outputPath);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }
    return j_string;
}

// Decompose RC3X intermediate circuit to dynamic circuit with measurement
nl::json decomposeIntermediateRC3XToDynamic(const std::string &inputPath, const std::string &outputPath, int cbit = 0) {
    auto ckt = new qcore::QCircuit();
    nl::json j_string = "";
    try{
        ckt->readQCircuit(inputPath);
        qcore::Qubit ancilla = ckt->getQregSize() - 1;  // Last qubit is ancilla
        auto decomposed = qcore::decompose_intermediate_RC3X_to_dynamic(*ckt, ancilla, cbit);
        decomposed.setSourceFormat(ckt->getSourceFormat());
        j_string = decomposed.toString(qcore::FileFormat::OpenQASM);
        decomposed.writeQCircuit(outputPath);
    }
    catch( const std::exception & ex ) {
          std::cout << ex.what() << std::endl;
    }
    return j_string;
}

template <typename T> struct PyTypeConv {
  using type = T;
	static type fwd(T const &in) {
		return (type)in;
    }
	static T bwd(type const &in) {
		return (T)in;
    }
};
template<typename T> using pytype = typename PyTypeConv<T>::type;
template <typename R, typename... Args>
std::function<pytype<R>(pytype<Args>...)>
python_wrapper(R (*fn)(Args...)) {
  return [fn](pytype<Args>... args) -> pytype<R> {
        return PyTypeConv<R>::fwd(
		  fn(PyTypeConv<Args>::bwd(args)...)
		  );
	};
}

PYBIND11_MODULE(pyqcore, m) {
    m.doc() = R"pbdoc(
        Python interface for the QCORE quantum core library
        ---------------------------------------------------
        .. autosummary::
           :toctree: _generate
           read_From_File
           readQASM
           circuitProperties
           dynamicCircuitProperties
           writeQASM
           decomposeToIntermediateRCCX
           decomposeToCliffordT
           decomposeToIntermediateRC3X
           decomposeIntermediateRCCXToDynamic
           decomposeIntermediateRC3XToDynamic
    )pbdoc";

    m.def("read_From_File", &read_From_File, "create a quantum circuit reading a file",
    "file_name");
    m.def("readQASM", &readQASM, "create a quantum circuit reading a qasm string",
    "qasm");
    m.def("circuitProperties", &circuitProperties, R"pbdoc(
        Provides following summary of a quantum circuit
        -----------------------------------------------
        number of qubits:
        number of cbits:
        circuit depth:
        number of gates of each type:
    )pbdoc");

    m.def("dynamicCircuitProperties", &dynamicCircuitProperties, R"pbdoc(
        Provides execution-path-aware properties for dynamic quantum circuits
        ---------------------------------------------------------------------
        For circuits with conditional gates (IF statements), this function
        separates gate counts by execution path, providing accurate resource
        estimates for each measurement outcome.

        Returns:
            Dictionary containing:
            - QuantumRegSize: Number of qubits
            - ClassicalRegSize: Number of classical bits
            - MeasurementCount: Number of measurement operations
            - CommonGates: Gates that always execute (unconditional)
              - GateSummary: Gate type counts for common gates
            - Paths: Dictionary of execution paths by measurement outcome
              - "c[0]==0": Properties when measurement result is 0
                - GateSummary: Gate counts for this path only
                - TotalGateSummary: Common gates + this path's gates
              - "c[0]==1": Properties when measurement result is 1
                - GateSummary: Gate counts for this path only
                - TotalGateSummary: Common gates + this path's gates

        This is essential for analyzing dynamic circuits where different
        measurement outcomes lead to different gate sequences. Particularly
        useful for comparing T-counts between measurement branches.
    )pbdoc");

    m.def("writeQASM", &writeQASM, "write quantum circuit to a file",
    "file_name");

    m.def("decomposeToIntermediateRCCX", &decomposeToIntermediateRCCX, R"pbdoc(
        Decompose MCX gates to intermediate form (RCCX, CCX, X gates)
        -------------------------------------------------------------
        Reads a QASM file, decomposes all multi-control Toffoli gates using
        the linear decomposition pattern, and writes the result.

        The ancilla qubit is automatically allocated as the next available qubit.
        Output contains only RCCX, CCX, and X gates (no Clifford+T decomposition).
    )pbdoc",
    "input_path", "output_path");

    m.def("decomposeToCliffordT", &decomposeToCliffordT, R"pbdoc(
        Decompose MCX gates to Clifford+T gates (H, T, TDG, CX)
        -------------------------------------------------------
        Reads a QASM file and decomposes the gates it finds to Clifford+T.
        RCCX and RC3X are decomposed according to their gate type.
        If MCX gates are present, the RCCX-based linear decomposition is used.

        The ancilla qubit is automatically allocated as the next available qubit.
    )pbdoc",
    "input_path"_a, "output_path"_a);

    m.def("decomposeToIntermediateRC3X", &decomposeToIntermediateRC3X, R"pbdoc(
        Decompose MCX gates using RC3X-based triplet encoding (RC3X, CCX, CX, X gates)
        ------------------------------------------------------------------------------
        Uses only RC3X (C3(iX)) gates for triplet encoding (3 controls at a time).
        Works for n >= 3 controls. Results in fewer gates than RCCX-based approach.

        The ancilla qubit is automatically allocated as the next available qubit.
    )pbdoc",
    "input_path", "output_path");

    m.def("decomposeIntermediateRCCXToDynamic", &decomposeIntermediateRCCXToDynamic, R"pbdoc(
        Convert RCCX intermediate circuit to dynamic circuit with measurement
        --------------------------------------------------------------------
        Implements dynamic decomposition pattern from "Adaptive Clifford+T Decomposition
        of MCT Gates with One Clean Ancilla" (Figure 3).

        Replaces the final RCCX gate (that uncomputes the ancilla) with:
          1. S†(ancilla) - Phase correction
          2. H(ancilla) - Hadamard basis measurement preparation
          3. MEASURE(ancilla) -> c[cbit] - Mid-circuit measurement
          4. IF (c[cbit] == 1): CZ(control, target) - Conditional correction

        This reduces T-count by 4 compared to the static CCX implementation.

        Args:
            input_path: Path to RCCX intermediate QASM file
            output_path: Path for output dynamic QASM file
            cbit: Classical bit for measurement result (default: 0)

        Returns:
            QASM string of the dynamic circuit
    )pbdoc",
    "input_path"_a, "output_path"_a, "cbit"_a = 0);

    m.def("decomposeIntermediateRC3XToDynamic", &decomposeIntermediateRC3XToDynamic, R"pbdoc(
        Convert RC3X intermediate circuit to dynamic circuit with measurement
        --------------------------------------------------------------------
        Implements dynamic decomposition pattern from "Adaptive Clifford+T Decomposition
        of MCT Gates with One Clean Ancilla" (Figure 4).

        Similar to RCCX variant but with different conditional operations:
          1. S†(ancilla) - Phase correction
          2. H(ancilla) - Hadamard basis measurement preparation
          3. MEASURE(ancilla) -> c[cbit] - Mid-circuit measurement
          4. IF (c[cbit] == 1): CX + CC(iZ) - Conditional correction (branch 1)
          5. IF (c[cbit] == 0): CS† - Conditional correction (branch 0, more economical)

        More efficient than RCCX variant when measurement outcome is 0.

        Args:
            input_path: Path to RC3X intermediate QASM file
            output_path: Path for output dynamic QASM file
            cbit: Classical bit for measurement result (default: 0)

        Returns:
            QASM string of the dynamic circuit
    )pbdoc",
    "input_path"_a, "output_path"_a, "cbit"_a = 0);

    py::native_enum<qcore::FileFormat>(m, "FileFormat", "enum.Enum")
            .value("Real", qcore::FileFormat::Real)
            .value("OpenQASM", qcore::FileFormat::OpenQASM)
            .value("OpenQASM3", qcore::FileFormat::OpenQASM3)
            .export_values()
            .finalize();

    py::native_enum<qcore::RotationType>(m, "RotationType", "enum.Enum")
            .value("ROTATENONE", qcore::rotation_t::ROTATENONE)
            .value("ROTATIONCOUNT", qcore::rotation_t::ROTATIONCOUNT)
            .value("THETA", qcore::rotation_t::THETA)
            .value("PHI", qcore::rotation_t::PHI)
            .value("LAMBDA", qcore::rotation_t::LAMBDA)
            .value("GAMMA", qcore::rotation_t::GAMMA)
            .export_values()
            .finalize();

    py::native_enum<qcore::ConditionType>(m, "ConditionType", "enum.Enum")
            .value("eq", qcore::ConditionType::eq)
            .value("neq", qcore::ConditionType::neq)
            .value("le", qcore::ConditionType::le)
            .value("lt", qcore::ConditionType::lt)
            .value("ge", qcore::ConditionType::ge)
            .value("gt", qcore::ConditionType::gt)
            .value("invalid", qcore::ConditionType::invalid)
            .export_values()
            .finalize();

    auto gate_types = py::native_enum<qcore::GateType>(m, "GateType", "enum.Enum");
    // https://en.cppreference.com/cpp/string/byte/toupper
    auto str_toupper = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); } // correct
        );
        return s;
    };
    for (qcore::GateType t = qcore::GateType::NONE; t != qcore::GateType::GATE_TYPE_COUNT;
         t = static_cast<qcore::GateType>(static_cast<uintptr_t>(t)+1)) {
	    gate_types.value(str_toupper(qcore::toString(t)).c_str(), t);
    }
    gate_types.export_values().finalize();

    py::classh<qcore::DecompositionConfiguration>(m, "DecompositionConfiguration")
        .def_readonly_static("ReduceTDepth", &qcore::DecompositionConfiguration::ReduceTDepth)
        .def_readonly_static("ReduceCNotCount", &qcore::DecompositionConfiguration::ReduceCNotCount)
        .def("withRelativePhase", &qcore::DecompositionConfiguration::withRelativePhase,
             py::arg("is_inverse"), py::arg("allow_dynamic_uncomputation") = false, py::arg("allow_target_error") = false);

    py::classh<qcore::AssertionInfo>(m, "AssertionInfo")
            .def("getArgument", &qcore::AssertionInfo::getArgument)
            .def("getQubits", &qcore::AssertionInfo::getQubits)
            .def("getCbits", &qcore::AssertionInfo::getCbits)
            .def("isValid", &qcore::AssertionInfo::isValid)
            .def("isSymbolic", &qcore::AssertionInfo::isSymbolic);

    py::classh<qcore::ClassicalCondition>(m, "ClassicalCondition")
            .def(py::init<const qcore::ConditionType, const std::string, const unsigned>())
            .def(py::init<>())
            .def("getType", &qcore::ClassicalCondition::getType)
            .def("getCbit", &qcore::ClassicalCondition::getCbit)
            .def("getValue", &qcore::ClassicalCondition::getValue);

    py::classh<qcore::QGate>(m, "QGate")
            .def(py::init<const qcore::GateType &, qcore::gsize_t, const qcore::TargetSet &>())
            .def(py::init<const qcore::GateType &, qcore::gsize_t, const qcore::ControlSet &, const qcore::TargetSet &>())
            .def(py::init<const qcore::GateType &, qcore::gsize_t, const qcore::RotationMap &, const qcore::ControlSet &, const qcore::TargetSet &>())
            .def(py::init<const qcore::GateType &, qcore::gsize_t, const qcore::RotationMap &, const qcore::TargetSet &>())
            .def("getId", &qcore::QGate::getId)
            .def("getType", &qcore::QGate::getType)
            .def("getControls", &qcore::QGate::getControls)
            .def("getTargets", &qcore::QGate::getTargets)
            .def("getCbits", &qcore::QGate::getCbits)
            .def("setCbits", &qcore::QGate::setCbits)
            .def("getAssertionInfo", &qcore::QGate::getAssertionInfo)
            .def("isAssertion", &qcore::QGate::isAssertion)
            .def("isAssumption", &qcore::QGate::isAssumption)
            .def("isClassical", &qcore::QGate::isClassical)
            .def("getClassical", &qcore::QGate::getClassical)
            .def("setClassical", py::overload_cast<const unsigned, const qcore::ConditionType &, const std::string &>(&qcore::QGate::setClassical))
            .def("getAngle", &qcore::QGate::getAngle);

    py::classh<qcore::QCircuit>(m, "QCircuit")
			.def(py::init<qcore::regsize_t, qcore::regsize_t>(), py::arg("num_qubits") = 0, py::arg("num_clbits") = 0)
			.def("getGates", py::overload_cast<>(&qcore::QCircuit::getGates))
            .def("setGates", &qcore::QCircuit::setGates)
            .def("getQubits", &qcore::QCircuit::getQubits)
            .def("getCbits", &qcore::QCircuit::getCbits)
            .def("inverse", &qcore::QCircuit::inverse)
            .def("readQCircuit", &qcore::QCircuit::readQCircuit)
            .def("writeQCircuit", &qcore::QCircuit::writeQCircuit)
            .def("readQCircuitFromString", &qcore::QCircuit::readQCircuitFromString)
            .def("toString", &qcore::QCircuit::toString)
            .def("getGateDefinitions", &qcore::QCircuit::getGateDefinitions);

	auto decomposition = m.def_submodule("decomposition");
	decomposition.def("CNX_Logarithmic_Mixed", qcore::decompose_CNX_Logarithmic_Mixed);
	decomposition.def("CNX_Linear_RCCX", qcore::decompose_CNX_Linear_RCCX);
	decomposition.def("CNX_Linear_RC3X", qcore::decompose_CNX_Linear_RC3X);
	decomposition.def("CNX_Tree_RC3X", qcore::decompose_CNX_Tree_RC3X);
	decomposition.def("resolve_meta_gates", qcore::decompose_meta_gates);
	decomposition.def("mcx", qcore::decompose_resolve_Toffoli);
	decomposition.def("resolve_assertions", qcore::insert_assertion_interpretation);
    decomposition.def("make_assert", []() -> qcore::QCircuit {
	  qcore::QCircuit qc(8, 0);
	  qc.setMaxNumAncilla(10);

	  qc.addQCircuit(qcore::decompose_resolve_Toffoli(qcore::DecompositionConfiguration::ReduceTDepth.withRelativePhase(false), {0, 1}, 2, {}, {}));
	  qc.addQCircuit(qcore::decompose_resolve_Toffoli(qcore::DecompositionConfiguration::ReduceTDepth, {2}, 3, {}, {}));
	  qc.addQCircuit(qcore::decompose_resolve_Toffoli(qcore::DecompositionConfiguration::ReduceTDepth.withRelativePhase(true), {0, 1}, 2, {}, {}));

	  qcore::AssertionInfo assertion;
	  assertion.type = qcore::equivalence;
	  assertion.argument = "ccx";
	  assertion.assert_qubits.push_back(0);
	  assertion.assert_qubits.push_back(1);
	  assertion.assert_qubits.push_back(3);
	  qcore::addGate(qcore::GateType::ASSERT_EQ, static_cast<size_t>(assertion.assert_qubits.size()),
					 {}, {}, {}, assertion.assert_qubits, {}, assertion, qc);
	  return qc;
	  });

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}

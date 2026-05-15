/*
 * This file is part of QCORE library which is released under the <LICENSE> license.
 * See file README.md or go to https://<LICENSE>/research/quantum/ for more information.
 */


#include <pybind11_json/pybind11_json.hpp>
#include <nlohmann/json.hpp>
#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "QCircuit.hpp"
#include "QGate.hpp"



#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)


namespace py = pybind11;
namespace nl = nlohmann;
using namespace pybind11::literals;

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
        ckt->readQASM(is);
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

PYBIND11_MODULE(pyqcore, m) {
    m.doc() = R"pbdoc(
        Python interface for the QCORE quantum core library
        ---------------------------------------------------
        .. autosummary::
           :toctree: _generate
           read_From_File
           readQASM
           circuitProperties
           writeQASM
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

    m.def("writeQASM", &writeQASM, "write quantum circuit to a file",
    "file_name");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
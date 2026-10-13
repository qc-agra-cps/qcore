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
 *  @file   qasm3_parser_test.cpp
 *  @brief  Unit Test for internal circuit representation
 *  @author Louis Krüger
 *  @date   11.06.2025
 ***********************************************************/

#include <fstream>

#include <gtest/gtest.h>

#include "QCircuit.hpp"
#include "QGate.hpp"

TEST(ReprTest, GateTest) {
    // Have a gate. Do things with it.
    qcore::QCircuit circuit = qcore::QCircuit(2,2);
    std::cout << circuit.toString(qcore::FileFormat::OpenQASM) << std::endl;
    qcore::QGate gate = qcore::QGate(qcore::GateType::X, 1, qcore::QubitSet{circuit.getQubits()[1]});

    std::ofstream file("test_resources/repr_output_gate_test.txt");
    if (file.is_open()) {
        circuit.writeQASM(file);
    } else {
        std::cerr << "Could not open file!" << std::endl;
    }

    file.close();
}
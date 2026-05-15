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
 *  @file   DecomposeTest.cpp
 *  @brief  Unit Test for Decompose
 *  @author Abhoy Kole
 *  @date   27.12.2025
 ***********************************************************/

#include <iostream>
#include <filesystem>


#include "QCircuit.hpp"
#include "decompose/Clifford_T.hpp"


using namespace qcore;

/*
int main() {
    qcore::QCircuit circuit(15,0);
    std::vector<QubitSet> clean_ancillas;
    std::vector<QubitSet> dirty_ancillas;
    
  
    //Ancilla required for a MCX gate: ((controls.size() - 2) / 2) + ((controls.size() - 2) % 2)
    
    //Required four ancillas
    circuit.addQGate(QGate(GateType::MCX, 11, (ControlSet){0,1,2,3,4,5,6,7,8,9}, {14}));
    QubitSet clean={10,11}; //Two clean (Not used by any gate) ancillas present 
    QubitSet dirty={12,13}; //Two dirty ancilla present
    clean_ancillas.push_back(clean);
    dirty_ancillas.push_back(dirty);

    QCircuit circuit_decompose(15,0);
    for (auto i = 0; i < circuit.getGates().size(); ++i) {
        if ((*circuit.getGates()[i]).getType() == GateType::CCX || (*circuit.getGates()[i]).getType() == GateType::MCX){
            QCircuit qc = decompose_MCT_Clifford_T((*circuit.getGates()[i]).getControls(),(*circuit.getGates()[i]).getTargets()[0],dirty_ancillas[i], clean_ancillas[i]);
            circuit_decompose.addQCircuit(qc);
        }
        else{
            circuit_decompose.addQGate(*circuit.getGates()[i]);
        }
    }
    circuit_decompose.writeQCircuit("./DecomposeTest.qasm");
    return 0;
}
*/

int main() {
    qcore::QCircuit circuit(8,0);
    std::vector<QubitSet> clean_ancillas;
    std::vector<QubitSet> dirty_ancillas;
    
    //Ancilla required for a MCX gate: ((controls.size() - 2) / 2) + ((controls.size() - 2) % 2)
    //Required two ancillas
    circuit.addQGate(QGate(GateType::MCX, 6, (ControlSet){0,1,2,3,4}, {7}));
    QubitSet clean={5}; //Two clean (Not used by any gate) ancillas present 
    QubitSet dirty={6}; //No dirty ancilla present
    clean_ancillas.push_back(clean);
    dirty_ancillas.push_back(dirty);

    //No ancilla required
    circuit.addQGate(QGate(GateType::CCX, 3, (ControlSet){1,5}, {7}));
    clean_ancillas.push_back({});
    dirty_ancillas.push_back({});

    //Reqired one ancillas
    circuit.addQGate(QGate(GateType::MCX, 5, (ControlSet){0,1,2,6}, {7}));
    clean_ancillas.push_back({}); //No clean ancilla present
    dirty_ancillas.push_back({5}); //One dirty ancilla present

    //Reqired one ancillas
    circuit.addQGate(QGate(GateType::CCX, 4, (ControlSet){1,2,5}, {7}));
    clean_ancillas.push_back({}); //No clean ancilla present
    dirty_ancillas.push_back({6}); //One dirty ancilla present

    QCircuit circuit_decompose(8,0);
    for (auto i = 0; i < circuit.getGates().size(); ++i) {
        if ((*circuit.getGates()[i]).getType() == GateType::CCX || (*circuit.getGates()[i]).getType() == GateType::MCX){
            QCircuit qc = decompose_MCT_Clifford_T((*circuit.getGates()[i]).getControls(),(*circuit.getGates()[i]).getTargets()[0],dirty_ancillas[i], clean_ancillas[i]);
            circuit_decompose.addQCircuit(qc);
            
        }
        else{
            circuit_decompose.addQGate(*circuit.getGates()[i]);
        }
    }
    circuit_decompose.writeQCircuit("./DecomposeTest.qasm");

    return 0;
}


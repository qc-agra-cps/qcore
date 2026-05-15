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
 *  @file   Clifford_T.cpp
 *  @brief  Instance Description for Quantum Gate Decomposition using Clifford+T Gates
 *  @author Abhoy Kole
 *  @date   04.05.2025
 ***********************************************************/

#include "decompose/Clifford_T.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace qcore {

QCircuit decompose_CCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});
    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    } else {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {c1}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {c1}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c1}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c1}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}

QCircuit decompose_RCCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});

    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));

    return qc;
}

QCircuit decompose_SRCCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});
    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    } else {
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}

QCircuit decompose_SSRCCX_Clifford_T(Qubit &c1, Qubit &c2, Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, t});
    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
    } else {
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::T, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {c2}));
        qc.addQGate(QGate(GateType::TDG, 1, {c2}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){t}, {c2}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}

QCircuit decompose_RC3X_Clifford_T(Qubit &c1, Qubit &c2, Qubit &c3, Qubit &t) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, c3, t});

    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));
    qc.addQGate(QGate(GateType::T, 1, {t}));
    qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
    qc.addQGate(QGate(GateType::TDG, 1, {t}));
    qc.addQGate(QGate(GateType::H, 1, {t}));

    return qc;
}

QCircuit decompose_SRC3X_Clifford_T(Qubit &c1, Qubit &c2, Qubit &c3, Qubit &t, bool inverse) {
    auto qc = QCircuit{};
    qc.setQubits({c1, c2, c3, t});

    if (inverse == false) {
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
    } else {
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c2}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c1}, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
        qc.addQGate(QGate(GateType::T, 1, {t}));
        qc.addQGate(QGate(GateType::CX, 2, (ControlSet){c3}, {t}));
        qc.addQGate(QGate(GateType::TDG, 1, {t}));
        qc.addQGate(QGate(GateType::H, 1, {t}));
    }

    return qc;
}

QCircuit decompose_MCT_Clifford_T(QubitSet &controls, Qubit target, QubitSet &dirty, QubitSet &clean, bool inverse) {
    auto qc = QCircuit{};
    /*printf("C=%d\nControls: { ",controls.size());
    for(auto q:controls)
        printf("%d ", q);
    printf("}, target: %d, clean: { ", target);
    for(auto q:clean)
        printf("%d ", q);
    printf("}, dirty: { ");
    for(auto q:dirty)
        printf("%d ", q);
    printf("}\n");*/
    if (controls.size() == 2) {
        qc = decompose_CCX_Clifford_T(controls[0], controls[1], target);
    } 
    else {
        qcore::regsize_t ancilla_need = ((controls.size() - 2) / 2) + ((controls.size() - 2) % 2);
        if (clean.size() + dirty.size() < ancilla_need) {
            throw QcoreException("[decompose_MCT_Clifford_T] decomposition error msg: ancilla required (" + std::to_string(ancilla_need) + ") is higher than available (" + std::to_string(clean.size() + dirty.size()) + ").");
        }
  
        QubitSet quibts = mergeQubits({controls, {target}, dirty, clean});
        qc.setQubits(quibts);
        
  
        //decompose using clean ancilla 
        auto p_caq = controls[0];
        auto controls_covered = 1;
        if (!clean.empty()){
            p_caq = clean[0];
            controls_covered = 0;
        }
        for (auto & caq: clean){
            if (controls.size() == 3 && controls_covered == 0){
                qc.addQCircuit(decompose_RCCX_Clifford_T(controls[0], controls[1], caq));
                controls_covered += 2; 
            }
            else if (controls.size() > 3 && controls_covered == 0){
                qc.addQCircuit(decompose_RC3X_Clifford_T(controls[0], controls[1], controls[2], caq));
                controls_covered += 3;
            }
            else if ((controls.size() - controls_covered) == 2) {
                qc.addQCircuit(decompose_RCCX_Clifford_T(p_caq, controls[controls_covered], caq));
                controls_covered += 1;
            }
            else if ((controls.size() - controls_covered) > 2) {
                qc.addQCircuit(decompose_RC3X_Clifford_T(p_caq, controls[controls_covered], controls[controls_covered + 1], caq));
                controls_covered += 2;
            }
            
            p_caq = caq;
            
            if ((controls.size() - controls_covered) == 1){
                break;
            }
        }
        auto qc_temp = qc; 
        //decompose using dirty ancilla
        if ((controls.size() - controls_covered) > 1){
            auto p_daq = dirty[0];
            auto qc2 = QCircuit{};
            auto qc3 = QCircuit{};
            auto qc_temp2 = QCircuit{};
            for (auto &daq: dirty){
                if (daq == dirty[0]){
                    qc3.addQCircuit(decompose_SSRCCX_Clifford_T(controls[controls_covered], daq, target));
                    controls_covered += 1; 
                }
                else if ((controls.size() - controls_covered) > 3) {
                    qc2.addQCircuit(decompose_SRC3X_Clifford_T(daq, controls[controls_covered], controls[controls_covered + 1], p_daq));
                    controls_covered += 2;
                }
                else if ((controls.size() - controls_covered) > 2) {
                    qc2.addQCircuit(decompose_SRCCX_Clifford_T(daq, controls[controls_covered], p_daq));
                    controls_covered += 1;
                }
                p_daq = daq;  
                if ((controls.size() - controls_covered) <= 2){
                    break;
                }    
                 
            }
            if ((controls.size() - controls_covered) == 1 ){
                qc_temp2 = qc2;
                qc2.addQCircuit(decompose_RCCX_Clifford_T(p_caq, controls[controls_covered], p_daq));
            }
            else if ((controls.size() - controls_covered) == 2) {
                qc_temp2 = qc2;
                qc2.addQCircuit(decompose_RC3X_Clifford_T(p_caq, controls[controls_covered] , controls[controls_covered + 1], p_daq));
            }
            else{
                throw QcoreException("[toString] Insufficient dirty ancilla, decomposition failed.");
            }
            qc2.addQCircuit(qc_temp2.inverse());
            qc.addQCircuit(qc3);
            qc.addQCircuit(qc2);
            qc.addQCircuit(qc3.inverse());
            qc.addQCircuit(qc2.inverse());                                    
        }
        else{
            if ((controls.size() - controls_covered) == 1) {
                qc.addQCircuit(decompose_CCX_Clifford_T(p_caq, controls[controls_covered], target));
            }
            else{
                throw QcoreException("[toString] Insufficient clean ancilla, decomposition failed.");
            }
        }
        qc.addQCircuit(qc_temp.inverse());
    }
    return qc;
}

}  // namespace qcore
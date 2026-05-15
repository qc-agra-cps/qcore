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
 *  @file   GateType.hpp
 *  @brief  Specification of Different Quantum Gates
 *  @author Abhoy Kole 
 *  @date   10.03.2022 
 ***********************************************************/
/*
======================================================
Author: Abhoy Kole
Created: 18th February, 2023
Last Modified: 18th February, 2023
Purpose: Reversible and Quantum GATE Instance Description
======================================================
*/
#pragma once

#include <iostream>

#include "Definition.hpp"

namespace qcore {


/**
 * @brief Reversible and Quantum Gate Types:
 * 
 * @details 
 *  Classical Gates:
 *      Pauli X, Controlled Pauli X (CX), Toffoli (CCX), Multiple Controlled Toffoli (MCX), SWAP, CSWAP
 *  Phase Gates:
 *      Pauli Z, sqrt of Z (S), inverse of S (SDG), sqrt of S (T), inverse of T (TDG), Phase (P), Rotation around Z axix (RZ)
 *  Non-unitory Gates:
 *      Active reset(RESET), measurement (MESURE), classical controlled operation (IF), BARRIER
 *  Hadamard Gate:
 *      H
 * Quantum Gates:
 *      Y, Rotation around X and Y axix (RX and RY), sqrt of X (SX), inverse of SX (SXDG), Rotation around XX axix (RXX), 
 *      Rotation around ZZ axis (RZZ), ISWAP 
 * Special Relative phase 3-qubit Toffoli Gate: 
 *      Margolus gate (RCCX), SRCCX, SRCCXDG,SSRCCX, SSRCCXDG
 * Special Relative phase 4-qubit Toffoli Gate:
 *      RC3X, SRC3X, SRC3XDG
 * Special unitaries from IBM:
 *      U1, U2, U3 and U
 */

enum GateType : std::uint8_t {  // Gate Type
    NONE,

    // Standard operations
    I,          //Identity Gate
    X,          //Inverter, NOT or  Pauli-X Gate
    CX,         //Controlled-Pauli-X or Feynman Gate 
    CCX,        //2-Controlled-Pauli-X or Toffoli Gate
    MCX,        //Multi-Controlled-Pauli-X or Multi-Controlled-Toffoli Gate

    //Quantum X-axis rotation and phase operations
    RX,         //Rotation about the X-axis through angle θ 
    CRX,        //Controlled-RX gate
    RXX,        //Two-qubit rotation along the X-axis through a specified angle or XX-Ising Gate 
    RCCX,       //Relative phase Toffoli or Margolus Gate 
    SRCCX,      //Sub or parts of RCCX Gate: {RCCX(a, b, c) + V(b, c)}  
    SRCCXDG,    //Inverse of SRCCX gate
    SSRCCX,     //Type-{b} Special form of RCCX Gate: { RCCX(a, b, c) + V(a, c)}
    SSRCCXDG,   //Inverse of SSRCCX Gate
    LCCX,       //Logical AND Gate
    LCCXDG,     //Inverse of LCCX Gate
    RC3X,       //3 Controlled relative phase X gate 
    SRC3X,      //Sub or parts of RC3X Gate: {RC3X(a, b, c, d) + V(b, c, d)}
    SRC3XDG,    //Inverse SRC3X Gate

    V,          //Square-Root-of-Paule-X Gate
    CV,         //Controlled-V Gate
    VDG,        //Inverse of V Gate
    CVDG,       //Congrolled-VDG Gate
    SX,         //Same as V
    CSX,        //Same as CV
    SXDG,       //Same as VDG
    CSXDG,      //Same as CVDG
    Y,          //Paule Y Gate
    CY,         //Controlled-Y Gate 
    RY,         //Rotation about the Y-axis through angle θ 
    CRY,        //Controlled-RY gate
    Z,          //Paule Z Gate
    CZ,         //Controlled-Z Gate 
    S,          //Square-Root-of-Paule-Z Gate
    CS,         //Controlled-S Gate
    SDG,        //Inverse of S Gate
    CSDG,       //Controlled-SDG Gate
    T,          //Square-Root-of-S Gate
    CT,         //Controlled-T Gate
    TDG,        //Inverse of T Gate
    CTDG,       //Controlled-TDG Gate
    P,          //Equivalent to RZ Gate (up to a phase factor, i.e. P(θ) = e^(iθ/2) RZ(θ), P(θ=π)=Z,  P(θ=π/2)=S, P(θ=π/4)=T)
    CP,         //Controlled-P Gate 
    RZ,         //Rotation about the Z-axis through angle θ 
    CRZ,        //Controlled-RZ Gate  
    RZZ,        //Two-qubit rotation along the Z-axis through a specified angle (maximally entangling at θ=π/2)
    H,          //Hadamard Gate
    CH,         //Controlled-H Gate  
    U1,         //Similar to P Gate (a deprecated gate from IBM Qiskit, U1(θ) = P(θ) = U(0, 0, θ))
    CU1,        //Controlled-U1 Gate  
    U2,         //Single-qubit rotation about the (X + Z) axis (a deprecated gate from IBM Qiskit, U2(ϕ,λ) = U(π/2, ϕ, λ))
    CU2,        //Controlled-U2 Gate  
    U3,         //Generic single-qubit rotation Gate with 3 Euler angles (a deprecated gate from IBM Qiskit, U3(θ, ϕ, λ) = U(θ, ϕ, λ))
    CU3,        //Controlled-U3 Gate
    U,          //Similar to U3 Gate
    CU,         //Controlled-U Gate
    PERES,      //A 3-qubit quantum Gate (realizing: P(a, b, c) -> (a', b', c') such that a' = a, b' = a ⊕ b, c' = (a & b) ⊕ c)  
    PERESDG,    //Inverse of PERES Gate
    SWAP,       //A 2-qubit Gate for state exchange
    CSWAP,      //Controlled-SWAP or Fredkin Gate
    ISWAP,      //A 2-qubit XX + YY interaction: iSWAP = e^(iπ/4(X ⊗ X + Y ⊗ Y))
    
    // Non-unitary operations
    RESET,
    MEASURE,
    IF,
    BARRIER,
    // Supported Gate Types
    TYPECOUNT
};

// Flag for classical control gate
using is_classical = bool;
using expression_t = std::string;

// Type for holding gate type
using gate_t = GateType;

// number of gates of each type
using PropertiesMap = std::map<gate_t, gcount_t>;
using PropertyIterator = PropertiesMap::iterator;
using ConstantPropertyIterator = PropertiesMap::const_iterator;

enum RotationType : std::uint8_t {  // Type of explicit rotations
    ROTATENONE,
    THETA,
    PHI,
    LAMBDA,
    GAMMA,
    ROTATIONCOUNT
};

// alias for rotation type
using rotation_t = RotationType;

// Collection for angle of rotation
using RotationMap = std::map<RotationType, angle_t>;

using RotationIterator = RotationMap::iterator;
using ConstantRotationIterator = RotationMap::const_iterator;

inline std::string toString(const rotation_t& rotationType) {
    switch (rotationType) {
        case RotationType::ROTATENONE:
            return "none";
        case RotationType::THETA:
            return "theta";
        case RotationType::PHI:
            return "phi";
        case RotationType::LAMBDA:
            return "lambda";
        case RotationType::GAMMA:
            return "gamma";
        default:
            throw std::invalid_argument("Invalid RotationType!");
    }
}

inline std::string toString(const gate_t& gateType) {
    switch (gateType) {
        case GateType::NONE:
            return "none";
        
        // Standard operations
        case GateType::I:
            return "id";
        case GateType::X:
            return "x";
        case GateType::CX:
            return "cx";
        case GateType::CCX:
            return "ccx";
        case GateType::MCX:
            return "mcx";
        
        //Quantum X-axis rotation and phase operations
        case GateType::RX:
            return "rx";
        case GateType::CRX:
            return "crx";
        case GateType::RXX:
            return "rxx";
        case GateType::RCCX:
            return "rccx";
        case GateType::SRCCX:
            return "srccx";
        case GateType::SRCCXDG:
            return "srccxdg";
        case GateType::SSRCCX:
            return "ssrccx";
        case GateType::SSRCCXDG:
            return "ssrccxdg";           
        case GateType::LCCX:
            return "lccx"; 
        case GateType::LCCXDG:
            return "lccxdg"; 
        case GateType::RC3X:
            return "rc3x";
        case GateType::SRC3X:
            return "src3x";
        case GateType::SRC3XDG:
            return "src3xdg";
        case GateType::V:
            return "v";
        case GateType::CV:
            return "cv";
        case GateType::VDG:
            return "vdg";
        case GateType::CVDG:
            return "cvdg";
        case GateType::SX:
            return "sx";
        case GateType::CSX:
            return "csx";
        case GateType::SXDG:
            return "sxdg";
        case GateType::CSXDG:
            return "csxdg";
        case GateType::Y:
            return "y";
        case GateType::CY:
            return "cy";
        case GateType::RY:
            return "ry";
        case GateType::CRY:
            return "cry";
        case GateType::Z:
            return "z";
        case GateType::CZ:
            return "cz";
        case GateType::S:
            return "s";
        case GateType::CS:
            return "cs";
        case GateType::SDG:
            return "sdg";
        case GateType::CSDG:
            return "csdg";
        case GateType::T:
            return "t";
        case GateType::CT:
            return "ct";
        case GateType::TDG:
            return "tdg";
        case GateType::CTDG:
            return "ctdg";
        case GateType::P:
            return "p";
        case GateType::CP:
            return "cp";
        case GateType::RZ:
            return "rz";
        case GateType::CRZ:
            return "crz";
        case GateType::RZZ:
            return "rzz";
        case GateType::H:
            return "h";
        case GateType::CH:
            return "ch";
        case GateType::U1:
            return "u1";
        case GateType::CU1:
            return "cu1";
        case GateType::U2:
            return "u2";
        case GateType::CU2:
            return "cu2";
        case GateType::U3:
            return "u3";
        case GateType::CU3:
            return "cu3";
        case GateType::U:
            return "u";
        case GateType::CU:
            return "cu";
        case GateType::PERES:
            return "peres";
        case GateType::PERESDG:
            return "peresdg";
        case GateType::SWAP:
            return "swap";
        case GateType::CSWAP:
            return "cswap";
        case GateType::ISWAP:
            return "iswap";
        case GateType::RESET:
            return "reset";
        case GateType::MEASURE:
            return "measure";
        case GateType::IF:
            return "if";
        case GateType::BARRIER:
            return "barrier";
        default:
            throw std::invalid_argument("Invalid OpType!");
    }
}

/**
 * @brief Obtaining the inverse of a given quantum gate type
 * 
 * @param gateType The gateType identifier for the given quantum gate type  
 * @return gate_t The inverse quantum gate type
 */
inline gate_t inverseGateType(const gate_t& gateType) {
    switch (gateType) {
        case GateType::SRCCX:
            return GateType::SRCCXDG;
        case GateType::SRCCXDG:
            return GateType::SRCCX;
        case GateType::SSRCCX:
            return GateType::SSRCCXDG;
        case GateType::SSRCCXDG:
            return GateType::SSRCCX;
        case GateType::SRC3X:
            return GateType::SRC3XDG;
        case GateType::SRC3XDG:
            return GateType::SRC3X;
        case GateType::LCCX:
            return GateType::LCCXDG;
        case GateType::LCCXDG:
            return GateType::LCCX;   
        case GateType::V:
            return GateType::VDG;
        case GateType::VDG:
            return GateType::V;
        case GateType::CV:
            return GateType::CVDG;
        case GateType::CVDG:
            return GateType::CV;    
        case GateType::SX:
            return GateType::SXDG;
        case GateType::SXDG:
            return GateType::SX;
        case GateType::CSX:
            return GateType::CSXDG;
        case GateType::CSXDG:
            return GateType::CSX;
        case GateType::S:
            return GateType::SDG;
        case GateType::SDG:
            return GateType::S;
        case GateType::CS:
            return GateType::CSDG;
        case GateType::CSDG:
            return GateType::CS;
        case GateType::T:
            return GateType::TDG;
        case GateType::TDG:
            return GateType::T;
        case GateType::CT:
            return GateType::CTDG;
        case GateType::CTDG:
            return GateType::CT;
        case GateType::PERES:
            return GateType::PERESDG;
        case GateType::PERESDG:
            return GateType::PERES;
        
        //Self Inverse
        case GateType::NONE:
        case GateType::I:
        case GateType::X:
        case GateType::CX:
        case GateType::CCX:
        case GateType::MCX:
        case GateType::RX:
        case GateType::CRX:
        case GateType::RXX:
        case GateType::RCCX:
        case GateType::RC3X:
        case GateType::Y:
        case GateType::CY:
        case GateType::RY:
        case GateType::CRY:
        case GateType::Z:
        case GateType::CZ:
        case GateType::P:
        case GateType::CP:
        case GateType::RZ:
        case GateType::CRZ:
        case GateType::RZZ:
        case GateType::H:
        case GateType::CH:
        case GateType::U1:
        case GateType::CU1:
        case GateType::U2:
        case GateType::CU2:
        case GateType::U3:
        case GateType::CU3:
        case GateType::U:
        case GateType::CU:
        case GateType::SWAP:
        case GateType::CSWAP:
        case GateType::ISWAP:
        case GateType::RESET:
        case GateType::MEASURE:
        case GateType::IF:
        case GateType::BARRIER:
            return gateType;
        default:
            throw std::invalid_argument("Invalid OpType!");
    }
}

inline gate_t gateTypeFromString(const std::string& gateType) {
    if (gateType == "none" || gateType == "0") {
        return GateType::NONE;
    }
    if (gateType == "i" || gateType == "id" || gateType == "1") {
        return GateType::I;
    }
    if (gateType == "x" || gateType == "2") {
        return GateType::X;
    }
    if (gateType == "cx" || gateType == "cnot" || gateType == "3") {
        return GateType::CX;
    }
    if (gateType == "ccx" || gateType == "c2x" || gateType == "4") {
        return GateType::CCX;
    }
    if (gateType == "mcx" || gateType == "5") {
        return GateType::MCX;
    }
    if (gateType == "rx" || gateType == "6") {
        return GateType::RX;
    }
    if (gateType == "crx" || gateType == "7") {
        return GateType::CRX;
    }
    if (gateType == "rxx" || gateType == "8") {
        return GateType::RXX;
    }
    if (gateType == "rccx" || gateType == "9") {
        return GateType::RCCX;
    }
    if (gateType == "srccx" || gateType == "10") {
        return GateType::SRCCX;
    }
    if (gateType == "srccxdg" || gateType == "11") {
        return GateType::SRCCXDG;
    }
    if (gateType == "ssrccx" || gateType == "12") {
        return GateType::SSRCCX;
    }
    if (gateType == "ssrccxdg" || gateType == "13") {
        return GateType::SSRCCXDG;
    }
    if (gateType == "lccx" || gateType == "14") {
        return GateType::LCCX;
    }
    if (gateType == "lccxdg" || gateType == "15") {
        return GateType::LCCXDG;
    }
    if (gateType == "rc3x" || gateType == "16") {
        return GateType::RC3X;
    }
    if (gateType == "src3x" || gateType == "17") {
        return GateType::SRC3X;
    }
    if (gateType == "src3xdg" || gateType == "18") {
        return GateType::SRC3XDG;
    }
    if (gateType == "v" || gateType == "19") {
        return GateType::V;
    }
    if (gateType == "cv" || gateType == "20") {
        return GateType::CV;
    }
    if (gateType == "vdg" || gateType == "21") {
        return GateType::VDG;
    }
    if (gateType == "cvdg" || gateType == "22") {
        return GateType::CVDG;
    }
    if (gateType == "sx" || gateType == "23") {
        return GateType::SX;
    }
    if (gateType == "csx" || gateType == "24") {
        return GateType::CSX;
    }
    if (gateType == "sxdg" || gateType == "25") {
        return GateType::SXDG;
    }
    if (gateType == "csxdg" || gateType == "26") {
        return GateType::CSXDG;
    }
    if (gateType == "y" || gateType == "27") {
        return GateType::Y;
    }
    if (gateType == "cy" || gateType == "28") {
        return GateType::CY;
    }
    if (gateType == "ry" || gateType == "29") {
        return GateType::RY;
    }
    if (gateType == "cry" || gateType == "30") {
        return GateType::CRY;
    }
    if (gateType == "z" || gateType == "31") {
        return GateType::Z;
    }
    if (gateType == "cz" || gateType == "32") {
        return GateType::CZ;
    }
    if (gateType == "s" || gateType == "33") {
        return GateType::S;
    }
    if (gateType == "cs" || gateType == "34") {
        return GateType::CS;
    }
    if (gateType == "sdg" || gateType == "35") {
        return GateType::SDG;
    }
    if (gateType == "csdg" || gateType == "36") {
        return GateType::CSDG;
    }
    if (gateType == "t" || gateType == "37") {
        return GateType::T;
    }
    if (gateType == "ct" || gateType == "38") {
        return GateType::CT;
    }
    if (gateType == "tdg" || gateType == "39") {
        return GateType::TDG;
    }
    if (gateType == "ctdg" || gateType == "40") {
        return GateType::CTDG;
    }
    if (gateType == "p" || gateType == "41") {
        return GateType::P;
    }
    if (gateType == "cp" || gateType == "42") {
        return GateType::CP;
    }
    if (gateType == "rz" || gateType == "43") {
        return GateType::RZ;
    }
    if (gateType == "crz" || gateType == "44") {
        return GateType::CRZ;
    }
    if (gateType == "rzz" || gateType == "45") {
        return GateType::RZZ;
    }
    if (gateType == "h" || gateType == "46") {
        return GateType::H;
    }
    if (gateType == "ch" || gateType == "47") {
        return GateType::CH;
    }
    if (gateType == "u1" || gateType == "48") {
        return GateType::U1;
    }
    if (gateType == "cu1" || gateType == "49") {
        return GateType::CU1;
    }
    if (gateType == "u2" || gateType == "50") {
        return GateType::U2;
    }
    if (gateType == "cu2" || gateType == "51") {
        return GateType::CU2;
    }
    if (gateType == "u3" || gateType == "52") {
        return GateType::U3;
    }
    if (gateType == "cu3" || gateType == "53") {
        return GateType::CU3;
    }
    if (gateType == "u" || gateType == "54") {
        return GateType::U;
    }
    if (gateType == "cu" || gateType == "55") {
        return GateType::CU;
    }
    if (gateType == "peres" || gateType == "56") {
        return GateType::PERES;
    }
    if (gateType == "peresdg" || gateType == "57") {
        return GateType::PERESDG;
    }
    if (gateType == "swap" || gateType == "58") {
        return GateType::SWAP;
    }
    if (gateType == "cswap" || gateType == "59") {
        return GateType::CSWAP;
    }
    if (gateType == "iswap" || gateType == "60") {
        return GateType::ISWAP;
    }
    if (gateType == "reset" || gateType == "61") {
        return GateType::RESET;
    }
    if (gateType == "measure" || gateType == "62") {
        return GateType::MEASURE;
    }
    if (gateType == "if" || gateType == "63") {
        return GateType::IF;
    }
    if (gateType == "barrier" || gateType == "64") {
        return GateType::BARRIER;
    }
    throw std::invalid_argument("Unknown operation type: " + gateType);
}

inline std::istream& operator>>(std::istream& in, gate_t& gateType) {
    std::string token;
    in >> token;

    if (token.empty()) {
        in.setstate(std::istream::failbit);
        return in;
    }

    gateType = gateTypeFromString(token);
    return in;
}

inline std::ostream& operator<<(std::ostream& out, gate_t& gateType) {
    out << toString(gateType);
    return out;
}

}  // namespace qcore
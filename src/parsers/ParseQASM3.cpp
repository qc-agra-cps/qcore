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
 *  @file   ParseQASM3.cpp
 *  @brief  Parser for QASM 3.0
 *  @author Majd Assaad
 *  @date   09.03.2026
 ***********************************************************/

#include "Definition.hpp"
#include "GateType.hpp"
#include "GateLibrary.hpp"
#include "QCircuit.hpp"
#include "QGate.hpp"

#include "parsers/Tokenizer.hpp"

#include <set>
#include <sstream>
#include <algorithm>
#include <functional>

namespace qcore {

// ───────────── Register info for tracking named registers ─────────────
struct RegisterInfo {
    std::uint64_t start_index; // first index in the global numbering
    std::uint64_t size;        // number of bits in this register
};

// ───────────── Parser class ─────────────
class QASM3Parser {
    const std::vector<Token>& tokens;
    size_t pos = 0;
    QCircuit& circuit;
    const GateLibrary* library;

    // register tracking
    std::map<std::string, RegisterInfo> qubit_registers;
    std::map<std::string, RegisterInfo> cbit_registers;
    std::uint64_t next_qubit_index = 0;
    std::uint64_t next_cbit_index = 0;

    // custom gate definitions
    std::map<std::string, GateDefinition> gate_defs;

    // cache: (gate name, params used) -> gate body with those params already substituted in
    std::map<std::pair<std::string, std::vector<std::string>>, std::shared_ptr<QCircuit>> specialized_gate_bodies;

    // built-in gate name lookup
    static std::map<std::string, GateType> gateType_map;

    // ───────────── Token navigation ─────────────
    const Token& peek(size_t offset = 0) const {
        size_t idx = pos + offset;
        if (idx >= tokens.size()) return tokens.back();
        return tokens[idx];
    }

    const Token& advance() {
        const Token& t = tokens[pos];
        if (pos < tokens.size() - 1) ++pos;
        return t;
    }

    bool check(TokenType type) const { return peek().type == type; }

    bool checkIdent(const std::string& name) const {
        return peek().type == TokenType::IDENTIFIER && peek().value == name;
    }

    const Token& expect(TokenType type, const std::string& context = "") {
        if (peek().type != type) {
            throw QcoreException("[readQASM3] l:" + std::to_string(peek().line) +
                " expected " + context + " but got '" + peek().value + "'");
        }
        return advance();
    }

    void expectSemicolon() { expect(TokenType::SEMICOLON, ";"); }

    bool match(TokenType type) {
        if (peek().type == type) { advance(); return true; }
        return false;
    }

    bool matchIdent(const std::string& name) {
        if (checkIdent(name)) { advance(); return true; }
        return false;
    }

    int currentLine() const { return peek().line; }

    // ───────────── Angle expression parsing ─────────────
    // Parses a simple arithmetic expression for gate parameters.
    // Supports: numbers, pi, identifiers (formal params), +, -, *, /
    std::string parseAngleExpr() {
        std::string result;
        int paren_depth = 0;

        while (true) {
            const Token& t = peek();

            if (t.type == TokenType::NUMBER) {
                result += advance().value;
            } else if (t.type == TokenType::IDENTIFIER && t.value == "pi") {
                result += advance().value;
            } else if (t.type == TokenType::IDENTIFIER) {
                // could be a formal parameter name
                result += advance().value;
            } else if (t.type == TokenType::PLUS) {
                result += advance().value;
            } else if (t.type == TokenType::MINUS) {
                result += advance().value;
            } else if (t.type == TokenType::STAR) {
                result += advance().value;
            } else if (t.type == TokenType::SLASH) {
                result += advance().value;
            } else if (t.type == TokenType::LPAREN) {
                result += advance().value;
                ++paren_depth;
            } else if (t.type == TokenType::RPAREN && paren_depth > 0) {
                result += advance().value;
                --paren_depth;
            } else {
                break;
            }
        }
        return result;
    }

    // Parse a comma-separated list of angle expressions inside ( )
    std::vector<std::string> parseParamList() {
        std::vector<std::string> params;
        if (!match(TokenType::LPAREN)) return params;

        if (!check(TokenType::RPAREN)) {
            params.push_back(parseAngleExpr());
            while (match(TokenType::COMMA)) {
                params.push_back(parseAngleExpr());
            }
        }
        expect(TokenType::RPAREN, ")");
        return params;
    }

    // ───────────── Qubit argument resolution ─────────────
    // Parse a qubit reference like q[0] or q and return the global qubit index(es)
    QubitSet resolveQubitArg() {
        std::string name = expect(TokenType::IDENTIFIER, "qubit name").value;

        if (match(TokenType::LBRACKET)) {
            // q[i]
            std::string idx_str = expect(TokenType::NUMBER, "qubit index").value;
            expect(TokenType::RBRACKET, "]");
            auto it = qubit_registers.find(name);
            if (it == qubit_registers.end()) {
                throw QcoreException("[readQASM3] l:" + std::to_string(currentLine()) +
                    " undeclared qubit register: " + name);
            }
            Qubit idx = std::stoull(idx_str);
            return {it->second.start_index + idx};
        }

        // bare name -- could be a single qubit or whole register
        auto it = qubit_registers.find(name);
        if (it != qubit_registers.end()) {
            if (it->second.size == 1) {
                return {it->second.start_index};
            }
            // return all qubits in register
            QubitSet qs;
            for (std::uint64_t j = 0; j < it->second.size; ++j)
                qs.push_back(it->second.start_index + j);
            return qs;
        }

        throw QcoreException("[readQASM3] l:" + std::to_string(currentLine()) +
            " undeclared qubit: " + name);
    }

    // Parse a cbit reference like c[0] or c
    Cbit resolveCbitArg() {
        std::string name = expect(TokenType::IDENTIFIER, "cbit name").value;

        if (match(TokenType::LBRACKET)) {
            std::string idx_str = expect(TokenType::NUMBER, "cbit index").value;
            expect(TokenType::RBRACKET, "]");
            auto it = cbit_registers.find(name);
            if (it == cbit_registers.end()) {
                throw QcoreException("[readQASM3] l:" + std::to_string(currentLine()) +
                    " undeclared cbit register: " + name);
            }
            Cbit idx = static_cast<Cbit>(std::stoul(idx_str));
            return static_cast<Cbit>(it->second.start_index + idx);
        }

        auto it = cbit_registers.find(name);
        if (it != cbit_registers.end() && it->second.size == 1) {
            return static_cast<Cbit>(it->second.start_index);
        }

        throw QcoreException("[readQASM3] l:" + std::to_string(currentLine()) +
            " undeclared cbit: " + name);
    }

    // Parse comma-separated qubit arguments: q[0], q[1], ...
    QubitSet parseQubitArgList() {
        QubitSet result;
        auto qs = resolveQubitArg();
        result.insert(result.end(), qs.begin(), qs.end());
        while (match(TokenType::COMMA)) {
            qs = resolveQubitArg();
            result.insert(result.end(), qs.begin(), qs.end());
        }
        return result;
    }

    // ───────────── Cbit expression for conditions ─────────────
    // Parse: (c[0]) or (c[0] == 1) or (creg == val)
    // Returns the condition expression string
    ClassicalCondition parseCondition() {
        expect(TokenType::LPAREN, "(");

        // read the lhs: could be c[0] or creg_name
        std::string lhs;
        std::string name = expect(TokenType::IDENTIFIER, "condition variable").value;
        if (match(TokenType::LBRACKET)) {
            std::string idx = expect(TokenType::NUMBER, "index").value;
            expect(TokenType::RBRACKET, "]");
            lhs = name + "[" + idx + "]";
        } else {
            lhs = name;
        }

        if (match(TokenType::EQEQ)) {
            std::string rhs = expect(TokenType::NUMBER, "condition value").value;
            expect(TokenType::RPAREN, ")");
            auto rhs_number = static_cast<unsigned>(std::stoul(rhs));
            return ClassicalCondition{ConditionType::eq, lhs, rhs_number};
        }

        // no == means boolean test: if (c[0]) equivalent to if (c[0] == 1)
        expect(TokenType::RPAREN, ")");
        return ClassicalCondition{ConditionType::eq, lhs, 1};
    }

    // Negate a simple condition: "c[0]==1" -> "c[0]==0" and vice versa
    static ClassicalCondition negateCondition(const ClassicalCondition& cond) {
        return cond.negate();
    }

    // ───────────── Parse & emit a single gate statement ─────────────
    // This handles: gate_name(params) qubit_args ;
    // Also handles measure and reset as gate statements.
    void parseGateStatement(const ClassicalCondition& cond) {
        std::string gate_name = peek().value;
        int line = currentLine();

        AssertionInfo assertion_empty;
        // measure with arrow syntax: measure q[0] -> c[0];
        if (gate_name == "measure") {
            advance(); // consume "measure"
            QubitSet qubits = resolveQubitArg();
            expect(TokenType::ARROW, "->");

            // resolve cbit target
            CbitSet cbits_out;
            std::string cname = expect(TokenType::IDENTIFIER, "cbit name").value;
            if (match(TokenType::LBRACKET)) {
                std::string idx = expect(TokenType::NUMBER, "cbit index").value;
                expect(TokenType::RBRACKET, "]");
                auto it = cbit_registers.find(cname);
                if (it == cbit_registers.end()) {
                    throw QcoreException("[readQASM3] l:" + std::to_string(line) +
                        " undeclared cbit register: " + cname);
                }
                cbits_out.push_back(static_cast<Cbit>(it->second.start_index + std::stoul(idx)));
            } else {
                auto it = cbit_registers.find(cname);
                if (it != cbit_registers.end()) {
                    cbits_out.push_back(static_cast<Cbit>(it->second.start_index));
                }
            }

            expectSemicolon();
            addGate(GateType::MEASURE, static_cast<gsize_t>(qubits.size()), cbits_out,
                    {}, {}, qubits, cond, assertion_empty, circuit);
            return;
        }

        // reset q[0];
        if (gate_name == "reset") {
            advance();
            QubitSet qubits = parseQubitArgList();
            expectSemicolon();
            for (auto q : qubits) {
                addGate(GateType::RESET, 1, {}, {}, {}, {q}, cond, assertion_empty, circuit);
            }
            return;
        }

        // barrier q[0], q[1];
        if (gate_name == "barrier") {
            advance();
            QubitSet qubits = parseQubitArgList();
            expectSemicolon();
            addGate(GateType::BARRIER, static_cast<gsize_t>(qubits.size()),
                    {}, {}, {}, qubits, cond, assertion_empty, circuit);
            return;
        }

        // assert quantum state equivalence
        if (gate_name == "assert_eq") {
            advance();
            Token ref_gate = expect(TokenType::IDENTIFIER, "assert_eq reference gate");
            expectSemicolon();
            // assert_eq's reference gate is just a name, never an actual gate call, so
            // pull it in from the library here -- otherwise it'd never get registered
            ensureGateDefinitionAvailable(ref_gate.value);
            AssertionInfo assertion;
            assertion.type = equivalence;
            assertion.argument = ref_gate.value;
            for(unsigned i=0;i<circuit.getQregSize(); ++i) {
                assertion.assert_qubits.push_back(i);
            }
            addGate(GateType::ASSERT_EQ, static_cast<gsize_t>(assertion.assert_qubits.size()),
                    {}, {}, {}, assertion.assert_qubits, cond, assertion, circuit);
            return;
        }

        // assert classical state
        if (gate_name == "assert_c") {
            advance();
            QubitSet qubits = parseQubitArgList();
            expect(TokenType::EQEQ, "assert_c =");
            AssertionInfo assertion;
            if (peek().type == TokenType::NUMBER) {
                Token state = expect(TokenType::NUMBER, "assert_c state");
                assertion.type = classic_concrete;
                assertion.argument = state.value;
            } else {
                Token state = expect(TokenType::IDENTIFIER, "assert_c symbolic state");
                assertion.type = classic_symbolic;
                assertion.argument = state.value;
            }
            expectSemicolon();
            for(Qubit qubit : qubits) {
                assertion.assert_qubits.push_back(qubit);
            }
            addGate(GateType::ASSERT_C, static_cast<gsize_t>(assertion.assert_qubits.size()),
                    {}, {}, {}, assertion.assert_qubits, cond, assertion, circuit);
            return;
        }

        // assert superposition
        if (gate_name == "assert_s") {
            advance();
            QubitSet qubits = parseQubitArgList();
            expectSemicolon();

            AssertionInfo assertion;
            // TODO |+> or |->
            assertion.type = superposition_concrete;
            for(Qubit qubit : qubits) {
                assertion.assert_qubits.push_back(qubit);
            }
            addGate(GateType::ASSERT_S, static_cast<gsize_t>(assertion.assert_qubits.size()),
                    {}, {}, {}, assertion.assert_qubits, cond, assertion, circuit);
            return;
        }

        // assert phase
        if (gate_name == "assert_phase") {
            advance();
            QubitSet qubits = parseQubitArgList();
            if(qubits.size() != 1) {
                throw QcoreException("[readQASM3] l:" + std::to_string(line) +
                                     " assert_phase expects exactly one qubit, given: " + std::to_string(qubits.size()));
            }
            expect(TokenType::EQEQ, "assert_phase =");
            std::string expected_phase = parseAngleExpr();
            expectSemicolon();

            AssertionInfo assertion;
            assertion.type = phase_symbolic; // TODO register assume names + check here for phase_concrete if no registered names
            assertion.argument = expected_phase;
            assertion.assert_qubits.push_back(qubits[0]);
            addGate(GateType::ASSERT_PHASE, static_cast<gsize_t>(assertion.assert_qubits.size()),
                    {}, {}, {}, assertion.assert_qubits, cond, assertion, circuit);
            return;
        }

        // assume classic state (0/1/symbol name)
        if (gate_name == "assume_classic") {
            advance();
            QubitSet qubits = parseQubitArgList();
            if(qubits.size() != 1) {
                throw QcoreException("[readQASM3] l:" + std::to_string(line) +
                                     " assume expects exactly one qubit, given: " + std::to_string(qubits.size()));
            }
            expect(TokenType::EQEQ, "assume_classic =");
            std::string expected_state;
            if(peek().type == TokenType::NUMBER) {
                expected_state = expect(TokenType::NUMBER).value;
            } else {
                expected_state = expect(TokenType::IDENTIFIER).value;
            }
            expectSemicolon();

            AssertionInfo assertion;
            assertion.type = assume_classic;
            assertion.argument = expected_state;
            assertion.assert_qubits.push_back(qubits[0]);
            addGate(GateType::ASSUME_CLASSIC, static_cast<gsize_t>(assertion.assert_qubits.size()),
                    {}, {}, {}, assertion.assert_qubits, cond, assertion, circuit);
            return;
        }

        // assume phase
        if (gate_name == "assume_phase") {
            advance();
            QubitSet qubits = parseQubitArgList();
            if(qubits.size() != 1) {
                throw QcoreException("[readQASM3] l:" + std::to_string(line) +
                                     " assume expects exactly one qubit, given: " + std::to_string(qubits.size()));
            }
            expect(TokenType::EQEQ, "assume_phase =");
            std::string expected_phase = parseAngleExpr();
            expectSemicolon();

            AssertionInfo assertion;
            assertion.type = assume_phase;
            assertion.argument = expected_phase;
            assertion.assert_qubits.push_back(qubits[0]);
            addGate(GateType::ASSUME_PHASE, static_cast<gsize_t>(assertion.assert_qubits.size()),
                    {}, {}, {}, assertion.assert_qubits, cond, assertion, circuit);
            return;
        }

        // declare symbolic symbol
        if (gate_name == "declare") {
            advance();
            std::string symbol_name = expect(TokenType::IDENTIFIER).value;
            expectSemicolon();

            AssertionInfo assertion;
            assertion.type = declare;
            assertion.argument = symbol_name;
            addGate(GateType::DECLARE, static_cast<gsize_t>(assertion.assert_qubits.size()),
                    {}, {}, {}, assertion.assert_qubits, cond, assertion, circuit);
            return;
        }

        // Try to look up as a built-in gate or custom gate
        advance(); // consume gate name

        // parse optional parameters
        std::vector<std::string> params = parseParamList();

        // parse qubit arguments
        QubitSet qubits = parseQubitArgList();
        expectSemicolon();

        // Check if it's a custom gate definition (local first, then library)
        if (ensureGateDefinitionAvailable(gate_name)) {
            expandCustomGate(gate_defs[gate_name], params, qubits, cond);
            return;
        }

        // Check the shared gate library for a pre-compiled (non-token-based) gate body
        if (library) {
            auto lib_body = library->findCompiledGate(gate_name);
            if (lib_body) {
                gsize_t gsize = static_cast<gsize_t>(qubits.size());
                TargetSet targets(qubits.begin(), qubits.end());
                auto gatePtr = QGatePtr(new QGate(GateType::SUBCIRCUIT, gsize, ControlSet{}, targets));
                gatePtr->setCustomGateName(gate_name);
                gatePtr->setSubcircuit(lib_body);
                if (cond.isActive()) {
                    gatePtr->setClassical(cond);
                }
                if (circuit.getMaxGateSize() < gsize) circuit.setMaxGateSize(gsize);
                circuit.setProperty(GateType::SUBCIRCUIT, circuit.getProperty(GateType::SUBCIRCUIT)+1);
                circuit.addGate(gatePtr);
                return;
            }
        }

        // Check multi-controlled X: c3x, c4x, ..., cnx
        std::string lookup_name = gate_name;
        bool is_mcx = false;
        if (gate_name.size() >= 3 && gate_name[0] == 'c' && gate_name.back() == 'x') {
            std::string middle = gate_name.substr(1, gate_name.size() - 2);
            bool all_digits = !middle.empty() && std::all_of(middle.begin(), middle.end(),
                [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); });
            if (all_digits) {
                lookup_name = "cnx";
                is_mcx = true;
            }
        }

        auto it = gateType_map.find(lookup_name);
        if (it == gateType_map.end()) {
            throw QcoreException("[readQASM3] l:" + std::to_string(line) +
                " unrecognized gate: " + gate_name);
        }

        gate_t gate_type = it->second;
        gsize_t gate_size = static_cast<gsize_t>(qubits.size());

        ControlSet controls{};
        TargetSet targets{};
        RotationMap angles{};

        // assign rotation angles based on gate type
        if (!params.empty()) {
            if (gate_type == GateType::U1 || gate_type == GateType::CU1) {
                if (params.size() >= 1) angles[RotationType::LAMBDA] = params[0];
            } else if (gate_type == GateType::U2 || gate_type == GateType::CU2) {
                if (params.size() >= 1) angles[RotationType::PHI] = params[0];
                if (params.size() >= 2) angles[RotationType::LAMBDA] = params[1];
            } else if (gate_type == GateType::U3 || gate_type == GateType::CU3 || gate_type == GateType::U || gate_type == GateType::CU) {
                if (params.size() >= 1) angles[RotationType::THETA] = params[0];
                if (params.size() >= 2) angles[RotationType::PHI] = params[1];
                if (params.size() >= 3) angles[RotationType::LAMBDA] = params[2];
                if (params.size() >= 4) angles[RotationType::GAMMA] = params[3];
            } else {
                // generic: theta, phi, lambda, gamma
                if (params.size() >= 1) angles[RotationType::THETA] = params[0];
                if (params.size() >= 2) angles[RotationType::PHI] = params[1];
                if (params.size() >= 3) angles[RotationType::LAMBDA] = params[2];
                if (params.size() >= 4) angles[RotationType::GAMMA] = params[3];
            }
        }

        // split qubits into controls and targets
        if (gate_type == GateType::SWAP || gate_type == GateType::ISWAP ||
            gate_type == GateType::RXX || gate_type == GateType::RZZ ||
            gate_type == GateType::BARRIER) {
            targets.insert(targets.begin(), qubits.begin(), qubits.end());
        } else if (gate_type == GateType::CSWAP) {
            controls.insert(controls.begin(), qubits.begin(), qubits.begin() + 1);
            targets.insert(targets.begin(), qubits.begin() + 1, qubits.end());
        } else {
            // last qubit is target, rest are controls
            if (qubits.size() > 1) {
                controls.insert(controls.begin(), qubits.begin(), qubits.end() - 1);
            }
            targets.insert(targets.begin(), qubits.end() - 1, qubits.end());
        }

        addGate(gate_type, gate_size, {}, angles, controls, targets, cond, assertion_empty, circuit);
    }

    // Copies a library-only gate into this circuit's local defs on first use. Call this
    // anywhere a gate name needs to resolve later, not just on actual gate calls (e.g.
    // assert_eq needs it too). Returns true if the name is now locally defined.
    bool ensureGateDefinitionAvailable(const std::string& gate_name) {
        if (gate_defs.find(gate_name) != gate_defs.end()) {
            return true;
        }
        if (library) {
            const GateDefinition* lib_def = library->findDefinition(gate_name);
            if (lib_def) {
                gate_defs[gate_name] = *lib_def;
                circuit.defineGateTxt(gate_name, *lib_def);
                circuit.defineGate(gate_name, parseCustomGate(*lib_def));
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<QCircuit> parseCustomGate(const GateDefinition& def) {
        // Build the body QCircuit using local qubit indices (0..n-1)
        auto body = std::make_shared<QCircuit>(
                static_cast<regsize_t>(def.qargs.size()), 0);

        // Build local qubit register: formal name -> local index
        std::map<std::string, RegisterInfo> local_qregs;
        for (size_t i = 0; i < def.qargs.size(); ++i) {
            local_qregs[def.qargs[i]] = {i, 1};
        }

        // Parse body tokens into the body QCircuit
        parseBodyTokensInto(def.body, local_qregs, *body);
        return body;
    }

    // Replace formal parameter identifiers (e.g. "theta") in a gate body's tokens with the
    // actual call-site expression text. Operates on a copy, never mutates `body` in place,
    // since the caller's token storage (gate_defs[name] / the shared GateLibrary) is reused
    // across every call to the same gate.
    std::vector<Token> substituteFormalParams(const std::vector<Token>& body,
                                               const std::vector<std::string>& formal,
                                               const std::vector<std::string>& actual) {
        std::map<std::string, std::string> subst;
        for (size_t i = 0; i < formal.size(); ++i) {
            subst[formal[i]] = "(" + actual[i] + ")";
        }
        std::vector<Token> out = body;
        for (auto& tok : out) {
            if (tok.type == TokenType::IDENTIFIER) {
                auto it = subst.find(tok.value);
                if (it != subst.end()) tok.value = it->second;
            }
        }
        return out;
    }

    // ───────────── Custom gate: emit as SUBCIRCUIT ─────────────
    void expandCustomGate(const GateDefinition& def,
                          const std::vector<std::string>& actual_params,
                          const QubitSet& actual_qubits,
                          const ClassicalCondition& cond) {
        if (actual_qubits.size() != def.qargs.size()) {
            throw QcoreException("[readQASM3] custom gate '" + def.name +
                "' expects " + std::to_string(def.qargs.size()) +
                " qubits but got " + std::to_string(actual_qubits.size()));
        }
        // TODO add actual parameter to subcircuit in some way
        // given that the custom gate is emitted as a subcircuit, we cannot replace the
        // parameters in the actual gate definition (except if we redefine the custom gate once for every single call)
        // qgate does not have the params attribute
        // qcircuit does not make sense
        // OLD: circuit.addSubcircuitGate(def.name, actual_qubits, false, cond);

        // NEW: fast path -- unchanged behavior for the common non-parameterized case
        if (def.params.empty()) {
            circuit.addSubcircuitGate(def.name, actual_qubits, false, cond);
            return;
        }
        if (actual_params.size() != def.params.size()) {
            throw QcoreException("[readQASM3] custom gate '" + def.name +
                "' expects " + std::to_string(def.params.size()) +
                " parameters but got " + std::to_string(actual_params.size()));
        }

        // NEW: build (and cache) a body bound to these actual values. Attached directly
        // rather than via defineGate()/defineGateTxt(), which would make writeQASM3() print
        // a spurious extra "gate <name> {...}" block per call (see branches above for the
        // same direct-attach pattern).
        auto key = std::make_pair(def.name, actual_params);
        auto cached = specialized_gate_bodies.find(key);
        std::shared_ptr<QCircuit> body;
        if (cached != specialized_gate_bodies.end()) {
            body = cached->second;
        } else {
            GateDefinition specialized_def = def;
            specialized_def.body = substituteFormalParams(def.body, def.params, actual_params);
            body = parseCustomGate(specialized_def);
            specialized_gate_bodies[key] = body;
        }

        gsize_t gsize = static_cast<gsize_t>(actual_qubits.size());
        TargetSet targets(actual_qubits.begin(), actual_qubits.end());
        auto gatePtr = QGatePtr(new QGate(GateType::SUBCIRCUIT, gsize, ControlSet{}, targets));
        gatePtr->setCustomGateName(def.name);
        gatePtr->setSubcircuit(body);
        gatePtr->setCallParams(actual_params);
        if (cond.isActive()) {
            gatePtr->setClassical(cond);
        }
        if (circuit.getMaxGateSize() < gsize) circuit.setMaxGateSize(gsize);
        circuit.setProperty(GateType::SUBCIRCUIT, circuit.getProperty(GateType::SUBCIRCUIT)+1);
        circuit.addGate(gatePtr);
    }

    // Parse gate statements from body tokens into a target QCircuit (using local qubit registers)
    void parseBodyTokensInto(const std::vector<Token>& body_tokens,
                             const std::map<std::string, RegisterInfo>& local_qregs,
                             QCircuit& target) {
        size_t bpos = 0;

        auto bpeek = [&](size_t offset = 0) -> const Token& {
            size_t idx = bpos + offset;
            if (idx >= body_tokens.size()) return body_tokens.back();
            return body_tokens[idx];
        };

        auto badvance = [&]() -> const Token& {
            const Token& t = body_tokens[bpos];
            if (bpos < body_tokens.size() - 1) ++bpos;
            return t;
        };

        auto bmatch = [&](TokenType type) -> bool {
            if (bpeek().type == type) { badvance(); return true; }
            return false;
        };

        while (bpeek().type != TokenType::END_OF_FILE) {
            if (bpeek().type != TokenType::IDENTIFIER) break;

            std::string gname = badvance().value;

            // parse params
            std::vector<std::string> params;
            if (bmatch(TokenType::LPAREN)) {
                std::string expr;
                int depth = 0;
                while (true) {
                    const Token& t = bpeek();
                    if (t.type == TokenType::RPAREN && depth == 0) { badvance(); break; }
                    if (t.type == TokenType::LPAREN) depth++;
                    if (t.type == TokenType::RPAREN) depth--;
                    if (t.type == TokenType::COMMA && depth == 0) {
                        params.push_back(expr);
                        expr.clear();
                        badvance();
                        continue;
                    }
                    if (t.type == TokenType::END_OF_FILE) break;
                    expr += badvance().value;
                }
                if (!expr.empty()) params.push_back(expr);
            }

            // parse qubit args using local register mapping
            QubitSet qubits;
            if (bpeek().type == TokenType::IDENTIFIER) {
                std::string qname = badvance().value;
                auto qit = local_qregs.find(qname);
                if (qit != local_qregs.end()) {
                    qubits.push_back(qit->second.start_index);
                }
                while (bmatch(TokenType::COMMA)) {
                    if (bpeek().type == TokenType::IDENTIFIER) {
                        qname = badvance().value;
                        qit = local_qregs.find(qname);
                        if (qit != local_qregs.end()) {
                            qubits.push_back(qit->second.start_index);
                        }
                    }
                }
            }

            bmatch(TokenType::SEMICOLON);

            // Check if it's a nested custom gate -- emit as nested SUBCIRCUIT
            // Look up in local gate_defs first, then in the shared library
            const GateDefinition* found_def = nullptr;
            auto custom_it = gate_defs.find(gname);
            if (custom_it != gate_defs.end()) {
                found_def = &custom_it->second;
            } else if (library) {
                found_def = library->findDefinition(gname);
            }
            if (found_def) {
                // For nested custom gates, build a sub-body and add SUBCIRCUIT to target
                const auto& nested_def = *found_def;
                if (qubits.size() == nested_def.qargs.size()) {
                    auto nested_body = std::make_shared<QCircuit>(
                        static_cast<regsize_t>(nested_def.qargs.size()), 0);
                    std::map<std::string, RegisterInfo> nested_local;
                    for (size_t i = 0; i < nested_def.qargs.size(); ++i) {
                        nested_local[nested_def.qargs[i]] = {i, 1};
                    }
                    // Substitute params in nested body
                    std::map<std::string, std::string> nested_param_map;
                    for (size_t i = 0; i < nested_def.params.size() && i < params.size(); ++i) {
                        nested_param_map[nested_def.params[i]] = params[i];
                    }
                    std::vector<Token> nested_tokens = nested_def.body;
                    nested_tokens.push_back({TokenType::END_OF_FILE, "", 0});
                    for (auto& tok : nested_tokens) {
                        if (tok.type == TokenType::IDENTIFIER) {
                            auto pit = nested_param_map.find(tok.value);
                            if (pit != nested_param_map.end()) tok.value = pit->second;
                        }
                    }
                    parseBodyTokensInto(nested_tokens, nested_local, *nested_body);

                    gsize_t gs = static_cast<gsize_t>(qubits.size());
                    TargetSet tgts(qubits.begin(), qubits.end());
                    auto gp = QGatePtr(new QGate(GateType::SUBCIRCUIT, gs, ControlSet{}, tgts));
                    gp->setCustomGateName(nested_def.name);
                    gp->setSubcircuit(nested_body);
                    target.addGate(gp);
                }
                continue;
            }

            // Check library for pre-compiled gates
            if (library) {
                auto lib_body = library->findCompiledGate(gname);
                if (lib_body) {
                    gsize_t gs = static_cast<gsize_t>(qubits.size());
                    TargetSet tgts(qubits.begin(), qubits.end());
                    auto gp = QGatePtr(new QGate(GateType::SUBCIRCUIT, gs, ControlSet{}, tgts));
                    gp->setCustomGateName(gname);
                    gp->setSubcircuit(lib_body);
                    target.addGate(gp);
                    continue;
                }
            }

            // Look up built-in gate
            std::string lookup = gname;
            if (gname.size() >= 3 && gname[0] == 'c' && gname.back() == 'x') {
                std::string mid = gname.substr(1, gname.size() - 2);
                if (!mid.empty() && std::all_of(mid.begin(), mid.end(),
                    [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); })) {
                    lookup = "cnx";
                }
            }

            auto git = gateType_map.find(lookup);
            if (git == gateType_map.end()) continue;

            gate_t gtype = git->second;
            gsize_t gsize = static_cast<gsize_t>(qubits.size());
            ControlSet ctrls{};
            TargetSet tgts{};
            RotationMap angles{};

            if (!params.empty()) {
                if (gtype == GateType::U1 || gtype == GateType::CU1) {
                    if (params.size() >= 1) angles[RotationType::LAMBDA] = params[0];
                } else if (gtype == GateType::U2 || gtype == GateType::CU2) {
                    if (params.size() >= 1) angles[RotationType::PHI] = params[0];
                    if (params.size() >= 2) angles[RotationType::LAMBDA] = params[1];
                } else if (gtype == GateType::U3 || gtype == GateType::CU3 || gtype == GateType::U || gtype == GateType::CU) {
                    if (params.size() >= 1) angles[RotationType::THETA] = params[0];
                    if (params.size() >= 2) angles[RotationType::PHI] = params[1];
                    if (params.size() >= 3) angles[RotationType::LAMBDA] = params[2];
                    if (params.size() >= 4) angles[RotationType::GAMMA] = params[3];
                } else {
                    if (params.size() >= 1) angles[RotationType::THETA] = params[0];
                    if (params.size() >= 2) angles[RotationType::PHI] = params[1];
                    if (params.size() >= 3) angles[RotationType::LAMBDA] = params[2];
                    if (params.size() >= 4) angles[RotationType::GAMMA] = params[3];
                }
            }

            if (gtype == GateType::SWAP || gtype == GateType::ISWAP ||
                gtype == GateType::RXX || gtype == GateType::RZZ) {
                tgts = qubits;
            } else if (gtype == GateType::CSWAP && qubits.size() >= 2) {
                ctrls.insert(ctrls.begin(), qubits.begin(), qubits.begin() + 1);
                tgts.insert(tgts.begin(), qubits.begin() + 1, qubits.end());
            } else {
                if (qubits.size() > 1) {
                    ctrls.insert(ctrls.begin(), qubits.begin(), qubits.end() - 1);
                }
                tgts.insert(tgts.begin(), qubits.end() - 1, qubits.end());
            }

            auto gatePtr = QGatePtr(new QGate(gtype, gsize, {}, angles, ctrls, tgts));
            target.addGate(gatePtr);
        }
    }

    // ───────────── Parse a block of statements { ... } ─────────────
    void parseBlock(const ClassicalCondition& cond) {
        expect(TokenType::LBRACE, "{");
        while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
            parseStatement(cond);
        }
        expect(TokenType::RBRACE, "}");
    }

    // ───────────── Parse a single statement { . } ─────────────
    void parseStatement(const ClassicalCondition& cond) {
        // skip stray semicolons
        if (match(TokenType::SEMICOLON)) return;

        // skip closing braces (handled by caller)
        if (check(TokenType::RBRACE)) return;

        if (peek().type != TokenType::IDENTIFIER) {
            throw QcoreException("[readQASM3] l:" + std::to_string(currentLine()) +
                " unexpected token: '" + peek().value + "'");
        }

        const std::string& word = peek().value;

        // ───────────── if / else ─────────────
        if (word == "if") {
            advance(); // consume "if"
            ClassicalCondition if_cond = parseCondition();

            if (check(TokenType::LBRACE)) {
                parseBlock(if_cond);
            } else {
                // single statement
                parseGateStatement(if_cond);
            }

            // check for else
            if (checkIdent("else")) {
                advance(); // consume "else"
                ClassicalCondition else_cond = negateCondition(if_cond);

                if (check(TokenType::LBRACE)) {
                    parseBlock(else_cond);
                } else {
                    parseGateStatement(else_cond);
                }
            }
            return;
        }

        // ───────────── Assignment-form measurement: c[0] = measure q[0]; ─────────────
        // Also handles: bit result = measure q;
        // We detect this by peeking ahead for '=' after an identifier with optional [index]
        if (word != "OPENQASM" && word != "include" && word != "qubit" && word != "bit" &&
            word != "qreg" && word != "creg" && word != "gate" && word != "int" &&
            word != "measure" && word != "reset" && word != "barrier" && word != "if") {
            // Could be: name[idx] = measure ... ; or name = measure ... ;
            // Peek ahead to check for assignment
            size_t save = pos;
            bool is_assignment = false;
            advance(); // consume name
            if (match(TokenType::LBRACKET)) {
                // skip index
                if (check(TokenType::NUMBER)) advance();
                if (check(TokenType::RBRACKET)) advance();
            }
            if (check(TokenType::EQUALS)) {
                is_assignment = true;
            }
            pos = save; // restore

            if (is_assignment) {
                // parse: name[idx] = measure qubit_ref ;
                std::string cname = advance().value;
                Cbit cbit_idx = 0;
                bool has_index = false;
                if (match(TokenType::LBRACKET)) {
                    std::string idx = expect(TokenType::NUMBER, "index").value;
                    expect(TokenType::RBRACKET, "]");
                    auto it = cbit_registers.find(cname);
                    if (it == cbit_registers.end()) {
                        throw QcoreException("[readQASM3] l:" + std::to_string(currentLine()) +
                            " undeclared cbit register: " + cname);
                    }
                    cbit_idx = static_cast<Cbit>(it->second.start_index + std::stoul(idx));
                    has_index = true;
                } else {
                    auto it = cbit_registers.find(cname);
                    if (it != cbit_registers.end()) {
                        cbit_idx = static_cast<Cbit>(it->second.start_index);
                        has_index = true;
                    }
                }

                expect(TokenType::EQUALS, "=");

                if (checkIdent("measure")) {
                    advance(); // consume "measure"
                    QubitSet qubits = resolveQubitArg();
                    expectSemicolon();

                    CbitSet cbits_out = {cbit_idx};
                    AssertionInfo assertion_empty;
                    addGate(GateType::MEASURE, static_cast<gsize_t>(qubits.size()),
                            cbits_out, {}, {}, qubits, cond, assertion_empty, circuit);
                    return;
                } else {
                    // some other assignment (e.g. int variable) -- skip rest of line
                    while (!check(TokenType::SEMICOLON) && !check(TokenType::END_OF_FILE)) advance();
                    match(TokenType::SEMICOLON);
                    return;
                }
            }
        }

        // ───────────── Gate statement (built-in or custom) ─────────────
        parseGateStatement(cond);
    }

    // ───────────── Parse top-level declarations and statements ─────────────
    void parseProgram() {
        while (!check(TokenType::END_OF_FILE)) {
            // skip stray semicolons
            if (match(TokenType::SEMICOLON)) continue;

            if (peek().type != TokenType::IDENTIFIER && peek().type != TokenType::END_OF_FILE) {
                // skip unknown tokens at top level
                advance();
                continue;
            }

            if (check(TokenType::END_OF_FILE)) break;

            const std::string& word = peek().value;

            // ───────────── OPENQASM version ─────────────
            if (word == "OPENQASM") {
                advance();
                std::string ver = expect(TokenType::NUMBER, "version number").value;
                version_t version = std::stod(ver);
                if (version > 3.0) {
                    throw QcoreException("[readQASM3] l:" + std::to_string(currentLine()) +
                        " unsupported QASM version: " + ver);
                }
                expectSemicolon();
                continue;
            }

            // ───────────── include ─────────────
            if (word == "include") {
                advance();
                if (check(TokenType::STRING_LIT)) advance(); // skip filename
                expectSemicolon();
                continue;
            }

            // ───────────── qubit[n] name; or qubit name; ─────────────
            if (word == "qubit") {
                advance();
                std::uint64_t size = 1;
                if (match(TokenType::LBRACKET)) {
                    size = std::stoull(expect(TokenType::NUMBER, "register size").value);
                    expect(TokenType::RBRACKET, "]");
                }
                std::string name = expect(TokenType::IDENTIFIER, "register name").value;
                expectSemicolon();

                qubit_registers[name] = {next_qubit_index, size};
                next_qubit_index += size;
                circuit.setQreg(next_qubit_index);
                continue;
            }

            // ───────────── bit[n] name; or bit name; ─────────────
            if (word == "bit") {
                advance();
                std::uint64_t size = 1;
                if (match(TokenType::LBRACKET)) {
                    size = std::stoull(expect(TokenType::NUMBER, "register size").value);
                    expect(TokenType::RBRACKET, "]");
                }
                std::string name = expect(TokenType::IDENTIFIER, "register name").value;
                expectSemicolon();

                cbit_registers[name] = {next_cbit_index, size};
                next_cbit_index += size;
                circuit.setCreg(next_cbit_index);
                continue;
            }

            // ───────────── qreg name[n]; (QASM2 compat) ─────────────
            if (word == "qreg") {
                advance();
                std::string name = expect(TokenType::IDENTIFIER, "register name").value;
                expect(TokenType::LBRACKET, "[");
                std::uint64_t size = std::stoull(expect(TokenType::NUMBER, "register size").value);
                expect(TokenType::RBRACKET, "]");
                expectSemicolon();

                qubit_registers[name] = {next_qubit_index, size};
                next_qubit_index += size;
                circuit.setQreg(next_qubit_index);
                continue;
            }

            // ───────────── creg name[n]; (QASM2 compat) ─────────────
            if (word == "creg") {
                advance();
                std::string name = expect(TokenType::IDENTIFIER, "register name").value;
                expect(TokenType::LBRACKET, "[");
                std::uint64_t size = std::stoull(expect(TokenType::NUMBER, "register size").value);
                expect(TokenType::RBRACKET, "]");
                expectSemicolon();

                cbit_registers[name] = {next_cbit_index, size};
                next_cbit_index += size;
                circuit.setCreg(next_cbit_index);
                continue;
            }

            // ───────────── gate definition ─────────────
            if (word == "gate") {
                advance();
                GateDefinition def;
                def.name = expect(TokenType::IDENTIFIER, "gate name").value;

                // optional parameters
                if (match(TokenType::LPAREN)) {
                    if (!check(TokenType::RPAREN)) {
                        def.params.push_back(expect(TokenType::IDENTIFIER, "param name").value);
                        while (match(TokenType::COMMA)) {
                            def.params.push_back(expect(TokenType::IDENTIFIER, "param name").value);
                        }
                    }
                    expect(TokenType::RPAREN, ")");
                }

                // qubit arguments
                if (peek().type == TokenType::IDENTIFIER) {
                    def.qargs.push_back(advance().value);
                    while (match(TokenType::COMMA)) {
                        def.qargs.push_back(expect(TokenType::IDENTIFIER, "qubit arg").value);
                    }
                }

                // body { ... }
                expect(TokenType::LBRACE, "{");
                int brace_depth = 1;
                while (brace_depth > 0 && !check(TokenType::END_OF_FILE)) {
                    if (check(TokenType::LBRACE)) ++brace_depth;
                    if (check(TokenType::RBRACE)) --brace_depth;
                    if (brace_depth > 0) {
                        def.body.push_back(advance());
                    }
                }
                expect(TokenType::RBRACE, "}");

                gate_defs[def.name] = def;
                circuit.defineGateTxt(def.name, def);
                auto body = parseCustomGate(def);
                circuit.defineGate(def.name, body);
                // gate redefined: clear old cached bodies (parameterized specializations) so we don't reuse stale ones
                for (auto it = specialized_gate_bodies.begin(); it != specialized_gate_bodies.end(); ) {
                    if (it->first.first == def.name) it = specialized_gate_bodies.erase(it);
                    else ++it;
                }
                continue;
            }

            // ───────────── int variable declaration (basic, skip) ─────────────
            if (word == "int" || word == "uint" || word == "float" || word == "bool") {
                // skip the whole declaration
                while (!check(TokenType::SEMICOLON) && !check(TokenType::END_OF_FILE)) advance();
                match(TokenType::SEMICOLON);
                continue;
            }

            // ───────────── Everything else is a statement ─────────────
            ClassicalCondition no_cond{};
            parseStatement(no_cond);
        }
    }

public:
    QASM3Parser(const std::vector<Token>& tokens, QCircuit& circuit,
                 const GateLibrary* lib = nullptr)
        : tokens(tokens), circuit(circuit), library(lib) {}

    void parse() {
        parseProgram();

        // Populate circuit qubit and cbit sets
        for (Qubit i = 0; i < next_qubit_index; ++i) {
            circuit.addQubit(i);
        }
        for (Cbit i = 0; i < next_cbit_index; ++i) {
            circuit.addCbit(i);
        }
    }
};

// ───────────── Static gate type map ─────────────
std::map<std::string, GateType> QASM3Parser::gateType_map{
    {"0", GateType::I},
    {"id", GateType::I},
    {"x", GateType::X},
    {"cx", GateType::CX},
    {"ccx", GateType::CCX},
    {"cnx", GateType::MCX},
    {"mcx", GateType::MCX},
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
    {"cciz", GateType::CCIZ},
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
    {"barrier", GateType::BARRIER}
};


void QCircuit::readQASM3(std::istream& is) {
    if (!this->gate_library_) {
        this->gate_library_ = GateLibrary::loadDefault();
    }
    auto tokens = tokenize(is);
    QASM3Parser parser(tokens, *this, this->gate_library_.get());
    parser.parse();
}

std::string QGate::toOpenQASM3() {
    return toOpenQASM();
}

void QCircuit::writeQASM3(std::ostream& os) {
    os << "OPENQASM 3.0;\n"
          "include \"stdgates.inc\";\n\n";

    std::set<std::string> parameters;
    for (auto const &gate : this->getGates()) {
        for (auto const rot : gate->getAngle()) {
            for (auto const &param : getNormalizedRotationParameters(rot.second)) {
                parameters.emplace(param);
            }
        }
    }

    for (const auto& [name,body] : gate_definitions) {
        os << "gate " << name << " ";
        for (unsigned i=0;i<gate_def_text[name].params.size();++i) {
            if(i==0) os << "(";
            else os << ", ";
            os << gate_def_text[name].params[i];
            if(i==gate_def_text[name].params.size()-1) os << ") ";
        }
        for (unsigned i=0;i<gate_def_text[name].qargs.size();++i) {
            os << gate_def_text[name].qargs[i] << (i==gate_def_text[name].qargs.size()-1 ? " {" : ", ");
        }
        for (auto & g : body->getGates()) {
            os << "\n" + g->toString(FileFormat::OpenQASM, gate_def_text[name].qargs);
        }
        os << "\n}\n\n";
    }

	if (this->getQregSize()) {
	    os << "qubit[" + std::to_string(this->getQregSize()) + "] q;\n";
    }
	if (this->getCregSize()) {
	    os << "bit[" + std::to_string(this->getCregSize()) + "] c;\n";
    }
	for(auto param : parameters) {
		os << "input float " << param << ";\n";
    }

    for (ConstantQGateIterator g = this->getGates().begin(); g != this->getGates().end(); ++g) {
        os << "\n" + (*g)->toOpenQASM3();
    }
}

}  // namespace qcore

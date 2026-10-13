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
 *  @file   GateLibrary.hpp
 *  @brief  Shared gate library for predefined custom gates
 *  @author Majd Assaad
 *  @date   16.03.2026
 ***********************************************************/

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace qcore {

// Forward declarations
class QCircuit;

// ───────────── Token types (shared with tokenizer) ─────────────
enum class TokenType : std::uint8_t {
    IDENTIFIER,   // alphanumeric word
    NUMBER,       // integer or float literal
    STRING_LIT,   // "..." quoted string
    LPAREN,       // (
    RPAREN,       // )
    LBRACKET,     // [
    RBRACKET,     // ]
    LBRACE,       // {
    RBRACE,       // }
    SEMICOLON,    // ;
    COMMA,        // ,
    EQUALS,       // =
    ARROW,        // ->
    EQEQ,         // ==
    COLON,        // :
    AT,           // @
    PLUS,         // +
    MINUS,        // -
    STAR,         // *
    SLASH,        // /
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;

    Token(TokenType type, std::string value) : type(type), value(std::move(value)), line(-1) {}
    Token(TokenType type, std::string value, int line) : type(type), value(std::move(value)), line(line) {}
};

// ───────────── Custom gate definition storage ─────────────
struct GateDefinition {
    std::string name;
    std::vector<std::string> params;   // formal parameter names (e.g. "theta")
    std::vector<std::string> qargs;    // formal qubit argument names (e.g. "a", "b")
    std::vector<Token> body;           // tokens between { }
};

class GateLibrary {
   public:
    GateLibrary() = default;

    /** @brief Add a gate from a token-level definition (supports parameterized gates) */
    void addDefinition(const std::string& name, const GateDefinition& def);

    /** @brief Add a pre-compiled gate body (no parameters, for programmatic API use) */
    void defineGate(const std::string& name, std::shared_ptr<QCircuit> body);

    /** @brief Look up a token-level gate definition; returns nullptr if not found */
    const GateDefinition* findDefinition(const std::string& name) const;

    /** @brief Look up a pre-compiled gate body; returns nullptr if not found */
    std::shared_ptr<QCircuit> findCompiledGate(const std::string& name) const;

    /** @brief Check if a gate exists in either definitions or compiled gates */
    bool hasGate(const std::string& name) const;

    /** @brief Load gate definitions from a .qasm file (parses only `gate` blocks) */
    void loadFromFile(const std::string& filepath);

    /** @brief Load all .qasm files from a directory */
    void loadFromDirectory(const std::string& dirpath);

    /** @brief Load from the compiled-in default gate library directory */
    static std::shared_ptr<GateLibrary> loadDefault();

   private:
    std::map<std::string, GateDefinition> definitions_;
    std::map<std::string, std::shared_ptr<QCircuit>> compiled_;
};

}  // namespace qcore

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
 *  @file   GateLibrary.cpp
 *  @brief  Shared gate library implementation
 *  @author Majd Assaad
 *  @date   16.03.2026
 ***********************************************************/

#include "GateLibrary.hpp"
#include "QCircuit.hpp"
#include "parsers/Tokenizer.hpp"

#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace qcore {

void GateLibrary::addDefinition(const std::string& name, const GateDefinition& def) {
    definitions_[name] = def;
}

void GateLibrary::defineGate(const std::string& name, std::shared_ptr<QCircuit> body) {
    compiled_[name] = std::move(body);
}

const GateDefinition* GateLibrary::findDefinition(const std::string& name) const {
    auto it = definitions_.find(name);
    if (it != definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::shared_ptr<QCircuit> GateLibrary::findCompiledGate(const std::string& name) const {
    auto it = compiled_.find(name);
    if (it != compiled_.end()) {
        return it->second;
    }
    return nullptr;
}

bool GateLibrary::hasGate(const std::string& name) const {
    return definitions_.count(name) > 0 || compiled_.count(name) > 0;
}

void GateLibrary::loadFromFile(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.good()) {
        throw std::runtime_error("[GateLibrary] cannot open file: " + filepath);
    }

    auto tokens = tokenize(ifs);
    size_t pos = 0;

    auto peek = [&]() -> const Token& { return tokens[pos]; };
    auto advance = [&]() -> const Token& { return tokens[pos++]; };
    auto check = [&](TokenType t) -> bool { return tokens[pos].type == t; };
    auto matchId = [&](const std::string& word) -> bool {
        return check(TokenType::IDENTIFIER) && tokens[pos].value == word;
    };

    while (!check(TokenType::END_OF_FILE)) {
        // Look for "gate" keyword
        if (matchId("gate")) {
            advance(); // consume "gate"

            GateDefinition def;

            // gate name
            if (!check(TokenType::IDENTIFIER)) {
                advance(); // skip and continue
                continue;
            }
            def.name = advance().value;

            // optional parameters in parentheses
            if (check(TokenType::LPAREN)) {
                advance(); // consume (
                if (!check(TokenType::RPAREN)) {
                    if (check(TokenType::IDENTIFIER)) {
                        def.params.push_back(advance().value);
                    }
                    while (check(TokenType::COMMA)) {
                        advance(); // consume ,
                        if (check(TokenType::IDENTIFIER)) {
                            def.params.push_back(advance().value);
                        }
                    }
                }
                if (check(TokenType::RPAREN)) advance(); // consume )
            }

            // qubit arguments
            while (check(TokenType::IDENTIFIER)) {
                def.qargs.push_back(advance().value);
                if (check(TokenType::COMMA)) advance(); // consume ,
            }

            // body { ... }
            if (!check(TokenType::LBRACE)) {
                continue; // malformed, skip
            }
            advance(); // consume {

            int brace_depth = 1;
            while (brace_depth > 0 && !check(TokenType::END_OF_FILE)) {
                if (check(TokenType::LBRACE)) ++brace_depth;
                if (check(TokenType::RBRACE)) --brace_depth;
                if (brace_depth > 0) {
                    def.body.push_back(advance());
                }
            }
            if (check(TokenType::RBRACE)) advance(); // consume closing }

            definitions_[def.name] = std::move(def);
        } else {
            // Skip non-gate tokens
            advance();
        }
    }
}

void GateLibrary::loadFromDirectory(const std::string& dirpath) {
    if (!std::filesystem::exists(dirpath) || !std::filesystem::is_directory(dirpath)) {
        return; // skip if directory doesn't exist
    }

    for (const auto& entry : std::filesystem::directory_iterator(dirpath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".qasm") {
            loadFromFile(entry.path().string());
        }
    }
}

std::shared_ptr<GateLibrary> GateLibrary::loadDefault() {
    // Cached once per process: every QCircuit that doesn't set its own library shares
    // this instance (see QCircuit::readQASM3), so re-scanning gate_library/ on every
    // parse would be wasted work.
    static std::shared_ptr<GateLibrary> cached = [] {
        auto lib = std::make_shared<GateLibrary>();
#ifdef QCORE_GATE_LIBRARY_DIR
        lib->loadFromDirectory(QCORE_GATE_LIBRARY_DIR);
#endif
        return lib;
    }();
    return cached;
}

}  // namespace qcore

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
 *  @file   Tokenizer.hpp
 *  @brief  Shared tokenizer for QASM 3.0 parsing
 *  @author Majd Assaad
 *  @date   16.03.2026
 ***********************************************************/

#pragma once

#include "GateLibrary.hpp"

#include <istream>
#include <iterator>

namespace qcore {

// ───────────── Tokenizer ─────────────
inline std::vector<Token> tokenize(std::istream& is) {
    std::vector<Token> tokens;
    std::string source((std::istreambuf_iterator<char>(is)),
                        std::istreambuf_iterator<char>());

    int line = 1;
    size_t i = 0;
    while (i < source.size()) {
        char c = source[i];

        // newlines
        if (c == '\n') { ++line; ++i; continue; }

        // whitespace
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }

        // line comment: // or #
        if (c == '#' || (c == '/' && i + 1 < source.size() && source[i + 1] == '/')) {
            while (i < source.size() && source[i] != '\n') ++i;
            continue;
        }

        // block comment: /* ... */
        if (c == '/' && i + 1 < source.size() && source[i + 1] == '*') {
            i += 2;
            while (i + 1 < source.size() && !(source[i] == '*' && source[i + 1] == '/')) {
                if (source[i] == '\n') ++line;
                ++i;
            }
            i += 2; // skip */
            continue;
        }

        // string literal
        if (c == '"') {
            ++i;
            std::string s;
            while (i < source.size() && source[i] != '"') {
                s += source[i++];
            }
            ++i; // skip closing "
            tokens.push_back({TokenType::STRING_LIT, s, line});
            continue;
        }

        // arrow ->
        if (c == '-' && i + 1 < source.size() && source[i + 1] == '>') {
            tokens.push_back({TokenType::ARROW, "->", line});
            i += 2; continue;
        }

        // ==
        if (c == '=' && i + 1 < source.size() && source[i + 1] == '=') {
            tokens.push_back({TokenType::EQEQ, "==", line});
            i += 2; continue;
        }

        // single-char tokens
        auto emit1 = [&](TokenType t, const std::string& v) {
            tokens.push_back({t, v, line}); ++i;
        };
        if (c == '(') { emit1(TokenType::LPAREN, "("); continue; }
        if (c == ')') { emit1(TokenType::RPAREN, ")"); continue; }
        if (c == '[') { emit1(TokenType::LBRACKET, "["); continue; }
        if (c == ']') { emit1(TokenType::RBRACKET, "]"); continue; }
        if (c == '{') { emit1(TokenType::LBRACE, "{"); continue; }
        if (c == '}') { emit1(TokenType::RBRACE, "}"); continue; }
        if (c == ';') { emit1(TokenType::SEMICOLON, ";"); continue; }
        if (c == ',') { emit1(TokenType::COMMA, ","); continue; }
        if (c == '=') { emit1(TokenType::EQUALS, "="); continue; }
        if (c == ':') { emit1(TokenType::COLON, ":"); continue; }
        if (c == '@') { emit1(TokenType::AT, "@"); continue; }
        if (c == '+') { emit1(TokenType::PLUS, "+"); continue; }
        if (c == '*') { emit1(TokenType::STAR, "*"); continue; }
        if (c == '/') { emit1(TokenType::SLASH, "/"); continue; }

        // minus (not arrow)
        if (c == '-') { emit1(TokenType::MINUS, "-"); continue; }

        // number: digits, possibly with '.'
        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && i + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[i + 1])))) {
            std::string num;
            while (i < source.size() && (std::isdigit(static_cast<unsigned char>(source[i])) || source[i] == '.')) {
                num += source[i++];
            }
            // handle scientific notation
            if (i < source.size() && (source[i] == 'e' || source[i] == 'E')) {
                num += source[i++];
                if (i < source.size() && (source[i] == '+' || source[i] == '-'))
                    num += source[i++];
                while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i])))
                    num += source[i++];
            }
            tokens.push_back({TokenType::NUMBER, num, line});
            continue;
        }

        // identifier: starts with letter or underscore
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string id;
            while (i < source.size() && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_')) {
                id += source[i++];
            }
            tokens.push_back({TokenType::IDENTIFIER, id, line});
            continue;
        }

        // unknown character -- skip
        ++i;
    }

    tokens.push_back({TokenType::END_OF_FILE, "", line});
    return tokens;
}

}  // namespace qcore

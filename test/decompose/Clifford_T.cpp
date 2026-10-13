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
 *  @brief  Unit Test for Clifford+T decomposition
 *  @author 
 *  @date   
 ***********************************************************/

#include <gtest/gtest.h>

#include "decompose/Clifford_T.hpp"

TEST(Clifford_T, Mixed) {
  auto circuit = qcore::decompose_CNX_Logarithmic_Mixed(qcore::DecompositionConfiguration::ReduceTDepth, {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, 0, 1, 2, {3});
  std::string output = circuit.toString(qcore::FileFormat::OpenQASM3);
  EXPECT_FALSE(output.empty());
  EXPECT_NE(output, "");
}


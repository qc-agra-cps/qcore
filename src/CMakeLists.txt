
# cmake-lint: disable=C0103
set(JSON_BuildTests
    OFF
    CACHE INTERNAL "")
# cmake-lint: disable=C0103
set(JSON_MultipleHeaders
    OFF
    CACHE INTERNAL "")
# add nlohmann::json library
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/json" "extern/json" EXCLUDE_FROM_ALL)

# main project library
add_library(
  ${PROJECT_NAME} ${QCORE_LIBRARY_TYPE}
  ${PROJECT_SOURCE_DIR}/include/Definition.hpp
  ${PROJECT_SOURCE_DIR}/include/GateType.hpp
  ${PROJECT_SOURCE_DIR}/include/QGate.hpp
  ${PROJECT_SOURCE_DIR}/include/QCircuit.hpp
  ${PROJECT_SOURCE_DIR}/include/GateLibrary.hpp
  ${PROJECT_SOURCE_DIR}/include/parsers/Tokenizer.hpp
  ${PROJECT_SOURCE_DIR}/include/decompose/Clifford_T.hpp
  ${PROJECT_SOURCE_DIR}/include/decompose/Dynamic.hpp
  ${PROJECT_SOURCE_DIR}/include/decompose/Distributed.hpp
  QCircuit.cpp
  QGate.cpp
  decompose/Clifford_T.cpp
  decompose/Dynamic.cpp
  decompose/Distributed.cpp
  decompose/assertions.cpp
  parsers/ParseQASM.cpp
  parsers/ParseQASM3.cpp
  GateLibrary.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# set include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                  ${PROJECT_BINARY_DIR}/include
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    QCORE_GATE_LIBRARY_DIR="${PROJECT_SOURCE_DIR}/gate_library"
)

target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

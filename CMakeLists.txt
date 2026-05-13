cmake_minimum_required(VERSION 3.16)
project(HumanEvolution VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug builds enable assertions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(HE_DEBUG)
endif()

include_directories(${CMAKE_SOURCE_DIR})

# === Core library ===
add_library(human_evolution_core STATIC
    core/random/random.cpp
    sim/command/command.cpp
)
target_include_directories(human_evolution_core PUBLIC ${CMAKE_SOURCE_DIR})

# === Sim library (header-only for now, but separate target) ===
add_library(human_evolution_sim INTERFACE)
target_include_directories(human_evolution_sim INTERFACE ${CMAKE_SOURCE_DIR})

# === Rules library (header-only, depends on sim) ===
add_library(human_evolution_rules INTERFACE)
target_include_directories(human_evolution_rules INTERFACE ${CMAKE_SOURCE_DIR})
target_link_libraries(human_evolution_rules INTERFACE human_evolution_sim)

# === Headless console app ===
add_executable(human_evolution_sim_app
    app/headless_console/main.cpp
)
target_link_libraries(human_evolution_sim_app
    human_evolution_core
    human_evolution_sim
    human_evolution_rules
)
set_target_properties(human_evolution_sim_app PROPERTIES
    OUTPUT_NAME "human_evolution_sim"
)

# === Tests ===
enable_testing()

add_executable(human_evolution_tests
    app/tests/test_main.cpp
    app/tests/grid_test.cpp
    app/tests/random_test.cpp
    app/tests/determinism_test.cpp
    app/tests/architecture_test.cpp
    app/tests/ecology_test.cpp
    app/tests/cognitive_test.cpp
    app/tests/pattern_test.cpp
    app/tests/engine_gauntlet_test.cpp
    app/tests/history_test.cpp
    app/tests/social_test.cpp
    app/tests/social_signal_runtime_test.cpp
    app/tests/imitation_runtime_test.cpp
)
target_link_libraries(human_evolution_tests
    human_evolution_core
    human_evolution_sim
    human_evolution_rules
)
target_compile_definitions(human_evolution_tests PRIVATE HE_DEBUG)

add_test(NAME unit_tests COMMAND human_evolution_tests)

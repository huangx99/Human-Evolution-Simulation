cmake_minimum_required(VERSION 3.16)
project(HumanEvolution VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include root as include path so "core/xxx" includes work
include_directories(${CMAKE_SOURCE_DIR})

# Source files
set(CORE_SOURCES
    core/random/random.cpp
)

set(SIM_SOURCES
    # Systems are header-only for now
)

# Headless console app
add_executable(human_evolution_sim
    app/headless_console/main.cpp
    ${CORE_SOURCES}
)

target_include_directories(human_evolution_sim PRIVATE ${CMAKE_SOURCE_DIR})

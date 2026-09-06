cmake_minimum_required(VERSION 3.16)
project(satsim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# --- Core library: orbit propagation, math, time systems ---
add_library(satsim_orbit
    src/orbit/Time.cpp
    src/orbit/KeplerianElements.cpp
    src/orbit/OrbitPropagator.cpp
)
target_include_directories(satsim_orbit PUBLIC include)

# --- Constellation library: Walker pattern generation, ground stations ---
add_library(satsim_constellation
    src/constellation/WalkerConstellation.cpp
    src/constellation/GroundStation.cpp
)
target_include_directories(satsim_constellation PUBLIC include)
target_link_libraries(satsim_constellation PUBLIC satsim_orbit)

# --- Network library: visibility checks, dynamic network graph ---
add_library(satsim_network
    src/network/Visibility.cpp
    src/network/NetworkGraph.cpp
)
target_include_directories(satsim_network PUBLIC include)
target_link_libraries(satsim_network PUBLIC satsim_constellation)

# --- Routing library: Dijkstra and greedy geographic routing strategies ---
add_library(satsim_routing
    src/routing/DijkstraRouter.cpp
    src/routing/GreedyGeographicRouter.cpp
)
target_include_directories(satsim_routing PUBLIC include)
target_link_libraries(satsim_routing PUBLIC satsim_network)

# --- Example programs ---
add_executable(ground_track_demo examples/ground_track_demo.cpp)
target_link_libraries(ground_track_demo PRIVATE satsim_orbit)

add_executable(constellation_demo examples/constellation_demo.cpp)
target_link_libraries(constellation_demo PRIVATE satsim_constellation)

add_executable(network_graph_demo examples/network_graph_demo.cpp)
target_link_libraries(network_graph_demo PRIVATE satsim_network)

add_executable(routing_demo examples/routing_demo.cpp)
target_link_libraries(routing_demo PRIVATE satsim_routing)

# --- Tests ---
enable_testing()
add_executable(satsim_tests
    tests/main_test.cpp
    tests/test_vector3.cpp
    tests/test_orbit.cpp
    tests/test_constellation.cpp
    tests/test_ground_station.cpp
    tests/test_visibility.cpp
    tests/test_network_graph.cpp
    tests/test_routing.cpp
)
target_link_libraries(satsim_tests PRIVATE satsim_routing)
target_include_directories(satsim_tests PRIVATE tests)

add_test(NAME satsim_unit_tests COMMAND satsim_tests)

cmake_minimum_required(VERSION 3.12)
project(lib-core-cmd)

add_executable(lib-core-cmd
    src/AsioExecutionContext.hpp
    src/AsioHttpClient.cpp
    src/AsioHttpClient.hpp
    src/main.cpp
    src/RequestResponce.cpp
    src/RequestResponce.hpp)

find_package(Boost 1.69 REQUIRED COMPONENTS system)

target_link_libraries(lib-core-cmd PRIVATE Boost::boost Boost::system)

find_library(LEDGER-LIB-CORE NAMES ledger-core PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ledger/core/bin NO_DEFAULT_PATH)

target_link_libraries(lib-core-cmd PRIVATE ${LEDGER-LIB-CORE})

target_include_directories(lib-core-cmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/ledger/core)

cmake_minimum_required(VERSION 3.12)
project(lib-core-cmd)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Uri_BUILD_TESTS OFF CACHE BOOL "Build the URI tests." FORCE)
set(Uri_BUILD_DOCS OFF CACHE BOOL "Build the URI documentation." FORCE)
set(Uri_FULL_WARNINGS OFF CACHE BOOL "Build the library with all warnings turned on." FORCE)
set(Uri_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors." FORCE)
set(Uri_USE_STATIC_CRT OFF CACHE BOOL "Use static C Runtime library (/MT or MTd)." FORCE)

add_subdirectory(lib/uri)

add_executable(lib-core-cmd
    src/AsioExecutionContext.hpp
    src/AsioExecutionContext.cpp
    src/AsioHttpClient.cpp
    src/AsioHttpClient.hpp
    src/main.cpp
    src/RequestResponce.cpp
    src/RequestResponce.hpp
    src/PathResolver.hpp
    src/PathResolver.cpp
    src/ThreadDispatcher.hpp
    src/LogPrinter.hpp
    src/Helper.hpp
    src/Parameters.hpp
    src/Parameters.cpp
    src/AddAccount.hpp
    src/AddAccount.cpp
    src/ListAccounts.hpp
    src/ListAccounts.cpp
    src/SyncAccount.hpp
    src/SyncAccount.cpp
    src/BuildTransaction.hpp
    src/BuildTransaction.cpp
    src/ListCurrencies.hpp
    src/ListCurrencies.cpp)

add_dependencies(lib-core-cmd network-uri)

find_package(OpenSSL REQUIRED)
# Need OpenSSL to allow boost.beast make https connections
target_link_libraries(lib-core-cmd PRIVATE OpenSSL::SSL OpenSSL::Crypto)

find_package(Boost 1.69 REQUIRED COMPONENTS system context date_time program_options filesystem)

find_library(LEDGER-LIB-CORE NAMES ledger-core PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ledger/core/bin NO_DEFAULT_PATH)

target_link_libraries(lib-core-cmd PRIVATE 
    Boost::boost Boost::system Boost::context Boost::date_time Boost::program_options Boost::filesystem
    ${LEDGER-LIB-CORE}
    network-uri)

target_include_directories(lib-core-cmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/ledger/core)
target_include_directories(lib-core-cmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/uri/include)

cmake_minimum_required(VERSION 3.20)
project(MEVSimBot VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags for optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra -fsanitize=address,undefined")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# External dependencies
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# libcurl
find_package(CURL REQUIRED)

# Boost (for Beast HTTP client)
find_package(Boost REQUIRED COMPONENTS system)

# Redis (optional, for caching)
find_package(PkgConfig)
pkg_check_modules(REDIS QUIET hiredis)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.hpp"
    "include/*.hpp"
)

# Create main executable
add_executable(mev_sim_bot ${SOURCES})

# Link libraries
target_link_libraries(mev_sim_bot
    PRIVATE
    nlohmann_json::nlohmann_json
    CURL::libcurl
    Boost::system
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Link Redis if available
if(REDIS_FOUND)
    target_link_libraries(mev_sim_bot PRIVATE ${REDIS_LIBRARIES})
    target_compile_definitions(mev_sim_bot PRIVATE ENABLE_REDIS)
endif()

# SIMD optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mev_sim_bot PRIVATE -mavx2 -msse4.2)
endif()

# Install target
install(TARGETS mev_sim_bot DESTINATION bin)

# Tests
enable_testing()
add_subdirectory(tests)

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    
    add_custom_target(docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

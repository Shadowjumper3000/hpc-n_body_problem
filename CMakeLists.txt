cmake_minimum_required(VERSION 3.18)
project(nbody_simulation LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Options
option(USE_CUDA "Enable CUDA support" OFF)
option(USE_OPENMP "Enable OpenMP support" ON)

# Find MPI (required)
find_package(MPI REQUIRED)
include_directories(${MPI_CXX_INCLUDE_DIRS})

# Find OpenMP (optional)
if(USE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/nbody.cpp
    src/barnes_hut.cpp
)

# Create executable
add_executable(nbody ${SOURCES})

# Link MPI
target_link_libraries(nbody ${MPI_CXX_LIBRARIES})

# CUDA support (optional)
if(USE_CUDA)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)

    add_definitions(-DUSE_CUDA)

    # Add CUDA source
    set_source_files_properties(src/forces.cu PROPERTIES LANGUAGE CUDA)
    target_sources(nbody PRIVATE src/forces.cu)

    # CUDA flags
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -arch=sm_70 -O3")

    target_link_libraries(nbody CUDA::cudart)
endif()

# OpenMP
if(OpenMP_CXX_FOUND)
    target_link_libraries(nbody OpenMP::OpenMP_CXX)
endif()

# Include directories
target_include_directories(nbody PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Installation
install(TARGETS nbody DESTINATION bin)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "MPI: Found")
message(STATUS "OpenMP: ${OpenMP_CXX_FOUND}")
message(STATUS "CUDA: ${USE_CUDA}")

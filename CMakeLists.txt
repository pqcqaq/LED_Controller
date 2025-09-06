cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Setup C++ compiler settings for C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME mx)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM, C, and C++ languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Collect u8g2 source files
file(GLOB U8G2_C_SOURCES "Libs/u8g2/csrc/*.c")
file(GLOB U8G2_CPP_SOURCES "Libs/u8g2/cppsrc/*.cpp")

# Collect Application source files (both C and C++)
file(GLOB APP_C_SOURCES "Application/*.c" "Application/drivers/*.c" "Application/utils/*.c" "Application/global/*.c")
file(GLOB APP_CPP_SOURCES "Application/*.cpp" "Application/drivers/*.cpp" "Application/utils/*.cpp" "Application/global/*.cpp", "Application/animations/*.cpp")

# Remove old button_encoder files from the list
list(FILTER APP_CPP_SOURCES EXCLUDE REGEX ".*button_encoder\\.cpp$")
list(FILTER APP_C_SOURCES EXCLUDE REGEX ".*button_encoder\\.c$")

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${U8G2_C_SOURCES}
    ${U8G2_CPP_SOURCES}
    ${APP_C_SOURCES}
    ${APP_CPP_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Application
    Application/drivers
    Application/utils
    Application/global
    Libs/u8g2/csrc
    Libs/u8g2/cppsrc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Compile options for C and C++
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    # Common flags for both C and C++
    -Wall                             # Enable all warnings
    -fdata-sections                   # Place data in separate sections
    -ffunction-sections               # Place functions in separate sections
    
    # C++ specific flags
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>                    # Disable RTTI for C++
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>              # Disable exceptions for C++
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>      # Disable threadsafe statics for C++
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>          # Disable CXA atexit for C++
    
    # Size optimization flags for Release mode
    $<$<CONFIG:Release>:-Os>                    # Optimize for size in Release mode
    $<$<CONFIG:Release>:-flto>                  # Link Time Optimization
    $<$<CONFIG:Release>:-fno-common>            # Disable common block allocation
    $<$<CONFIG:Release>:-fomit-frame-pointer>   # Omit frame pointer when possible
    $<$<CONFIG:Release>:-fmerge-all-constants>  # Merge identical constants
    
    # Debug flags for Debug mode
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-DDEBUG>
)

# Remove problematic implicit libraries for C/C++ mixed compilation
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
if(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES)
    list(REMOVE_ITEM CMAKE_CXX_IMPLICIT_LINK_LIBRARIES ob)
endif()

# Link options for C/C++ mixed compilation
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    # Standard library linking for C++
    -lstdc++                                      # Link standard C++ library
    -lm                                          # Link math library
    
    # Linker optimization flags for size reduction
    $<$<CONFIG:Release>:-flto>                    # Link Time Optimization
    $<$<CONFIG:Release>:-Wl,--gc-sections>        # Remove unused sections
    $<$<CONFIG:Release>:-Wl,--strip-all>          # Strip all symbols
    $<$<CONFIG:Release>:-Wl,--as-needed>          # Only link needed libraries
    
    # Memory usage reporting
    -Wl,--print-memory-usage
    -Wl,--cref                                     # Cross-reference table
    
    # Debug link options
    $<$<CONFIG:Debug>:-Wl,--no-undefined>         # Report undefined symbols in debug
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

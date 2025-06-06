cmake_minimum_required(VERSION 3.15)

project(ChopsBrowser VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)

# Find SQLite3
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
if(NOT SQLITE3_FOUND)
    message(FATAL_ERROR "SQLite3 not found")
endif()

# Add JUCE
add_subdirectory(JUCE)

# Create the shared library for common code
add_library(ChopsCommon STATIC
    # Core functionality
    Source/Core/ChordParser.cpp
    Source/Core/ChordParser.h
    Source/Core/ChordTypes.h
    Source/Core/MetadataService.cpp
    Source/Core/MetadataService.h
    Source/Core/MetadataServiceTest.cpp
    Source/Core/MetadataServiceTest.h

    # Database functionality
    Source/Database/ChopsDatabase.cpp
    Source/Database/ChopsDatabase.h
    Source/Database/DatabaseSyncManager.cpp
    Source/Database/DatabaseSyncManager.h

    # Utility functions
    Source/Utils/FilenameUtils.cpp
    Source/Utils/FilenameUtils.h

    # Shared configuration
    Source/Shared/SharedConfig.h
)

# IMPORTANT: Link JUCE libraries and configure includes properly
target_link_libraries(ChopsCommon
    PUBLIC
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_audio_basics
        juce::juce_audio_formats
        ${SQLITE3_LIBRARIES}
)

# Set target properties for ChopsCommon - IMPORTANT: Include JuceLibraryCode directory
target_include_directories(ChopsCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode  # Add this line for JuceHeader.h
        ${SQLITE3_INCLUDE_DIRS}
    PRIVATE
        # Add JUCE modules directory
        ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules
)

# Configure JUCE for the library
target_compile_definitions(ChopsCommon
    PUBLIC
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        # Ensure JUCE modules are available
        JUCE_MODULE_AVAILABLE_juce_core=1
        JUCE_MODULE_AVAILABLE_juce_data_structures=1
        JUCE_MODULE_AVAILABLE_juce_events=1
        JUCE_MODULE_AVAILABLE_juce_graphics=1
        JUCE_MODULE_AVAILABLE_juce_gui_basics=1
        JUCE_MODULE_AVAILABLE_juce_audio_basics=1
        JUCE_MODULE_AVAILABLE_juce_audio_formats=1
        # Disable features we don't need
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link directories for SQLite3
target_link_directories(ChopsCommon
    PUBLIC
        ${SQLITE3_LIBRARY_DIRS}
)

# Compiler flags for SQLite3
target_compile_options(ChopsCommon
    PUBLIC
        ${SQLITE3_CFLAGS_OTHER}
)

# Add subdirectories for applications
add_subdirectory(StandaloneApp)
add_subdirectory(Plugin)

# Copy schema.sql to build directory for easy access
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Database/schema.sql
    ${CMAKE_CURRENT_BINARY_DIR}/schema.sql
    COPYONLY
)

# Set up proper folder structure in IDEs
set_target_properties(ChopsCommon PROPERTIES FOLDER "Libraries")

# Add custom target to copy database schema
add_custom_target(copy_schema ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Database/schema.sql
        ${CMAKE_CURRENT_BINARY_DIR}/StandaloneApp/schema.sql
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Database/schema.sql
        ${CMAKE_CURRENT_BINARY_DIR}/Plugin/schema.sql
    COMMENT "Copying database schema to build directories"
)

# Make sure schema is copied before building applications
add_dependencies(ChopsCommon copy_schema)

# Print configuration summary
message(STATUS "")
message(STATUS "ChopsBrowser Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  SQLite3 Version: ${SQLITE3_VERSION}")
message(STATUS "  SQLite3 Libraries: ${SQLITE3_LIBRARIES}")
message(STATUS "  SQLite3 Include Dirs: ${SQLITE3_INCLUDE_DIRS}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")

# Development helpers
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ChopsCommon
        PUBLIC
            CHOPS_DEBUG=1
            JUCE_DEBUG=1
    )
    message(STATUS "Debug build - additional logging enabled")
endif()

# Platform-specific settings
if(APPLE)
    target_compile_definitions(ChopsCommon
        PUBLIC
            JUCE_MAC=1
    )
    set_target_properties(ChopsCommon PROPERTIES
        XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17"
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
    )
elseif(WIN32)
    target_compile_definitions(ChopsCommon
        PUBLIC
            JUCE_WINDOWS=1
            NOMINMAX=1
            WIN32_LEAN_AND_MEAN=1
    )
elseif(UNIX)
    target_compile_definitions(ChopsCommon
        PUBLIC
            JUCE_LINUX=1
    )
    
    # Add threading support for Linux
    find_package(Threads REQUIRED)
    target_link_libraries(ChopsCommon
        PUBLIC
            Threads::Threads
    )
endif()

# Compiler-specific warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ChopsCommon
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(ChopsCommon
        PRIVATE
            /W4
            /wd4100  # unreferenced formal parameter
    )
endif()

# Enable link-time optimization for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(ChopsCommon PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()
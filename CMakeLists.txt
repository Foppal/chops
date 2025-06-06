cmake_minimum_required(VERSION 3.15)
project(ChopsBrowserProject VERSION 1.0.0)

# Define PROJECT_ROOT_DIRECTORY for use in C++ code (e.g., finding schema.sql)
add_compile_definitions(PROJECT_ROOT_DIRECTORY="${CMAKE_SOURCE_DIR}")

# Make JUCE available to subdirectories
# Ensure your JUCE library is in "ChopsBrowser/JUCE/"
add_subdirectory(JUCE)

# Find SQLite3 for use by sub-projects if needed globally
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Define an INTERFACE library for common source files
add_library(ChopsCommon INTERFACE)

target_sources(ChopsCommon INTERFACE
    # Paths are relative to this root CMakeLists.txt
    Source/Database/ChopsDatabase.cpp
    Source/Database/DatabaseSyncManager.cpp
    Source/Core/ChordParser.cpp
    Source/Utils/FilenameUtils.cpp
)

target_include_directories(ChopsCommon INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source # Allows #include "Database/ChopsDatabase.h" etc.
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utils # If FilenameUtils.h is there
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(ChopsCommon INTERFACE
    juce::juce_core # ChopsCommon will likely use juce::String etc.
    ${SQLITE3_LIBRARIES}
)

# Add your application subdirectory
add_subdirectory(StandaloneApp)

# Add your plugin subdirectory - THIS WAS MISSING!
add_subdirectory(Plugin)
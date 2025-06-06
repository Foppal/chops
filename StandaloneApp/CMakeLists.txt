cmake_minimum_required(VERSION 3.15)

# Create the standalone GUI application
juce_add_gui_app(ChopsLibraryManager
    # Basic app properties
    COMPANY_NAME "Camp Rock"
    BUNDLE_ID "com.camprock.chopslibrary"
    PRODUCT_NAME "Chops Library Manager"
    
    # Icon and other properties
    # ICON_BIG ...
    # ICON_SMALL ...
    
    # macOS specific
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE
)

# Generate JUCE header
juce_generate_juce_header(ChopsLibraryManager)

# Add source files to the standalone app target
target_sources(ChopsLibraryManager
    PRIVATE
        main.cpp
)

# Include directories
target_include_directories(ChopsLibraryManager
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Source
        ${SQLITE3_INCLUDE_DIRS}
)

# Preprocessor definitions
target_compile_definitions(ChopsLibraryManager
    PRIVATE
        # JUCE stuff
        JUCE_WEB_BROWSER=0  # We don't need web browser in standalone app
        JUCE_USE_CURL=0     # We don't need CURL
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ChopsLibraryManager,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:ChopsLibraryManager,JUCE_VERSION>"
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link libraries
target_link_libraries(ChopsLibraryManager
    PRIVATE
        ChopsCommon  # Our shared library with database and parsing code
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
cmake_minimum_required(VERSION 3.15)

# Create the VST3 plugin
juce_add_plugin(ChopsBrowserPlugin
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    COMPANY_NAME "Camp Rock"                   # Specify the name of the plugin's author
    BUNDLE_ID "com.camprock.chopsbrowserplugin" # Fixed bundle ID (no spaces, different from app)
    IS_SYNTH FALSE                               # Is this a synth or an effect?
    NEEDS_MIDI_INPUT FALSE                       # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT FALSE                      # Does the plugin need midi output?
    IS_MIDI_EFFECT FALSE                         # Is this plugin a MIDI effect?
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE             # Does the editor need keyboard focus?
    COPY_PLUGIN_AFTER_BUILD TRUE                 # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE Juce               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Chps                            # A unique four-character plugin id with exactly one upper-case character
                                                 # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS AU VST3 Standalone                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "Chops Browser"               # The name of the final executable, which can differ from the target name
    
    # VST3 specific settings
    VST3_CATEGORY "Fx|Tools"                     # VST3 category
    AU_MAIN_TYPE "kAudioUnitType_Effect"         # AU main type
)

# Generate JUCE header
juce_generate_juce_header(ChopsBrowserPlugin)

# Add source files to the plugin target
target_sources(ChopsBrowserPlugin
    PRIVATE
        PluginEditor.cpp
        PluginEditor.h
        PluginProcessor.cpp
        PluginProcessor.h
        UIBridge.cpp
        UIBridge.h
)

# Include directories
target_include_directories(ChopsBrowserPlugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Source
        ${SQLITE3_INCLUDE_DIRS}
)

# Preprocessor definitions
target_compile_definitions(ChopsBrowserPlugin
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=1  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link libraries
target_link_libraries(ChopsBrowserPlugin
    PRIVATE
        ChopsCommon  # Our shared library with database and parsing code
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
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
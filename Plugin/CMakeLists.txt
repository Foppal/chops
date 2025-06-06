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
    VERSION "1.0.0"                            # Plugin version
    
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
        
        # Enable metadata service in plugin
        CHOPS_PLUGIN_METADATA=1
        
        # Plugin-specific settings
        JucePlugin_Name="Chops Browser"
        JucePlugin_Desc="Chord sample browser and organizer"
        JucePlugin_Manufacturer="Camp Rock"
        JucePlugin_ManufacturerWebsite="https://github.com/Foppal/chops"
        JucePlugin_ManufacturerEmail="support@camprock.com"
        JucePlugin_ManufacturerCode=0x4d616e75  # 'Manu'
        JucePlugin_PluginCode=0x43687073        # 'Chps'
        JucePlugin_IsSynth=0
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_EditorRequiresKeyboardFocus=1
        JucePlugin_Version=1.0.0
        JucePlugin_VersionCode=0x10000
        JucePlugin_VersionString="1.0.0"
)

# Link libraries
target_link_libraries(ChopsBrowserPlugin
    PRIVATE
        ChopsCommon  # Our shared library with database, parsing, and metadata code
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

# Platform-specific plugin settings
if(APPLE)
    # macOS/iOS specific settings
    set_target_properties(ChopsBrowserPlugin PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application"
        # XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID"
    )
    
    # macOS bundle settings
    target_compile_definitions(ChopsBrowserPlugin
        PUBLIC
            JucePlugin_CFBundleIdentifier=com.camprock.chopsbrowserplugin
    )
    
elseif(WIN32)
    # Windows specific settings
    target_compile_definitions(ChopsBrowserPlugin
        PUBLIC
            # Windows-specific plugin definitions if needed
    )
    
elseif(UNIX AND NOT APPLE)
    # Linux specific settings
    target_compile_definitions(ChopsBrowserPlugin
        PUBLIC
            # Linux-specific plugin definitions if needed
    )
endif()

# Copy React UI files to plugin bundle if they exist
set(UI_DIST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../UI/dist")
if(EXISTS ${UI_DIST_PATH})
    message(STATUS "Found React UI dist folder, copying to plugin bundle")
    
    # Copy UI files to plugin resource folder
    add_custom_command(TARGET ChopsBrowserPlugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${UI_DIST_PATH}
            $<TARGET_FILE_DIR:ChopsBrowserPlugin>/UI
        COMMENT "Copying React UI files to plugin bundle"
    )
else()
    message(STATUS "No React UI dist folder found at ${UI_DIST_PATH}")
    message(STATUS "Plugin will use fallback HTML UI")
endif()

# Copy database schema to plugin directory
add_custom_command(TARGET ChopsBrowserPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../Source/Database/schema.sql
        $<TARGET_FILE_DIR:ChopsBrowserPlugin>/schema.sql
    COMMENT "Copying database schema to plugin directory"
)

# Development helpers
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ChopsBrowserPlugin
        PRIVATE
            CHOPS_PLUGIN_DEBUG=1
            CHOPS_METADATA_DEBUG=1
    )
    
    message(STATUS "Plugin: Debug build with metadata service enabled")
endif()

# Set IDE folder
set_target_properties(ChopsBrowserPlugin PROPERTIES FOLDER "Plugins")

# Print plugin build info
message(STATUS "Plugin Configuration:")
message(STATUS "  Target: ChopsBrowserPlugin")
message(STATUS "  Formats: AU, VST3, Standalone")
message(STATUS "  Including metadata service: YES")
message(STATUS "  Web browser support: YES")
message(STATUS "  React UI integration: ${UI_DIST_PATH}")
message(STATUS "")
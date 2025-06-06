cmake_minimum_required(VERSION 3.15)

# Create the standalone GUI application
juce_add_gui_app(ChopsLibraryManager
    # Basic app properties
    COMPANY_NAME "Camp Rock"
    BUNDLE_ID "com.camprock.chopslibrary"
    PRODUCT_NAME "Chops Library Manager"
    VERSION "1.0.0"
    
    # Icon and other properties
    # ICON_BIG ...
    # ICON_SMALL ...
    
    # macOS specific
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE
    
    # File associations (optional)
    # DOCUMENT_EXTENSIONS wav aif aiff mp3 flac
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
        JUCE_STANDALONE_APPLICATION=1
        
        # Enable metadata service debugging
        CHOPS_METADATA_DEBUG=1
)

# Link libraries
target_link_libraries(ChopsLibraryManager
    PRIVATE
        ChopsCommon  # Our shared library with database, parsing, and metadata code
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

# Platform-specific configurations
if(APPLE)
    # macOS specific settings
    set_target_properties(ChopsLibraryManager PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.camprock.chopslibrary"
        MACOSX_BUNDLE_BUNDLE_NAME "Chops Library Manager"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2024 Camp Rock"
    )
    
    # Code signing (optional)
    # set_target_properties(ChopsLibraryManager PROPERTIES
    #     XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application"
    #     XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID"
    # )
    
elseif(WIN32)
    # Windows specific settings
    set_target_properties(ChopsLibraryManager PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # Add Windows resources if you have them
    # target_sources(ChopsLibraryManager PRIVATE
    #     Resources/Windows/app.rc
    # )
    
elseif(UNIX AND NOT APPLE)
    # Linux specific settings
    
    # Install desktop file
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Linux/chopslibrary.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/chopslibrary.desktop
        @ONLY
    )
    
    # Install target for Linux
    install(TARGETS ChopsLibraryManager
        RUNTIME DESTINATION bin
    )
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/chopslibrary.desktop
        DESTINATION share/applications
    )
endif()

# Copy database schema to the application directory
add_custom_command(TARGET ChopsLibraryManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../Source/Database/schema.sql
        $<TARGET_FILE_DIR:ChopsLibraryManager>/schema.sql
    COMMENT "Copying database schema to application directory"
)

# Development helpers
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Enable additional debugging for the standalone app
    target_compile_definitions(ChopsLibraryManager
        PRIVATE
            CHOPS_STANDALONE_DEBUG=1
    )
    
    message(STATUS "StandaloneApp: Debug build with metadata service testing enabled")
endif()

# Set IDE folder
set_target_properties(ChopsLibraryManager PROPERTIES FOLDER "Applications")

# Print build info
message(STATUS "StandaloneApp Configuration:")
message(STATUS "  Target: ChopsLibraryManager")
message(STATUS "  Including metadata service testing: YES")
message(STATUS "  Database schema will be copied to build directory")

# Create test directories in build folder for development
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_files)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/metadata_tests)

message(STATUS "  Created test directories in build folder")
message(STATUS "")
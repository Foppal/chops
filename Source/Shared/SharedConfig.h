#pragma once

#include <JuceHeader.h>

/**
 * Shared configuration between Standalone App and Plugin
 * This ensures both components look in the same places
 */
namespace ChopsConfig
{
    // Default paths
    inline File getDefaultLibraryDirectory()
    {
        #if JUCE_MAC
            return File::getSpecialLocation(File::userDocumentsDirectory)
                .getChildFile("Chops Library");
        #elif JUCE_WINDOWS
            return File::getSpecialLocation(File::userDocumentsDirectory)
                .getChildFile("Chops Library");
        #else
            return File::getSpecialLocation(File::userHomeDirectory)
                .getChildFile("Documents")
                .getChildFile("Chops Library");
        #endif
    }
    
    inline File getDatabaseFile()
    {
        return getDefaultLibraryDirectory()
            .getChildFile("chops_library.sqlite");
    }
    
    inline File getConfigFile()
    {
        return getDefaultLibraryDirectory()
            .getChildFile("config.xml");
    }
    
    inline File getDatabaseTimestampFile()
    {
        return getDefaultLibraryDirectory()
            .getChildFile(".db_update_timestamp");
    }
    
    // Shared preferences keys
    namespace PreferenceKeys
    {
        const String libraryPath = "libraryPath";
        const String lastScanTime = "lastScanTime";
        const String autoScanEnabled = "autoScanEnabled";
        const String previewGain = "previewGain";
    }
    
    // File organization structure
    namespace FolderNames
    {
        const String chopsRoot = "Chops";
        const String uploadFolder = "1. Chops upload";
        const String processedFolder = "2. Processed";
        const String mismatchFolder = "3. Filename mismatch";
    }
    
    // Audio file extensions
    inline StringArray getSupportedAudioExtensions()
    {
        return StringArray({".wav", ".aif", ".aiff", ".mp3", ".flac", ".m4a", ".ogg"});
    }
    
    inline bool isAudioFile(const File& file)
    {
        return getSupportedAudioExtensions().contains(file.getFileExtension().toLowerCase());
    }
    
    // Database update notification
    class DatabaseUpdateNotifier
    {
    public:
        DatabaseUpdateNotifier() : timestampFile(getDatabaseTimestampFile()) {}
        
        // Called by standalone app after database update
        void notifyDatabaseUpdated()
        {
            timestampFile.create();
            timestampFile.setLastModificationTime(Time::getCurrentTime());
        }
        
        // Called by plugin to check for updates
        bool hasUpdatesAfter(Time lastCheckTime) const
        {
            if (!timestampFile.existsAsFile())
                return false;
                
            return timestampFile.getLastModificationTime() > lastCheckTime;
        }
        
        Time getLastUpdateTime() const
        {
            if (!timestampFile.existsAsFile())
                return Time(0);
                
            return timestampFile.getLastModificationTime();
        }
        
    private:
        File timestampFile;
    };
    
    // Simple IPC message protocol
    namespace IPCMessages
    {
        const String libraryUpdated = "LIBRARY_UPDATED";
        const String requestRescan = "REQUEST_RESCAN";
        const String appIsRunning = "APP_RUNNING";
    }
}
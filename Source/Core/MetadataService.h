#pragma once

#include <JuceHeader.h>
#include "../Database/ChopsDatabase.h"

/**
 * MetadataService - Core service for reading/writing WAV file metadata
 * 
 * This service handles:
 * - Reading iXML chunks from WAV files
 * - Writing chord metadata to iXML chunks
 * - Syncing metadata between files and database
 * - Handling the transition from filename-based to metadata-based system
 */
class MetadataService
{
public:
    MetadataService();
    ~MetadataService() = default;
    
    // Core metadata structure that gets stored in iXML
    struct ChordMetadata
    {
        juce::String rootNote;
        juce::String chordType;
        juce::String chordTypeDisplay;
        juce::StringArray extensions;
        juce::StringArray alterations;
        juce::StringArray addedNotes;
        juce::StringArray suspensions;
        juce::String bassNote;
        juce::String inversion;
        
        // User metadata
        juce::StringArray tags;
        int rating = 0;
        bool isFavorite = false;
        juce::String userNotes;
        juce::Colour color = juce::Colours::transparentBlack;
        
        // System metadata
        juce::String originalFilename;
        juce::Time dateAdded;
        juce::Time dateModified;
        int playCount = 0;
        juce::Time lastPlayed;
        
        // Validation
        bool isValid() const;
        juce::String toString() const; // For debugging
        
        // Convert to/from database format
        ChopsDatabase::SampleInfo toDatabaseSampleInfo(const juce::String& filePath, int64 fileSize) const;
        static ChordMetadata fromDatabaseSampleInfo(const ChopsDatabase::SampleInfo& sampleInfo);
    };
    
    // Core metadata operations
    bool readMetadataFromFile(const juce::File& audioFile, ChordMetadata& metadata);
    bool writeMetadataToFile(const juce::File& audioFile, const ChordMetadata& metadata);
    bool hasMetadata(const juce::File& audioFile);
    
    // Database sync operations
    bool syncFileWithDatabase(const juce::File& audioFile, ChopsDatabase* database);
    bool updateFileMetadata(const juce::File& audioFile, const ChordMetadata& metadata, ChopsDatabase* database);
    
    // Batch operations for library scanning
    struct ScanResult
    {
        int filesProcessed = 0;
        int filesWithMetadata = 0;
        int filesWithoutMetadata = 0;
        int metadataWritten = 0;
        int databaseUpdated = 0;
        int errors = 0;
        juce::StringArray errorMessages;
    };
    
    ScanResult scanAndSyncDirectory(const juce::File& directory, ChopsDatabase* database, 
                                  bool recursive = true, bool writeMetadataToFiles = true);
    
    // Migration helpers (for transitioning from current system)
    bool migrateFromFilenameToMetadata(const juce::File& audioFile, ChopsDatabase* database);
    ScanResult migrateEntireLibrary(const juce::File& libraryRoot, ChopsDatabase* database);
    
    // Validation and repair
    bool validateFileMetadata(const juce::File& audioFile, juce::StringArray& issues);
    bool repairMetadata(const juce::File& audioFile, ChopsDatabase* database);

private:
    // iXML reading/writing implementation
    bool readIXMLChunk(const juce::File& audioFile, juce::String& iXMLContent);
    bool writeIXMLChunk(const juce::File& audioFile, const juce::String& iXMLContent);
    
    // Metadata serialization
    juce::String metadataToIXML(const ChordMetadata& metadata);
    bool iXMLToMetadata(const juce::String& iXMLContent, ChordMetadata& metadata);
    
    // Helper methods
    bool isAudioFile(const juce::File& file);
    juce::String sanitizeXMLString(const juce::String& input);
    juce::String escapeXMLAttribute(const juce::String& input);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetadataService)
};
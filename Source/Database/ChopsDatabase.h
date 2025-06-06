#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

class ChopsDatabase
{
public:
    ChopsDatabase();
    ~ChopsDatabase();
    
    bool open(const juce::String& databasePath);
    void close();
    bool isOpen() const { return db != nullptr; }
    
    struct SampleInfo
    {
        int id = 0;
        juce::String originalFilename;
        juce::String currentFilename;
        juce::String filePath;
        int64 fileSize = 0;
        
        juce::String rootNote;
        juce::String chordType;
        juce::String chordTypeDisplay;
        
        juce::StringArray extensions;
        juce::StringArray alterations;
        juce::StringArray addedNotes;
        juce::StringArray suspensions;
        
        juce::String bassNote;
        juce::String inversion;
        
        juce::Time dateAdded;
        juce::Time dateModified;
        
        juce::StringArray tags;
        int rating = 0;
        juce::Colour color = juce::Colours::transparentBlack;
        bool isFavorite = false;
        int playCount = 0;
        juce::String userNotes;
        juce::Time lastPlayed;
        
        juce::String getFullChordName() const;
        juce::String getShortChordName() const;
    };
    
    enum BoolFilter { DontCare, Yes, No };
    
    // Search and retrieval
    std::vector<SampleInfo> searchSamples(
        const juce::String& query = "",
        const juce::String& rootNote = "",
        const juce::String& chordType = "",
        BoolFilter hasExtensions = DontCare,
        BoolFilter hasAlterations = DontCare,
        int limit = 100,
        int offset = 0
    );
    
    std::unique_ptr<SampleInfo> getSampleByPath(const juce::String& filePath);
    std::unique_ptr<SampleInfo> getSampleById(int sampleId);
    
    // Sample management
    int insertSample(const SampleInfo& sample);
    bool updateSample(const SampleInfo& sample);
    bool deleteSample(int sampleId);
    
    // User metadata
    bool addTag(int sampleId, const juce::String& tag);
    bool removeTag(int sampleId, const juce::String& tag);
    bool setRating(int sampleId, int rating);
    bool setColor(int sampleId, const juce::Colour& color);
    bool addToFavorites(int sampleId);
    bool removeFromFavorites(int sampleId);
    bool incrementPlayCount(int sampleId);
    bool setNotes(int sampleId, const juce::String& notes);
    
    // Tag management
    juce::StringArray getTags(int sampleId);
    juce::StringArray getAllTags();
    std::vector<SampleInfo> getSamplesByTag(const juce::String& tag);
    
    // Chord type information
    struct ChordTypeInfo
    {
        juce::String typeKey;
        juce::String displayName;
        juce::StringArray intervals;
        juce::String family;
        int complexity = 0;
    };
    
    std::vector<ChordTypeInfo> getChordTypes(const juce::String& family = "");
    
    // Statistics and analysis
    struct Statistics
    {
        int totalSamples = 0;
        std::vector<std::pair<juce::String, int>> byChordType;
        std::vector<std::pair<juce::String, int>> byRootNote;
        int withExtensions = 0;
        int withAlterations = 0;
        int addedLastWeek = 0;
    };
    
    Statistics getStatistics();
    
    // Distinct values for filters
    juce::StringArray getDistinctRootNotes();
    juce::StringArray getDistinctChordTypes();
    
    // Transaction support
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // Database maintenance
    bool vacuum();
    bool analyze();
    juce::String getDatabaseInfo();

private:
    void* db;
    void* searchStmt;
    void* sampleByPathStmt;
    void* sampleByIdStmt;
    
    void prepareStatements();
    void finalizeStatements();
    
    SampleInfo parseRow(void* stmt);
    juce::StringArray parseJsonArray(const juce::String& json);
    juce::String stringArrayToJson(const juce::StringArray& array);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopsDatabase)
};
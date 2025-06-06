#pragma once

#include <JuceHeader.h>
#include "../Source/Database/ChopsDatabase.h"
#include "../Source/Database/DatabaseSyncManager.h"
#include <memory>

//==============================================================================
/**
    Chops Browser Plugin Processor
    
    This plugin doesn't process audio, but provides a sample browser interface
    with preview capabilities and drag & drop functionality.
*/
class ChopsBrowserPluginProcessor : public juce::AudioProcessor,
                                    public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    ChopsBrowserPluginProcessor();
    ~ChopsBrowserPluginProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Custom methods for Chops Browser
    
    // Database access
    DatabaseSyncManager* getDatabaseManager() { return &databaseManager; }
    void setDatabasePath(const juce::String& path);
    bool isDatabaseAvailable() const;
    juce::String getDatabaseInfo() const;
    
    // Search functionality
    struct SearchCriteria
    {
        juce::String rootNote;
        juce::String chordType;
        juce::String searchText;
        juce::StringArray tags;
        int minRating = 0;
        bool favoritesOnly = false;
        bool hasExtensions = false;
        bool hasAlterations = false;
        bool filterByExtensions = false;
        bool filterByAlterations = false;
    };
    
    std::vector<ChopsDatabase::SampleInfo> searchSamples(const SearchCriteria& criteria);
    
    // Preview functionality
    void loadSampleForPreview(const juce::String& filePath);
    void playPreview();
    void stopPreview();
    void seekPreview(float position); // 0.0 to 1.0
    bool isPreviewPlaying() const { return previewIsPlaying; }
    float getPreviewProgress() const;
    
    // Drag and drop
    juce::String getCurrentSamplePath() const { return currentSamplePath; }
    
    // Settings
    void setChopsLibraryPath(const juce::String& path);
    juce::String getChopsLibraryPath() const { return chopsLibraryPath; }
    
    // State
    juce::String getLastSearchQuery() const { return lastSearchQuery; }

    //==============================================================================
    // *** TEST METHODS ADDED HERE ***
    // Database testing and diagnostics
    void testDatabaseConnection();
    void createTestData();
    bool hasAnyData();
    bool hasWriteAccess();

private:
    //==============================================================================
    // Database
    DatabaseSyncManager databaseManager;
    juce::String chopsLibraryPath;
    juce::String currentDatabasePath;
    
    // Preview player
    std::unique_ptr<juce::AudioFormatManager> formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioBuffer<float> previewBuffer;
    bool previewIsPlaying = false;
    juce::String currentSamplePath;
    
    // State
    juce::String lastSearchQuery;
    
    // Thread safety
    juce::CriticalSection audioLock;
    
    // Initialize components
    void initializeAudioFormats();
    void initializeDatabase();

    //==============================================================================
    // *** TEST DATA STRUCTURE ADDED HERE ***
    struct TestSample {
        juce::String filename;
        juce::String rootNote;
        juce::String chordType;
        juce::String chordTypeDisplay;
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopsBrowserPluginProcessor)
};
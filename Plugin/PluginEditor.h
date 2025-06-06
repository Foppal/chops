#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UIBridge.h"
#include "../Source/Database/ChopsDatabase.h"
#include "../Source/Database/DatabaseSyncManager.h"
#include "../Source/Core/ChordParser.h"
#include <memory>
#include <unordered_map>

//==============================================================================
/**
    This is the editor component that our filter will display.
*/
class ChopsBrowserPluginEditor : public juce::AudioProcessorEditor,
                                 public juce::ChangeListener,
                                 public juce::Timer
{
public:
    ChopsBrowserPluginEditor(ChopsBrowserPluginProcessor&);
    ~ChopsBrowserPluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    
    //==============================================================================
    // Change listener for processor updates
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
    //==============================================================================
    // Timer callback for preview progress updates
    void timerCallback() override;

private:
    //==============================================================================
    // UI Components
    std::unique_ptr<UIBridge> uiBridge;
    std::unique_ptr<juce::Component> fallbackUI;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    bool useReactUI = true;
    
    //==============================================================================
    // Data and State
    ChopsBrowserPluginProcessor& audioProcessor;
    std::vector<ChopsDatabase::SampleInfo> currentResults;
    int selectedSampleIndex = -1;
    
    //==============================================================================
    // UI Bridge Callbacks Setup
    void setupUIBridgeCallbacks();
    
    //==============================================================================
    // Search and Selection Handlers
    void handleSearchRequested(const juce::String& query);
    void parseQueryIntoCriteria(const juce::String& query, ChopsBrowserPluginProcessor::SearchCriteria& criteria);
    void handleChordSelected(const ChordParser::ParsedData& chordData);
    void handleSampleSelected(int sampleId);
    
    //==============================================================================
    // Preview Handlers
    void handlePreviewPlay();
    void handlePreviewStop();
    void handlePreviewSeek(float position);
    
    //==============================================================================
    // Sample Metadata Handlers
    void handleSampleRatingChanged(int sampleId, int rating);
    void handleSampleTagAdded(int sampleId, const juce::String& tag);
    void handleSampleFavoriteToggled(int sampleId);
    
    //==============================================================================
    // Library Handlers
    void handleLibraryPathChanged(const juce::String& path);
    void handleLibraryRescanRequested();
    void handleUIEvent(const juce::String& eventType, const juce::var& eventData);
    
    //==============================================================================
    // UI Management
    void loadUI();
    void loadReactUI();
    void createFallbackUI();
    void switchToFallbackUI();
    juce::String createHTMLContent();
    
    //==============================================================================
    // State Synchronization
    void updateUIState();
    void sendCurrentData();
    
    //==============================================================================
    // Utilities
    juce::File getUIHtmlFile();
    juce::String generateInlineHTML();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopsBrowserPluginEditor)
};
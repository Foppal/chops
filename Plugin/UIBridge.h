#pragma once

#include <JuceHeader.h>
#include "../Source/Database/ChopsDatabase.h"
#include "../Source/Core/ChordParser.h"
#include <memory>
#include <functional>

class UIBridge; // Forward declaration

/**
 * Custom WebBrowserComponent that forwards page load events to UIBridge
 * and handles JavaScript message communication
 */
class CustomWebBrowserComponent : public juce::WebBrowserComponent
{
public:
    CustomWebBrowserComponent(UIBridge* bridge);
    ~CustomWebBrowserComponent() override = default;
    
    bool pageAboutToLoad(const juce::String& newURL) override;
    void pageFinishedLoading(const juce::String& url) override;
    
    // FIXED: Remove override - this is not a virtual function in JUCE WebBrowserComponent
    // We'll use a different approach for JavaScript communication
    void handleJavaScriptCall(const juce::String& functionName, 
                             const juce::StringArray& parameters);
    
private:
    UIBridge* uiBridge;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomWebBrowserComponent)
};

/**
 * UIBridge - Communication bridge between C++ JUCE plugin and React UI
 * 
 * This class manages the WebBrowserComponent and provides a clean interface
 * for bidirectional communication between the C++ backend and React frontend.
 */
class UIBridge : public juce::Component
{
public:
    //==========================================================================
    // Constructor & Destructor
    UIBridge();
    ~UIBridge() override;
    
    //==========================================================================
    // Component Interface
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    //==========================================================================
    // Page handling (called by CustomWebBrowserComponent)
    bool pageAboutToLoad(const juce::String& newURL);
    void pageFinishedLoading(const juce::String& url);
    
    //==========================================================================
    // JavaScript Message Handling - FIXED: Updated approach for JUCE compatibility
    void handleJavaScriptCall(const juce::String& functionName, 
                             const juce::StringArray& parameters);
    void handleJavaScriptMessage(const juce::String& message);
    
    //==========================================================================
    // C++ to React Communication (Send data to React)
    
    // Send chord/sample data
    void sendChordData(const ChordParser::ParsedData& chordData);
    void sendSampleResults(const std::vector<ChopsDatabase::SampleInfo>& samples);
    void sendSelectedSample(const ChopsDatabase::SampleInfo& sample);
    
    // Send UI state updates
    void sendLoadingState(bool isLoading);
    void sendErrorMessage(const juce::String& error);
    void sendPreviewState(bool isPlaying, float progress = 0.0f);
    
    // Send configuration
    void sendDatabaseStats(const ChopsDatabase::Statistics& stats);
    void sendLibraryPath(const juce::String& path);
    
    //==========================================================================
    // React to C++ Communication (Callbacks from React)
    
    struct Callbacks
    {
        // Search and selection
        std::function<void(const juce::String&)> onSearchRequested;
        std::function<void(const ChordParser::ParsedData&)> onChordSelected;
        std::function<void(int)> onSampleSelected;
        
        // Preview control - enhanced for specific sample preview
        std::function<void()> onPreviewPlay;
        std::function<void()> onPreviewStop;
        std::function<void(float)> onPreviewSeek;
        std::function<void(int, const juce::String&)> onPreviewSample; // sampleId, filePath
        
        // Sample management
        std::function<void(int, int)> onSampleRatingChanged;
        std::function<void(int, const juce::String&)> onSampleTagAdded;
        std::function<void(int)> onSampleFavoriteToggled;
        
        // Library management
        std::function<void(const juce::String&)> onLibraryPathChanged;
        std::function<void()> onLibraryRescanRequested;
        
        // UI events
        std::function<void(const juce::String&, const juce::var&)> onUIEvent;
    };
    
    void setCallbacks(const Callbacks& callbacks);
    
    //==========================================================================
    // UI Management
    
    // Load the React UI
    void loadUI(const juce::File& htmlFile);
    void loadUI(const juce::String& htmlContent);
    
    // Get the web browser component
    juce::WebBrowserComponent* getWebBrowser() { return webBrowser.get(); }
    
    // UI state
    bool isUILoaded() const { return uiLoaded; }
    bool isInitializationFailed() const { return initializationFailed; }
    void reloadUI();
    
    //==========================================================================
    // Development helpers
    
    void enableDevMode(bool enabled);
    void openDevTools();
    void injectTestData();

private:
    //==========================================================================
    // Private members
    
    std::unique_ptr<CustomWebBrowserComponent> webBrowser;
    Callbacks callbacks;
    
    // State
    bool uiLoaded = false;
    bool devModeEnabled = false;
    bool initializationFailed = false;
    bool contentLoadAttempted = false;
    juce::String currentURL;
    
    // Message queuing for before UI loads
    juce::StringArray pendingMessages;
    
    //==========================================================================
    // Private methods
    
    // JavaScript execution
    void executeJavaScript(const juce::String& script);
    void executeJavaScriptWhenReady(const juce::String& script);
    void loadSimpleTestHTML();
    void loadReactUIContent(); // NEW METHOD
    void loadEnhancedTestHTML(); // FIXED: Added missing declaration
    
    // Message processing
    void processJavaScriptMessage(const juce::var& messageData);
    void sendPendingMessages();
    
    // Data serialization helpers
    juce::var chordDataToVar(const ChordParser::ParsedData& data);
    juce::var sampleInfoToVar(const ChopsDatabase::SampleInfo& sample);
    juce::var sampleArrayToVar(const std::vector<ChopsDatabase::SampleInfo>& samples);
    juce::var statsToVar(const ChopsDatabase::Statistics& stats);
    
    // Message type handlers
    void handleSearchMessage(const juce::var& data);
    void handleChordSelectionMessage(const juce::var& data);
    void handleSampleSelectionMessage(const juce::var& data);
    void handlePreviewMessage(const juce::var& data);
    void handleSampleMetadataMessage(const juce::var& data);
    void handleLibraryMessage(const juce::var& data);
    void handleUIEventMessage(const juce::var& data);
    
    // Chord data parsing
    ChordParser::ParsedData varToChordData(const juce::var& data);
    
    // UI file management
    juce::File getUIHtmlFile(); // Updated to find React UI files
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIBridge)
};
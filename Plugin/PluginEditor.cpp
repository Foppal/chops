#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
ChopsBrowserPluginEditor::ChopsBrowserPluginEditor(ChopsBrowserPluginProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    logFile.appendText("=== PluginEditor Constructor Started ===\n");
    
    juce::Logger::writeToLog("ChopsBrowserPluginEditor constructor called");
    
    // CRITICAL: Set size FIRST before creating any child components
    setSize(900, 600);
    logFile.appendText("PluginEditor size set to: 900x600\n");
    
    // Set resize limits to ensure proper sizing
    setResizable(true, true);
    setResizeLimits(400, 300, 1200, 800);
    logFile.appendText("Resize limits set\n");
    
    try {
        // Initialize the UI bridge with error handling
        logFile.appendText("Creating UIBridge...\n");
        uiBridge = std::make_unique<UIBridge>();
        
        if (!uiBridge) {
            logFile.appendText("ERROR: Failed to create UIBridge!\n");
            createFallbackUI();
            return;
        }
        
        logFile.appendText("UIBridge created successfully\n");
        
        // Add to component hierarchy
        addAndMakeVisible(*uiBridge);
        logFile.appendText("UIBridge added to hierarchy\n");
        
        // CRITICAL: Force initial sizing
        auto initialBounds = getLocalBounds();
        logFile.appendText("PluginEditor bounds: " + initialBounds.toString() + "\n");
        uiBridge->setBounds(initialBounds);
        logFile.appendText("UIBridge bounds set to: " + initialBounds.toString() + "\n");
        
        // Setup callbacks for communication between UI and backend
        setupUIBridgeCallbacks();
        logFile.appendText("UI callbacks set up\n");
        
        // Load the UI
        loadUI();
        logFile.appendText("UI loading initiated\n");
        
    } catch (const std::exception& e) {
        logFile.appendText("EXCEPTION in constructor: " + juce::String(e.what()) + "\n");
        juce::Logger::writeToLog("PluginEditor: Exception in constructor: " + juce::String(e.what()));
        createFallbackUI();
        return;
    }
    
    // Add this editor as a change listener to the processor
    audioProcessor.addChangeListener(this);
    
    // Start timer for preview progress updates (30 FPS)
    startTimer(33);
    
    // Force a repaint
    repaint();
    
    logFile.appendText("PluginEditor constructor completed successfully\n");
    logFile.appendText("Final PluginEditor bounds: " + getBounds().toString() + "\n");
    if (uiBridge) {
        logFile.appendText("Final UIBridge bounds: " + uiBridge->getBounds().toString() + "\n");
    }
    logFile.appendText("==============================\n\n");
    
    juce::Logger::writeToLog("ChopsBrowserPluginEditor initialized successfully");
}

ChopsBrowserPluginEditor::~ChopsBrowserPluginEditor()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    logFile.appendText("=== PluginEditor Destructor Called ===\n");
    
    juce::Logger::writeToLog("ChopsBrowserPluginEditor destructor called");
    
    // Stop timer
    stopTimer();
    
    // Remove change listener
    audioProcessor.removeChangeListener(this);
    
    // Clean up UI bridge
    uiBridge.reset();
    webView.reset();
    fallbackUI.reset();
    
    logFile.appendText("PluginEditor destructor completed\n");
}

//==============================================================================
void ChopsBrowserPluginEditor::paint(juce::Graphics& g)
{
    // Always paint a background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // If no UI is loaded, show a status message
    if (!uiBridge && !fallbackUI) {
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(20.0f));
        g.drawText("Chops Browser Loading...", getLocalBounds(), juce::Justification::centred);
        
        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::FontOptions(14.0f));
        g.drawText("If this persists, check the debug log on your Desktop", 
                   getLocalBounds().translated(0, 30), juce::Justification::centred);
    }
}

void ChopsBrowserPluginEditor::resized()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    auto bounds = getLocalBounds();
    logFile.appendText("=== PluginEditor RESIZED ===\n");
    logFile.appendText("New bounds: " + bounds.toString() + "\n");
    logFile.appendText("Time: " + juce::Time::getCurrentTime().toString(true, true, true, true) + "\n");
    
    if (uiBridge)
    {
        logFile.appendText("Setting UIBridge bounds to: " + bounds.toString() + "\n");
        uiBridge->setBounds(bounds);
        logFile.appendText("UIBridge bounds after resize: " + uiBridge->getBounds().toString() + "\n");
    }
    else if (webView)
    {
        webView->setBounds(bounds);
        logFile.appendText("WebView bounds set\n");
    }
    else if (fallbackUI)
    {
        fallbackUI->setBounds(bounds);
        logFile.appendText("FallbackUI bounds set\n");
    }
    else
    {
        logFile.appendText("ERROR: No UI component to resize!\n");
    }
    
    logFile.appendText("========================\n\n");
}

//==============================================================================
void ChopsBrowserPluginEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // Handle changes from the processor (like preview state changes)
    if (source == &audioProcessor)
    {
        juce::Logger::writeToLog("Processor state changed - updating UI");
        
        // Update UI state based on processor changes
        updateUIState();
    }
}

void ChopsBrowserPluginEditor::timerCallback()
{
    // Update preview progress if playing
    if (audioProcessor.isPreviewPlaying() && uiBridge)
    {
        float progress = audioProcessor.getPreviewProgress();
        uiBridge->sendPreviewState(true, progress);
    }
}

//==============================================================================
void ChopsBrowserPluginEditor::setupUIBridgeCallbacks()
{
    if (!uiBridge) return;
    
    UIBridge::Callbacks callbacks;
    
    // Search and selection callbacks
    callbacks.onSearchRequested = [this](const juce::String& query) {
        handleSearchRequested(query);
    };
    
    callbacks.onChordSelected = [this](const ChordParser::ParsedData& chordData) {
        handleChordSelected(chordData);
    };
    
    callbacks.onSampleSelected = [this](int sampleId) {
        handleSampleSelected(sampleId);
    };
    
    // Preview control callbacks
    callbacks.onPreviewPlay = [this]() {
        handlePreviewPlay();
    };
    
    callbacks.onPreviewStop = [this]() {
        handlePreviewStop();
    };
    
    callbacks.onPreviewSeek = [this](float position) {
        handlePreviewSeek(position);
    };
    
    // Sample metadata callbacks
    callbacks.onSampleRatingChanged = [this](int sampleId, int rating) {
        handleSampleRatingChanged(sampleId, rating);
    };
    
    callbacks.onSampleTagAdded = [this](int sampleId, const juce::String& tag) {
        handleSampleTagAdded(sampleId, tag);
    };
    
    callbacks.onSampleFavoriteToggled = [this](int sampleId) {
        handleSampleFavoriteToggled(sampleId);
    };
    
    // Library management callbacks
    callbacks.onLibraryPathChanged = [this](const juce::String& path) {
        handleLibraryPathChanged(path);
    };
    
    callbacks.onLibraryRescanRequested = [this]() {
        handleLibraryRescanRequested();
    };
    
    callbacks.onUIEvent = [this](const juce::String& eventType, const juce::var& eventData) {
        handleUIEvent(eventType, eventData);
    };
    
    uiBridge->setCallbacks(callbacks);
}

//==============================================================================
void ChopsBrowserPluginEditor::handleSearchRequested(const juce::String& query)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== SEARCH REQUEST RECEIVED ===\n");
    logFile.appendText("Time: " + juce::Time::getCurrentTime().toString(true, true, true, true) + "\n");
    logFile.appendText("Query: '" + query + "'\n");
    
    juce::Logger::writeToLog("Search requested: " + query);
    
    if (!uiBridge) {
        logFile.appendText("❌ ERROR: uiBridge is null!\n");
        logFile.appendText("==============================\n\n");
        return;
    }
    
    logFile.appendText("✅ UIBridge is available\n");
    
    // Send loading state
    uiBridge->sendLoadingState(true);
    logFile.appendText("Sent loading state: true\n");
    
    // Parse the query into search criteria
    ChopsBrowserPluginProcessor::SearchCriteria criteria;
    parseQueryIntoCriteria(query, criteria);
    
    logFile.appendText("Parsed search criteria:\n");
    logFile.appendText("  - Root note: '" + criteria.rootNote + "'\n");
    logFile.appendText("  - Chord type: '" + criteria.chordType + "'\n");
    logFile.appendText("  - Search text: '" + criteria.searchText + "'\n");
    
    // Check if processor is available
    logFile.appendText("Checking processor availability...\n");
    logFile.appendText("  - Processor database available: " + juce::String(audioProcessor.isDatabaseAvailable() ? "YES" : "NO") + "\n");
    
    if (audioProcessor.isDatabaseAvailable())
    {
        auto dbInfo = audioProcessor.getDatabaseInfo();
        logFile.appendText("  - Database info: " + dbInfo + "\n");
    }
    
    // Perform the search
    logFile.appendText("Calling audioProcessor.searchSamples()...\n");
    currentResults = audioProcessor.searchSamples(criteria);
    logFile.appendText("Search completed, got " + juce::String(currentResults.size()) + " results\n");
    
    if (currentResults.size() > 0)
    {
        logFile.appendText("First few results:\n");
        for (int i = 0; i < juce::jmin(3, (int)currentResults.size()); ++i)
        {
            const auto& sample = currentResults[i];
            logFile.appendText("  " + juce::String(i+1) + ". " + sample.currentFilename + 
                             " (Root: '" + sample.rootNote + "', Type: '" + sample.chordType + "')\n");
        }
    }
    
    // Send results to UI
    logFile.appendText("Sending results to UI...\n");
    uiBridge->sendSampleResults(currentResults);
    uiBridge->sendLoadingState(false);
    logFile.appendText("Results sent to UI\n");
    
    logFile.appendText("==============================\n\n");
    
    juce::Logger::writeToLog("Search completed with " + juce::String(currentResults.size()) + " results");
}

void ChopsBrowserPluginEditor::parseQueryIntoCriteria(const juce::String& query, ChopsBrowserPluginProcessor::SearchCriteria& criteria)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== PARSING QUERY INTO CRITERIA ===\n");
    logFile.appendText("Input query: '" + query + "'\n");
    
    // Simple query parsing - can be enhanced
    juce::String trimmedQuery = query.trim();
    
    if (trimmedQuery.isEmpty())
    {
        logFile.appendText("Empty query - returning all samples\n");
        // Empty query - return all samples
        criteria.searchText = "";
        criteria.rootNote = "";
        criteria.chordType = "";
        logFile.appendText("================================\n\n");
        return;
    }
    
    logFile.appendText("Trimmed query: '" + trimmedQuery + "'\n");
    
    // FIXED: Try to parse as chord notation (e.g., "C", "Cmaj7", "Am", "F#dim")
    ChordParser parser;
    
    // First try to parse as a simple chord name by adding a dummy filename extension
    juce::String testFilename = trimmedQuery + ".wav";
    auto parsedChord = parser.parseFilename(testFilename);
    
    logFile.appendText("Chord parser results (first attempt):\n");
    logFile.appendText("  - Root note: '" + parsedChord.rootNote + "'\n");
    logFile.appendText("  - Standardized quality: '" + parsedChord.standardizedQuality + "'\n");
    logFile.appendText("  - Issues: " + parsedChord.issues.joinIntoString(", ") + "\n");
    
    // If that didn't work, try some common chord patterns
    if (parsedChord.rootNote.isEmpty())
    {
        logFile.appendText("First attempt failed, trying alternative patterns...\n");
        
        // Try with common chord suffixes
        juce::StringArray testPatterns = {
            trimmedQuery + "maj.wav",  // For "C" -> "Cmaj"
            trimmedQuery + "min.wav",  // For "C" -> "Cmin"  
            trimmedQuery + "_chord.wav", // Generic chord pattern
            "chord_" + trimmedQuery + ".wav", // Alternative pattern
        };
        
        for (const auto& pattern : testPatterns)
        {
            auto testParsed = parser.parseFilename(pattern);
            if (!testParsed.rootNote.isEmpty())
            {
                parsedChord = testParsed;
                logFile.appendText("Alternative pattern '" + pattern + "' worked!\n");
                break;
            }
        }
        
        // FIXED: Manual chord recognition for simple cases
        if (parsedChord.rootNote.isEmpty())
        {
            logFile.appendText("Parser failed, trying manual recognition...\n");
            
            // Check if it's a simple root note (C, D, E, F, G, A, B with optional # or b)
            juce::String upperQuery = trimmedQuery.toUpperCase();
            if (upperQuery.length() >= 1 && upperQuery.length() <= 2)
            {
                juce::String rootCandidate = upperQuery.substring(0, 1);
                if (rootCandidate.containsAnyOf("CDEFGAB"))
                {
                    // Check for sharp or flat
                    if (upperQuery.length() == 2)
                    {
                        juce::String accidental = upperQuery.substring(1, 2);
                        if (accidental == "#" || accidental == "B")
                        {
                            rootCandidate += accidental;
                        }
                    }
                    
                    parsedChord.rootNote = rootCandidate;
                    parsedChord.standardizedQuality = "maj"; // Default to major
                    logFile.appendText("Manual recognition: Root='" + rootCandidate + "', Quality='maj'\n");
                }
            }
            
            // Check for common chord patterns manually
            if (parsedChord.rootNote.isEmpty())
            {
                // Simple pattern matching for common chords
                if (upperQuery.endsWith("MAJ7") || upperQuery.endsWith("M7"))
                {
                    parsedChord.rootNote = upperQuery.substring(0, upperQuery.length() - 4);
                    parsedChord.standardizedQuality = "maj7";
                }
                else if (upperQuery.endsWith("MIN7") || upperQuery.endsWith("M7"))
                {
                    parsedChord.rootNote = upperQuery.substring(0, upperQuery.length() - 4);
                    parsedChord.standardizedQuality = "min7";
                }
                else if (upperQuery.endsWith("MAJ"))
                {
                    parsedChord.rootNote = upperQuery.substring(0, upperQuery.length() - 3);
                    parsedChord.standardizedQuality = "maj";
                }
                else if (upperQuery.endsWith("MIN") || upperQuery.endsWith("M"))
                {
                    parsedChord.rootNote = upperQuery.substring(0, upperQuery.length() - 3);
                    parsedChord.standardizedQuality = "min";
                }
                else if (upperQuery.endsWith("7"))
                {
                    parsedChord.rootNote = upperQuery.substring(0, upperQuery.length() - 1);
                    parsedChord.standardizedQuality = "dom7";
                }
                
                if (!parsedChord.rootNote.isEmpty())
                {
                    logFile.appendText("Pattern matching: Root='" + parsedChord.rootNote + "', Quality='" + parsedChord.standardizedQuality + "'\n");
                }
            }
        }
    }
    
    if (!parsedChord.rootNote.isEmpty() && !parsedChord.standardizedQuality.isEmpty())
    {
        // Parsed as chord - use structured search
        criteria.rootNote = parsedChord.rootNote;
        criteria.chordType = parsedChord.standardizedQuality;
        criteria.searchText = ""; // Clear text search when using structured search
        
        logFile.appendText("✅ Parsed as chord: " + criteria.rootNote + " " + criteria.chordType + "\n");
        juce::Logger::writeToLog("Parsed as chord: " + criteria.rootNote + " " + criteria.chordType);
    }
    else
    {
        // Use as text search
        criteria.searchText = trimmedQuery;
        criteria.rootNote = "";
        criteria.chordType = "";
        
        logFile.appendText("✅ Using text search: " + criteria.searchText + "\n");
        juce::Logger::writeToLog("Using text search: " + criteria.searchText);
    }
    
    logFile.appendText("Final criteria:\n");
    logFile.appendText("  - Root note: '" + criteria.rootNote + "'\n");
    logFile.appendText("  - Chord type: '" + criteria.chordType + "'\n");
    logFile.appendText("  - Search text: '" + criteria.searchText + "'\n");
    logFile.appendText("================================\n\n");
}

void ChopsBrowserPluginEditor::handleChordSelected(const ChordParser::ParsedData& chordData)
{
    juce::Logger::writeToLog("Chord selected: " + chordData.rootNote + " " + chordData.standardizedQuality);
    
    // Trigger search based on selected chord
    ChopsBrowserPluginProcessor::SearchCriteria criteria;
    criteria.rootNote = chordData.rootNote;
    criteria.chordType = chordData.standardizedQuality;
    
    if (uiBridge)
    {
        uiBridge->sendLoadingState(true);
        currentResults = audioProcessor.searchSamples(criteria);
        uiBridge->sendSampleResults(currentResults);
        uiBridge->sendLoadingState(false);
    }
}

void ChopsBrowserPluginEditor::handleSampleSelected(int sampleId)
{
    juce::Logger::writeToLog("Sample selected: " + juce::String(sampleId));
    
    // Find the selected sample
    auto it = std::find_if(currentResults.begin(), currentResults.end(),
                          [sampleId](const ChopsDatabase::SampleInfo& sample) {
                              return sample.id == sampleId;
                          });
    
    if (it != currentResults.end())
    {
        selectedSampleIndex = static_cast<int>(std::distance(currentResults.begin(), it));
        
        // Load sample for preview
        audioProcessor.loadSampleForPreview(it->filePath);
        
        // Send updated sample info to UI
        if (uiBridge)
        {
            uiBridge->sendSelectedSample(*it);
        }
        
        juce::Logger::writeToLog("Sample loaded: " + it->currentFilename);
    }
    else
    {
        juce::Logger::writeToLog("Sample with ID " + juce::String(sampleId) + " not found");
    }
}

//==============================================================================
void ChopsBrowserPluginEditor::handlePreviewPlay()
{
    juce::Logger::writeToLog("Preview play requested");
    audioProcessor.playPreview();
}

void ChopsBrowserPluginEditor::handlePreviewStop()
{
    juce::Logger::writeToLog("Preview stop requested");
    audioProcessor.stopPreview();
}

void ChopsBrowserPluginEditor::handlePreviewSeek(float position)
{
    juce::Logger::writeToLog("Preview seek requested: " + juce::String(position * 100.0f, 1) + "%");
    audioProcessor.seekPreview(position);
}

//==============================================================================
void ChopsBrowserPluginEditor::handleSampleRatingChanged(int sampleId, int rating)
{
    juce::Logger::writeToLog("Rating changed for sample " + juce::String(sampleId) + ": " + juce::String(rating));
    
    // Update via database manager if available
    auto* dbManager = audioProcessor.getDatabaseManager();
    if (dbManager)
    {
        dbManager->setRating(sampleId, rating);
    }
}

void ChopsBrowserPluginEditor::handleSampleTagAdded(int sampleId, const juce::String& tag)
{
    juce::Logger::writeToLog("Tag added to sample " + juce::String(sampleId) + ": " + tag);
    
    // Update via database manager if available
    auto* dbManager = audioProcessor.getDatabaseManager();
    if (dbManager)
    {
        dbManager->addTag(sampleId, tag);
    }
}

void ChopsBrowserPluginEditor::handleSampleFavoriteToggled(int sampleId)
{
    juce::Logger::writeToLog("Favorite toggled for sample " + juce::String(sampleId));
    
    // Update via database manager if available
    auto* dbManager = audioProcessor.getDatabaseManager();
    if (dbManager)
    {
        dbManager->toggleFavorite(sampleId);
    }
}

//==============================================================================
void ChopsBrowserPluginEditor::handleLibraryPathChanged(const juce::String& path)
{
    juce::Logger::writeToLog("Library path changed: " + path);
    audioProcessor.setChopsLibraryPath(path);
}

void ChopsBrowserPluginEditor::handleLibraryRescanRequested()
{
    juce::Logger::writeToLog("Library rescan requested");
    
    if (uiBridge)
    {
        uiBridge->sendLoadingState(true);
        // Note: Rescanning would typically be handled by the standalone app
        // Here we just refresh the current search
        if (!currentResults.empty())
        {
            ChopsBrowserPluginProcessor::SearchCriteria criteria;
            criteria.searchText = ""; // Reload all
            currentResults = audioProcessor.searchSamples(criteria);
            uiBridge->sendSampleResults(currentResults);
        }
        uiBridge->sendLoadingState(false);
    }
}

void ChopsBrowserPluginEditor::handleUIEvent(const juce::String& eventType, const juce::var& eventData)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("🎛️ UI EVENT: " + eventType + "\n");
    juce::Logger::writeToLog("UI event received: " + eventType);
    
    if (eventType == "chopsie_daisy")
    {
        // Handle Chopsie Daisy effects window request
        juce::Logger::writeToLog("Chopsie Daisy effects requested - not implemented yet");
        
        if (uiBridge)
        {
            uiBridge->sendErrorMessage("Chopsie Daisy effects not yet implemented");
        }
    }
    else if (eventType == "bridgeReady")
    {
        // FIXED: UI is ready - send comprehensive initial data
        logFile.appendText("🚀 BRIDGE READY - Sending initial data to React\n");
        sendCurrentData();
        
        // FIXED: Also send database statistics if available
        if (audioProcessor.isDatabaseAvailable())
        {
            auto* db = audioProcessor.getDatabaseManager()->getReadDatabase();
            if (db)
            {
                auto stats = db->getStatistics();
                if (uiBridge)
                {
                    uiBridge->sendDatabaseStats(stats);
                    logFile.appendText("📊 Database stats sent to React\n");
                }
            }
        }
        
        // FIXED: Send library path
        if (uiBridge)
        {
            uiBridge->sendLibraryPath(audioProcessor.getChopsLibraryPath());
            logFile.appendText("📚 Library path sent to React\n");
        }
        
        // FIXED: Trigger an initial search to populate the UI
        logFile.appendText("🔍 Triggering initial search for React\n");
        handleSearchRequested(""); // Empty search to show some samples
    }
    else
    {
        // For any other event types, we might use eventData in the future
        juce::ignoreUnused(eventData);
        logFile.appendText("❓ Unknown UI event type: " + eventType + "\n");
        juce::Logger::writeToLog("Unknown UI event type: " + eventType);
    }
}

//==============================================================================
void ChopsBrowserPluginEditor::loadUI()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    if (useReactUI && uiBridge)
    {
        logFile.appendText("Loading React UI via UIBridge\n");
        loadReactUI();
    }
    else
    {
        logFile.appendText("Creating fallback UI (no React/UIBridge)\n");
        createFallbackUI();
    }
}

void ChopsBrowserPluginEditor::loadReactUI()
{
    if (!uiBridge) return;
    
    juce::Logger::writeToLog("Loading React UI");
    
    // Try to load external HTML file first
    juce::File htmlFile = getUIHtmlFile();
    
    if (htmlFile.existsAsFile())
    {
        juce::Logger::writeToLog("Loading UI from file: " + htmlFile.getFullPathName());
        uiBridge->loadUI(htmlFile);
    }
    else
    {
        // Load inline HTML
        juce::Logger::writeToLog("Loading inline UI");
        juce::String htmlContent = generateInlineHTML();
        uiBridge->loadUI(htmlContent);
    }
}

void ChopsBrowserPluginEditor::createFallbackUI()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("Creating fallback UI component\n");
    
    // Create a simple JUCE component as fallback
    fallbackUI = std::make_unique<juce::Component>();
    
    // Create a custom component with basic functionality
    auto* container = new juce::Component();
    
    // Header
    auto* titleLabel = new juce::Label("title", "Chops Browser VST3");
    titleLabel->setBounds(20, 20, 400, 40);
    titleLabel->setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel->setColour(juce::Label::textColourId, juce::Colour(0xff4CAF50));
    container->addAndMakeVisible(titleLabel);
    
    // Status label
    auto* statusLabel = new juce::Label("status", "WebBrowser UI failed to load - using fallback");
    statusLabel->setBounds(20, 70, 600, 30);
    statusLabel->setFont(juce::FontOptions(16.0f));
    statusLabel->setColour(juce::Label::textColourId, juce::Colours::orange);
    container->addAndMakeVisible(statusLabel);
    
    // Info text
    auto* infoLabel = new juce::Label("info", "Check ChopsBrowser_VST_Debug.log on your Desktop for details");
    infoLabel->setBounds(20, 110, 600, 25);
    infoLabel->setFont(juce::FontOptions(14.0f));
    infoLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    container->addAndMakeVisible(infoLabel);
    
    // Test search button
    auto* testButton = new juce::TextButton("Test Search");
    testButton->setBounds(20, 150, 150, 35);
    testButton->onClick = [this]() {
        handleSearchRequested("C");
        juce::Logger::writeToLog("Fallback UI: Test search triggered");
    };
    container->addAndMakeVisible(testButton);
    
    // Simple sample list
    auto* listLabel = new juce::Label("list", "Sample Results:");
    listLabel->setBounds(20, 200, 200, 25);
    listLabel->setFont(juce::FontOptions(16.0f, juce::Font::bold));
    container->addAndMakeVisible(listLabel);
    
    auto* listBox = new juce::ListBox();
    listBox->setBounds(20, 230, 800, 300);
    container->addAndMakeVisible(listBox);
    
    fallbackUI->addAndMakeVisible(container);
    addAndMakeVisible(*fallbackUI);
    
    // Set bounds to current size
    auto bounds = getLocalBounds();
    fallbackUI->setBounds(bounds);
    container->setBounds(bounds);
    
    logFile.appendText("Fallback UI created and sized to: " + bounds.toString() + "\n");
    juce::Logger::writeToLog("Fallback UI created");
}

void ChopsBrowserPluginEditor::switchToFallbackUI()
{
    useReactUI = false;
    if (uiBridge) {
        uiBridge->setVisible(false);
    }
    createFallbackUI();
}

juce::String ChopsBrowserPluginEditor::createHTMLContent()
{
    return generateInlineHTML();
}

//==============================================================================
void ChopsBrowserPluginEditor::updateUIState()
{
    if (!uiBridge) return;
    
    // Send current preview state
    uiBridge->sendPreviewState(audioProcessor.isPreviewPlaying(), 
                              audioProcessor.getPreviewProgress());
    
    // Send database stats if available
    if (audioProcessor.isDatabaseAvailable())
    {
        auto* db = audioProcessor.getDatabaseManager()->getReadDatabase();
        if (db)
        {
            auto stats = db->getStatistics();
            uiBridge->sendDatabaseStats(stats);
        }
    }
    
    // Send library path
    uiBridge->sendLibraryPath(audioProcessor.getChopsLibraryPath());
}

void ChopsBrowserPluginEditor::sendCurrentData()
{
    if (!uiBridge) return;
    
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("📤 SENDING CURRENT DATA TO REACT\n");
    juce::Logger::writeToLog("Sending current data to UI");
    
    // Send current search results if any
    if (!currentResults.empty())
    {
        logFile.appendText("📦 Sending " + juce::String(currentResults.size()) + " existing results\n");
        uiBridge->sendSampleResults(currentResults);
    }
    else
    {
        // FIXED: If no current results, perform a quick search to get some data
        logFile.appendText("🔍 No current results, performing initial search\n");
        ChopsBrowserPluginProcessor::SearchCriteria criteria;
        criteria.searchText = ""; // Empty search to get some samples
        
        auto initialResults = audioProcessor.searchSamples(criteria);
        logFile.appendText("📦 Initial search returned " + juce::String(initialResults.size()) + " samples\n");
        
        if (!initialResults.empty())
        {
            currentResults = initialResults;
            uiBridge->sendSampleResults(currentResults);
        }
    }
    
    // Update UI state
    updateUIState();
    
    logFile.appendText("✅ Current data sent to React\n");
}

//==============================================================================
juce::File ChopsBrowserPluginEditor::getUIHtmlFile()
{
    // Try various locations for the UI HTML file
    juce::StringArray possiblePaths = {
        // Development paths
        "UI/dist/index.html",
        "../UI/dist/index.html",
        "../../UI/dist/index.html",
        
        // Installed paths
        "Resources/UI/index.html",
        "../Resources/UI/index.html"
    };
    
    for (const auto& path : possiblePaths)
    {
        juce::File candidate = juce::File::getCurrentWorkingDirectory().getChildFile(path);
        if (candidate.existsAsFile())
        {
            return candidate;
        }
    }
    
    return juce::File();
}

juce::String ChopsBrowserPluginEditor::generateInlineHTML()
{
    return juce::String(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "    <meta charset=\"UTF-8\">"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "    <title>Chops Browser</title>"
        "    <style>"
        "        body {"
        "            margin: 0;"
        "            padding: 0;"
        "            background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);"
        "            color: #e0e0e0;"
        "            font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", \"Roboto\", sans-serif;"
        "            overflow: hidden;"
        "        }"
        "        .chops-container {"
        "            width: 100vw;"
        "            height: 100vh;"
        "            display: flex;"
        "            flex-direction: column;"
        "        }"
        "        .header {"
        "            display: flex;"
        "            align-items: center;"
        "            padding: 12px 20px;"
        "            background: rgba(0, 0, 0, 0.3);"
        "            border-bottom: 1px solid #444;"
        "            gap: 20px;"
        "        }"
        "        .logo {"
        "            font-size: 18px;"
        "            font-weight: bold;"
        "            color: #4caf50;"
        "        }"
        "        .current-chord {"
        "            flex: 1;"
        "            text-align: center;"
        "        }"
        "        .chord-symbol {"
        "            font-size: 24px;"
        "            font-weight: bold;"
        "            color: #4caf50;"
        "        }"
        "        .chord-details {"
        "            font-size: 12px;"
        "            color: #888;"
        "        }"
        "        .search-box {"
        "            padding: 8px 12px;"
        "            background: rgba(255, 255, 255, 0.1);"
        "            border: 1px solid #555;"
        "            border-radius: 4px;"
        "            color: #e0e0e0;"
        "            width: 200px;"
        "        }"
        "        .main-content {"
        "            flex: 1;"
        "            display: flex;"
        "            flex-direction: column;"
        "            overflow: hidden;"
        "        }"
        "        .piano-section {"
        "            padding: 20px;"
        "            background: rgba(0, 0, 0, 0.2);"
        "            border-bottom: 1px solid #444;"
        "        }"
        "        .piano-container {"
        "            margin: 0 auto;"
        "            max-width: 600px;"
        "            height: 80px;"
        "            background: #f5f5f5;"
        "            border-radius: 8px;"
        "            display: flex;"
        "            position: relative;"
        "        }"
        "        .piano-key {"
        "            flex: 1;"
        "            background: white;"
        "            border: 1px solid #ccc;"
        "            cursor: pointer;"
        "            transition: all 0.1s ease;"
        "            display: flex;"
        "            align-items: flex-end;"
        "            justify-content: center;"
        "            padding-bottom: 8px;"
        "            font-size: 10px;"
        "            color: #666;"
        "        }"
        "        .piano-key:hover {"
        "            background: #f0f0f0;"
        "        }"
        "        .piano-key.active {"
        "            background: #4caf50;"
        "            color: white;"
        "        }"
        "        .results-area {"
        "            flex: 1;"
        "            padding: 20px;"
        "            overflow-y: auto;"
        "        }"
        "        .results-header {"
        "            display: flex;"
        "            justify-content: space-between;"
        "            margin-bottom: 16px;"
        "            padding-bottom: 8px;"
        "            border-bottom: 1px solid #444;"
        "        }"
        "        .sample-item {"
        "            padding: 8px 12px;"
        "            border-bottom: 1px solid #333;"
        "            cursor: pointer;"
        "            transition: background 0.2s ease;"
        "        }"
        "        .sample-item:hover {"
        "            background: rgba(255, 255, 255, 0.05);"
        "        }"
        "        .sample-item.selected {"
        "            background: rgba(76, 175, 80, 0.2);"
        "        }"
        "        .chord-name {"
        "            font-weight: bold;"
        "            color: #4caf50;"
        "        }"
        "        .filename {"
        "            font-size: 12px;"
        "            color: #888;"
        "            margin-top: 2px;"
        "        }"
        "        .test-buttons {"
        "            display: flex;"
        "            gap: 10px;"
        "            margin: 20px;"
        "            justify-content: center;"
        "        }"
        "        .test-btn {"
        "            padding: 10px 20px;"
        "            background: #4caf50;"
        "            color: white;"
        "            border: none;"
        "            border-radius: 4px;"
        "            cursor: pointer;"
        "            font-size: 14px;"
        "        }"
        "        .test-btn:hover {"
        "            background: #45a049;"
        "        }"
        "        .status {"
        "            padding: 10px 20px;"
        "            background: rgba(0, 0, 0, 0.3);"
        "            border-top: 1px solid #444;"
        "            font-size: 12px;"
        "            color: #888;"
        "        }"
        "    </style>"
        "</head>"
        "<body>"
        "    <div class=\"chops-container\">"
        "        <div class=\"header\">"
        "            <div class=\"logo\">Chops Browser</div>"
        "            <div class=\"current-chord\">"
        "                <div class=\"chord-symbol\" id=\"currentChord\">C</div>"
        "                <div class=\"chord-details\" id=\"chordDetails\">Select a chord</div>"
        "            </div>"
        "            <input type=\"text\" class=\"search-box\" placeholder=\"Search chords...\" id=\"searchInput\">"
        "        </div>"
        "        <div class=\"main-content\">"
        "            <div class=\"piano-section\">"
        "                <div class=\"piano-container\" id=\"pianoContainer\">"
        "                </div>"
        "            </div>"
        "            <div class=\"results-area\">"
        "                <div class=\"results-header\">"
        "                    <div id=\"resultsCount\">0 samples found</div>"
        "                    <div>List View</div>"
        "                </div>"
        "                <div id=\"samplesList\">"
        "                </div>"
        "            </div>"
        "        </div>"
        "        <div class=\"test-buttons\">"
        "            <button class=\"test-btn\" onclick=\"testSearch()\">Test Search</button>"
        "            <button class=\"test-btn\" onclick=\"testChord()\">Test Chord</button>"
        "        </div>"
        "        <div class=\"status\" id=\"statusBar\">Ready</div>"
        "    </div>"
        "    <script>"
        "        document.addEventListener('DOMContentLoaded', function() {"
        "            console.log('Chops Browser UI loading...');"
        "            generatePiano();"
        "            setupSearch();"
        "            initializeBridge();"
        "        });"
        "        function generatePiano() {"
        "            const container = document.getElementById('pianoContainer');"
        "            const whiteKeys = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];"
        "            whiteKeys.forEach(note => {"
        "                const key = document.createElement('div');"
        "                key.className = 'piano-key';"
        "                key.textContent = note;"
        "                key.onclick = () => selectNote(note);"
        "                container.appendChild(key);"
        "            });"
        "        }"
        "        function setupSearch() {"
        "            const searchInput = document.getElementById('searchInput');"
        "            searchInput.addEventListener('input', function(e) {"
        "                performSearch(e.target.value);"
        "            });"
        "        }"
        "        function selectNote(note) {"
        "            console.log('Note selected:', note);"
        "            document.getElementById('currentChord').textContent = note;"
        "            document.getElementById('chordDetails').textContent = note + ' major';"
        "            document.querySelectorAll('.piano-key').forEach(k => k.classList.remove('active'));"
        "            event.target.classList.add('active');"
        "            performSearch(note);"
        "        }"
        "        function performSearch(query) {"
        "            console.log('Performing search for:', query);"
        "            document.getElementById('statusBar').textContent = 'Searching for: ' + query;"
        "            if (window.ChopsBridge) {"
        "                window.ChopsBridge.sendMessage('searchRequested', { query: query });"
        "            } else {"
        "                displayMockResults(query);"
        "            }"
        "        }"
        "        function displayMockResults(query) {"
        "            const mockSamples = ["
        "                { id: 1, chordTypeDisplay: query + 'maj7', currentFilename: 'Piano_' + query + 'maj7.wav' },"
        "                { id: 2, chordTypeDisplay: query + 'm7', currentFilename: 'Guitar_' + query + 'm7.wav' },"
        "                { id: 3, chordTypeDisplay: query + '7', currentFilename: 'Synth_' + query + '7.wav' }"
        "            ];"
        "            displaySampleResults(mockSamples);"
        "        }"
        "        function displaySampleResults(samples) {"
        "            const samplesList = document.getElementById('samplesList');"
        "            const resultsCount = document.getElementById('resultsCount');"
        "            resultsCount.textContent = samples.length + ' samples found';"
        "            samplesList.innerHTML = '';"
        "            samples.forEach(sample => {"
        "                const item = document.createElement('div');"
        "                item.className = 'sample-item';"
        "                item.innerHTML = '<div class=\"chord-name\">' + sample.chordTypeDisplay + '</div><div class=\"filename\">' + sample.currentFilename + '</div>';"
        "                item.onclick = () => selectSample(sample);"
        "                samplesList.appendChild(item);"
        "            });"
        "        }"
        "        function selectSample(sample) {"
        "            console.log('Sample selected:', sample);"
        "            document.querySelectorAll('.sample-item').forEach(item => item.classList.remove('selected'));"
        "            event.target.classList.add('selected');"
        "            if (window.ChopsBridge) {"
        "                window.ChopsBridge.sendMessage('sampleSelected', { id: sample.id, filePath: sample.filePath || sample.currentFilename });"
        "            }"
        "        }"
        "        function testSearch() {"
        "            console.log('Testing search functionality...');"
        "            performSearch('Cmaj7');"
        "        }"
        "        function testChord() {"
        "            console.log('Testing chord selection...');"
        "            selectNote('G');"
        "        }"
        "        function initializeBridge() {"
        "            setTimeout(function() {"
        "                if (window.ChopsBridge) {"
        "                    console.log('ChopsBridge found, setting up callbacks...');"
        "                    window.ChopsBridge.setCallback('onSampleResults', function(samples) {"
        "                        console.log('Received sample results:', samples);"
        "                        displaySampleResults(samples);"
        "                    });"
        "                    window.ChopsBridge.setCallback('onLoadingState', function(loading) {"
        "                        document.getElementById('statusBar').textContent = loading ? 'Loading...' : 'Ready';"
        "                    });"
        "                    window.ChopsBridge.setCallback('onErrorMessage', function(error) {"
        "                        document.getElementById('statusBar').textContent = 'Error: ' + error;"
        "                    });"
        "                    window.ChopsBridge.sendMessage('bridgeReady', {});"
        "                    console.log('Chops Browser Bridge initialized');"
        "                } else {"
        "                    console.log('ChopsBridge not available, running in standalone mode');"
        "                    document.getElementById('statusBar').textContent = 'Running in test mode';"
        "                }"
        "            }, 1000);"
        "        }"
        "    </script>"
        "</body>"
        "</html>"
    );
}
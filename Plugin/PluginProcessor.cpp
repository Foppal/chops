#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Source/Shared/SharedConfig.h"

//==============================================================================
ChopsBrowserPluginProcessor::ChopsBrowserPluginProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Create debug log file
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== PLUGIN PROCESSOR INITIALIZATION ===\n");
    logFile.appendText("Time: " + juce::Time::getCurrentTime().toString(true, true, true, true) + "\n");
    
    // Initialize format manager first
    formatManager = std::make_unique<juce::AudioFormatManager>();
    initializeAudioFormats();
    
    // Initialize transport source with a dummy callback
    transportSource.addChangeListener(nullptr);
    
    // Initialize database manager - try to find and connect to existing database
    logFile.appendText("Starting database initialization...\n");
    initializeDatabase();
    logFile.appendText("Database initialization completed\n");
    logFile.appendText("=======================================\n\n");
}

ChopsBrowserPluginProcessor::~ChopsBrowserPluginProcessor()
{
    transportSource.setSource(nullptr);
    readerSource.reset();
}

//==============================================================================
void ChopsBrowserPluginProcessor::initializeAudioFormats()
{
    // Register all the basic formats FIRST
    formatManager->registerBasicFormats(); // WAV, AIFF, etc.
    
    // Add additional format support if needed
    #if JUCE_USE_MP3AUDIOFORMAT
    formatManager->registerFormat(new juce::MP3AudioFormat(), false);
    #endif
}

void ChopsBrowserPluginProcessor::initializeDatabase()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== DATABASE INITIALIZATION ===\n");
    
    // Try to find the database from default location first
    juce::File defaultDb = ChopsConfig::getDatabaseFile();
    logFile.appendText("Default database path: " + defaultDb.getFullPathName() + "\n");
    logFile.appendText("Default database exists: " + juce::String(defaultDb.existsAsFile() ? "YES" : "NO") + "\n");
    
    if (defaultDb.existsAsFile())
    {
        logFile.appendText("Found database at default location: " + defaultDb.getFullPathName() + "\n");
        setDatabasePath(defaultDb.getFullPathName());
        chopsLibraryPath = ChopsConfig::getDefaultLibraryDirectory().getFullPathName();
        logFile.appendText("Set library path to: " + chopsLibraryPath + "\n");
    }
    else
    {
        logFile.appendText("Default database not found, searching alternative locations...\n");
        
        // Try to find database in common locations
        juce::StringArray searchPaths = {
            // User documents
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile("Chops Library").getChildFile("chops_library.sqlite").getFullPathName(),
            
            // Desktop
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                .getChildFile("Chops Library").getChildFile("chops_library.sqlite").getFullPathName(),
            
            // Home directory
            juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                .getChildFile("Chops Library").getChildFile("chops_library.sqlite").getFullPathName(),
                
            // Application directory
            juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                .getParentDirectory().getChildFile("chops_library.sqlite").getFullPathName()
        };
        
        bool found = false;
        for (int i = 0; i < searchPaths.size(); ++i)
        {
            const auto& path = searchPaths[i];
            juce::File candidate(path);
            logFile.appendText("  " + juce::String(i+1) + ". Checking: " + path + " -> " + 
                             juce::String(candidate.existsAsFile() ? "FOUND" : "NOT FOUND") + "\n");
            
            if (candidate.existsAsFile())
            {
                logFile.appendText("Found database at: " + path + "\n");
                setDatabasePath(path);
                chopsLibraryPath = candidate.getParentDirectory().getFullPathName();
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            logFile.appendText("ERROR: No database found in any location!\n");
            logFile.appendText("Plugin will operate without sample data\n");
            juce::Logger::writeToLog("No database found - plugin will operate without sample data");
        }
    }
    
    // Test database connection
    if (isDatabaseAvailable())
    {
        auto* db = databaseManager.getReadDatabase();
        if (db)
        {
            auto stats = db->getStatistics();
            logFile.appendText("Database connected successfully!\n");
            logFile.appendText("Database stats:\n");
            logFile.appendText("  - Total samples: " + juce::String(stats.totalSamples) + "\n");
            logFile.appendText("  - With extensions: " + juce::String(stats.withExtensions) + "\n");
            logFile.appendText("  - With alterations: " + juce::String(stats.withAlterations) + "\n");
            logFile.appendText("  - Added last week: " + juce::String(stats.addedLastWeek) + "\n");
            
            // Test a simple search
            logFile.appendText("Testing simple database search...\n");
            auto testResults = db->searchSamples("", "", "", ChopsDatabase::DontCare, ChopsDatabase::DontCare, 5, 0);
            logFile.appendText("Test search returned " + juce::String(testResults.size()) + " samples\n");
            
            if (!testResults.empty())
            {
                logFile.appendText("Sample test results:\n");
                for (int i = 0; i < juce::jmin(3, (int)testResults.size()); ++i)
                {
                    const auto& sample = testResults[i];
                    logFile.appendText("  " + juce::String(i+1) + ". " + sample.currentFilename + 
                                     " (Root: " + sample.rootNote + ", Type: " + sample.chordType + ")\n");
                }
            }
        }
        else
        {
            logFile.appendText("ERROR: Database manager created but read database is null!\n");
        }
    }
    else
    {
        logFile.appendText("ERROR: Database not available after initialization!\n");
    }
    
    logFile.appendText("==============================\n\n");
}

//==============================================================================
const juce::String ChopsBrowserPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChopsBrowserPluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ChopsBrowserPluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ChopsBrowserPluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ChopsBrowserPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ChopsBrowserPluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ChopsBrowserPluginProcessor::getCurrentProgram()
{
    return 0;
}

void ChopsBrowserPluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String ChopsBrowserPluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ChopsBrowserPluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void ChopsBrowserPluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    transportSource.prepareToPlay(samplesPerBlock, sampleRate);
}

void ChopsBrowserPluginProcessor::releaseResources()
{
    transportSource.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChopsBrowserPluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ChopsBrowserPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Preview audio playback
    if (previewIsPlaying)
    {
        juce::AudioSourceChannelInfo info(buffer);
        transportSource.getNextAudioBlock(info);
        
        // Check if preview has finished
        if (!transportSource.isPlaying())
        {
            previewIsPlaying = false;
            sendChangeMessage(); // Notify UI
        }
    }
    else
    {
        // When not previewing, just pass through input
        // (or silence if this is being used as a generator)
    }
}

//==============================================================================
bool ChopsBrowserPluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ChopsBrowserPluginProcessor::createEditor()
{
    return new ChopsBrowserPluginEditor (*this);
}

//==============================================================================
void ChopsBrowserPluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Store plugin state
    juce::XmlElement xml("ChopsBrowserState");
    
    xml.setAttribute("libraryPath", chopsLibraryPath);
    xml.setAttribute("databasePath", currentDatabasePath);
    
    // Store last search query
    xml.setAttribute("lastSearchQuery", lastSearchQuery);
    
    copyXmlToBinary(xml, destData);
}

void ChopsBrowserPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore plugin state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState != nullptr && xmlState->hasTagName("ChopsBrowserState"))
    {
        chopsLibraryPath = xmlState->getStringAttribute("libraryPath");
        juce::String dbPath = xmlState->getStringAttribute("databasePath");
        lastSearchQuery = xmlState->getStringAttribute("lastSearchQuery");
        
        if (dbPath.isNotEmpty())
            setDatabasePath(dbPath);
    }
}

//==============================================================================
// Custom methods implementation

void ChopsBrowserPluginProcessor::setDatabasePath(const juce::String& path)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== SETTING DATABASE PATH ===\n");
    logFile.appendText("Path: " + path + "\n");
    
    currentDatabasePath = path;
    
    juce::File dbFile(path);
    logFile.appendText("File exists: " + juce::String(dbFile.existsAsFile() ? "YES" : "NO") + "\n");
    
    if (dbFile.existsAsFile())
    {
        logFile.appendText("File size: " + juce::String(dbFile.getSize()) + " bytes\n");
        
        if (databaseManager.initialize(juce::File(path)))
        {
            logFile.appendText("✅ Database initialized successfully: " + path + "\n");
            juce::Logger::writeToLog("Database initialized successfully: " + path);
            sendChangeMessage(); // Notify UI that database has changed
        }
        else
        {
            logFile.appendText("❌ Failed to initialize database: " + path + "\n");
            juce::Logger::writeToLog("Failed to initialize database: " + path);
        }
    }
    else
    {
        logFile.appendText("❌ Database file not found: " + path + "\n");
        juce::Logger::writeToLog("Database file not found: " + path);
    }
    
    logFile.appendText("============================\n\n");
}

void ChopsBrowserPluginProcessor::setChopsLibraryPath(const juce::String& path)
{
    chopsLibraryPath = path;
    
    // Look for database in the library path
    juce::File libraryDir(path);
    juce::File dbFile = libraryDir.getChildFile("chops_library.sqlite");
    
    if (dbFile.existsAsFile())
    {
        setDatabasePath(dbFile.getFullPathName());
    }
    else
    {
        juce::Logger::writeToLog("No database found in library path: " + path);
    }
}

std::vector<ChopsDatabase::SampleInfo> ChopsBrowserPluginProcessor::searchSamples(const SearchCriteria& criteria)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== SEARCH SAMPLES REQUEST ===\n");
    logFile.appendText("Time: " + juce::Time::getCurrentTime().toString(true, true, true, true) + "\n");
    logFile.appendText("Search criteria:\n");
    logFile.appendText("  - Root note: '" + criteria.rootNote + "'\n");
    logFile.appendText("  - Chord type: '" + criteria.chordType + "'\n");
    logFile.appendText("  - Search text: '" + criteria.searchText + "'\n");
    logFile.appendText("  - Min rating: " + juce::String(criteria.minRating) + "\n");
    logFile.appendText("  - Favorites only: " + juce::String(criteria.favoritesOnly ? "YES" : "NO") + "\n");
    
    auto* db = databaseManager.getReadDatabase();
    if (!db || !db->isOpen())
    {
        logFile.appendText("❌ ERROR: Database not available for search!\n");
        logFile.appendText("  - Database manager exists: " + juce::String(db ? "YES" : "NO") + "\n");
        if (db)
            logFile.appendText("  - Database is open: " + juce::String(db->isOpen() ? "YES" : "NO") + "\n");
        logFile.appendText("==============================\n\n");
        
        juce::Logger::writeToLog("Database not available for search");
        return {};
    }
    
    logFile.appendText("✅ Database is available and open\n");
    
    // Store the last search query for state saving
    if (!criteria.searchText.isEmpty())
        lastSearchQuery = criteria.searchText;
    else if (!criteria.rootNote.isEmpty() && !criteria.chordType.isEmpty())
        lastSearchQuery = criteria.rootNote + criteria.chordType;
    
    logFile.appendText("Calling database search...\n");
    
    // Use the database search method
    auto results = db->searchSamples(
        criteria.searchText,
        criteria.rootNote,
        criteria.chordType,
        criteria.filterByExtensions ? (criteria.hasExtensions ? ChopsDatabase::Yes : ChopsDatabase::No) : ChopsDatabase::DontCare,
        criteria.filterByAlterations ? (criteria.hasAlterations ? ChopsDatabase::Yes : ChopsDatabase::No) : ChopsDatabase::DontCare,
        100, // limit
        0    // offset
    );
    
    logFile.appendText("Database search completed\n");
    logFile.appendText("Results: " + juce::String(results.size()) + " samples found\n");
    
    if (results.size() > 0)
    {
        logFile.appendText("Sample results (first 3):\n");
        for (int i = 0; i < juce::jmin(3, (int)results.size()); ++i)
        {
            const auto& sample = results[i];
            logFile.appendText("  " + juce::String(i+1) + ". " + sample.currentFilename + 
                             " (ID: " + juce::String(sample.id) + 
                             ", Root: '" + sample.rootNote + 
                             "', Type: '" + sample.chordType + "')\n");
        }
    }
    else
    {
        logFile.appendText("❌ No samples found!\n");
        
        // Let's do a debug search with broader criteria
        logFile.appendText("Trying broader search for debugging...\n");
        auto debugResults = db->searchSamples("", "", "", ChopsDatabase::DontCare, ChopsDatabase::DontCare, 10, 0);
        logFile.appendText("Broad search found: " + juce::String(debugResults.size()) + " samples\n");
        
        if (!debugResults.empty())
        {
            logFile.appendText("Available samples in database:\n");
            for (int i = 0; i < juce::jmin(5, (int)debugResults.size()); ++i)
            {
                const auto& sample = debugResults[i];
                logFile.appendText("  " + juce::String(i+1) + ". " + sample.currentFilename + 
                                 " (Root: '" + sample.rootNote + 
                                 "', Type: '" + sample.chordType + "')\n");
            }
        }
    }
    
    logFile.appendText("==============================\n\n");
    
    juce::Logger::writeToLog("Database search returned " + juce::String(results.size()) + " results");
    
    return results;
}

void ChopsBrowserPluginProcessor::loadSampleForPreview(const juce::String& filePath)
{
    juce::File file(filePath);
    
    if (!file.existsAsFile())
    {
        juce::Logger::writeToLog("Preview file not found: " + filePath);
        return;
    }
    
    // Stop current preview
    stopPreview();
    
    // Check format manager is initialized
    if (!formatManager)
    {
        juce::Logger::writeToLog("Format manager not initialized!");
        return;
    }
    
    // Load new file
    auto* reader = formatManager->createReaderFor(file);
    
    if (reader != nullptr)
    {
        currentSamplePath = filePath;
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource = std::move(newSource);
        
        juce::Logger::writeToLog("Sample loaded for preview: " + file.getFileName());
    }
    else
    {
        juce::Logger::writeToLog("Could not create reader for file: " + filePath);
    }
}

void ChopsBrowserPluginProcessor::playPreview()
{
    if (readerSource != nullptr)
    {
        transportSource.setPosition(0);
        transportSource.start();
        previewIsPlaying = true;
        sendChangeMessage();
        
        juce::Logger::writeToLog("Preview started");
    }
    else
    {
        juce::Logger::writeToLog("No sample loaded for preview");
    }
}

void ChopsBrowserPluginProcessor::stopPreview()
{
    if (previewIsPlaying)
    {
        transportSource.stop();
        previewIsPlaying = false;
        sendChangeMessage();
        
        juce::Logger::writeToLog("Preview stopped");
    }
}

void ChopsBrowserPluginProcessor::seekPreview(float position)
{
    if (readerSource != nullptr && position >= 0.0f && position <= 1.0f)
    {
        double lengthInSeconds = transportSource.getLengthInSeconds();
        if (lengthInSeconds > 0.0)
        {
            double seekPosition = position * lengthInSeconds;
            transportSource.setPosition(seekPosition);
            
            juce::Logger::writeToLog("Preview seeked to: " + juce::String(position * 100.0f, 1) + "%");
        }
    }
}

float ChopsBrowserPluginProcessor::getPreviewProgress() const
{
    if (readerSource != nullptr && transportSource.getLengthInSeconds() > 0)
    {
        return static_cast<float>(transportSource.getCurrentPosition() / 
                                  transportSource.getLengthInSeconds());
    }
    return 0.0f;
}

bool ChopsBrowserPluginProcessor::isDatabaseAvailable() const
{
    auto* db = databaseManager.getReadDatabase();
    return db && db->isOpen();
}

juce::String ChopsBrowserPluginProcessor::getDatabaseInfo() const
{
    auto* db = databaseManager.getReadDatabase();
    if (!db || !db->isOpen())
        return "No database connected";
    
    return db->getDatabaseInfo();
}

//==============================================================================
// *** TEST METHODS IMPLEMENTATION ADDED HERE ***

void ChopsBrowserPluginProcessor::testDatabaseConnection()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== DATABASE CONNECTION TEST ===\n");
    logFile.appendText("Time: " + juce::Time::getCurrentTime().toString(true, true, true, true) + "\n");
    
    // Test 1: Check if database manager exists
    logFile.appendText("1. Database manager exists: " + juce::String(isDatabaseAvailable() ? "YES" : "NO") + "\n");
    
    if (!isDatabaseAvailable())
    {
        logFile.appendText("❌ Database not available - stopping test\n");
        logFile.appendText("Current database path: '" + currentDatabasePath + "'\n");
        logFile.appendText("Library path: '" + chopsLibraryPath + "'\n");
        logFile.appendText("==============================\n\n");
        return;
    }
    
    auto* db = databaseManager.getReadDatabase();
    
    // Test 2: Get database statistics
    auto stats = db->getStatistics();
    logFile.appendText("2. Database statistics:\n");
    logFile.appendText("   - Total samples: " + juce::String(stats.totalSamples) + "\n");
    logFile.appendText("   - With extensions: " + juce::String(stats.withExtensions) + "\n");
    logFile.appendText("   - With alterations: " + juce::String(stats.withAlterations) + "\n");
    logFile.appendText("   - Added last week: " + juce::String(stats.addedLastWeek) + "\n");
    
    // Test 3: Get distinct root notes
    auto rootNotes = db->getDistinctRootNotes();
    logFile.appendText("3. Distinct root notes (" + juce::String(rootNotes.size()) + "): " + rootNotes.joinIntoString(", ") + "\n");
    
    // Test 4: Get distinct chord types
    auto chordTypes = db->getDistinctChordTypes();
    logFile.appendText("4. Distinct chord types (" + juce::String(chordTypes.size()) + "): " + chordTypes.joinIntoString(", ") + "\n");
    
    // Test 5: Simple searches
    logFile.appendText("5. Testing various searches:\n");
    
    // Test search with no criteria (should return all)
    auto allResults = db->searchSamples("", "", "", ChopsDatabase::DontCare, ChopsDatabase::DontCare, 10, 0);
    logFile.appendText("   - All samples (limit 10): " + juce::String(allResults.size()) + " results\n");
    
    // Test search for C root note
    auto cResults = db->searchSamples("", "C", "", ChopsDatabase::DontCare, ChopsDatabase::DontCare, 10, 0);
    logFile.appendText("   - Root note 'C': " + juce::String(cResults.size()) + " results\n");
    
    // Test search for major chords
    auto majorResults = db->searchSamples("", "", "maj", ChopsDatabase::DontCare, ChopsDatabase::DontCare, 10, 0);
    logFile.appendText("   - Chord type 'maj': " + juce::String(majorResults.size()) + " results\n");
    
    // Test text search
    auto textResults = db->searchSamples("C", "", "", ChopsDatabase::DontCare, ChopsDatabase::DontCare, 10, 0);
    logFile.appendText("   - Text search 'C': " + juce::String(textResults.size()) + " results\n");
    
    // Test 6: Show some sample data
    if (allResults.size() > 0)
    {
        logFile.appendText("6. Sample data (first 5):\n");
        for (int i = 0; i < juce::jmin(5, (int)allResults.size()); ++i)
        {
            const auto& sample = allResults[i];
            logFile.appendText("   " + juce::String(i+1) + ". ID:" + juce::String(sample.id) + 
                             " File:'" + sample.currentFilename + 
                             "' Root:'" + sample.rootNote + 
                             "' Type:'" + sample.chordType + 
                             "' Display:'" + sample.chordTypeDisplay + "'\n");
        }
    }
    else
    {
        logFile.appendText("6. ❌ No sample data found in database!\n");
        logFile.appendText("   This means the database is empty or the search is failing.\n");
        
        // Check if we can create test data
        if (hasWriteAccess())
        {
            logFile.appendText("   Database has write access - can create test data\n");
        }
        else
        {
            logFile.appendText("   Database is read-only\n");
        }
    }
    
    logFile.appendText("==============================\n\n");
}

bool ChopsBrowserPluginProcessor::hasAnyData()
{
    if (!isDatabaseAvailable()) return false;
    
    auto* db = databaseManager.getReadDatabase();
    auto stats = db->getStatistics();
    return stats.totalSamples > 0;
}

bool ChopsBrowserPluginProcessor::hasWriteAccess()
{
    // For now, return true if we have a database connection
    // In a full implementation, you might check file permissions
    return isDatabaseAvailable();
}

void ChopsBrowserPluginProcessor::createTestData()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== CREATING TEST DATA ===\n");
    
    if (!isDatabaseAvailable())
    {
        logFile.appendText("❌ Cannot create test data - database not available\n");
        logFile.appendText("==========================\n\n");
        return;
    }
    
    // Create some basic test samples
    std::vector<TestSample> testSamples = {
        // C major chords
        {"Cmaj_test.wav", "C", "maj", "C"},
        {"Cmaj7_test.wav", "C", "maj7", "Cmaj7"},
        {"Cmaj9_test.wav", "C", "maj9", "Cmaj9"},
        
        // C minor chords
        {"Cm_test.wav", "C", "min", "Cm"},
        {"Cm7_test.wav", "C", "min7", "Cm7"},
        
        // Other root notes
        {"Dmaj_test.wav", "D", "maj", "D"},
        {"Em_test.wav", "E", "min", "Em"},
        {"Fmaj7_test.wav", "F", "maj7", "Fmaj7"},
        {"G7_test.wav", "G", "dom7", "G7"},
        {"Am_test.wav", "A", "min", "Am"},
        {"Bdim_test.wav", "B", "dim", "Bdim"}
    };
    
    logFile.appendText("Creating " + juce::String(testSamples.size()) + " test samples...\n");
    
    int createdCount = 0;
    for (const auto& testSample : testSamples)
    {
        ChopsDatabase::SampleInfo sample;
        sample.originalFilename = testSample.filename;
        sample.currentFilename = testSample.filename;
        sample.filePath = "/test/path/" + testSample.filename; // Dummy path
        sample.fileSize = 44100 * 4; // Dummy size
        sample.rootNote = testSample.rootNote;
        sample.chordType = testSample.chordType;
        sample.chordTypeDisplay = testSample.chordTypeDisplay;
        sample.rating = 3; // Default rating
        
        // Try to insert the sample
        int sampleId = databaseManager.insertProcessedSample(sample);
        if (sampleId > 0)
        {
            createdCount++;
            logFile.appendText("✅ Created: " + sample.originalFilename + " (ID: " + juce::String(sampleId) + ")\n");
        }
        else
        {
            logFile.appendText("❌ Failed: " + sample.originalFilename + "\n");
        }
    }
    
    logFile.appendText("Created " + juce::String(createdCount) + " out of " + juce::String(testSamples.size()) + " test samples\n");
    logFile.appendText("==========================\n\n");
    
    if (createdCount > 0)
    {
        // Notify that database has been updated
        sendChangeMessage();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChopsBrowserPluginProcessor();
}
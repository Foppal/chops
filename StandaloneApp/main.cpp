#include <JuceHeader.h>
#include "Database/ChopsDatabase.h"
#include "Database/DatabaseSyncManager.h"
#include "Core/ChordTypes.h"
#include "Core/ChordParser.h"
#include "Core/MetadataService.h"
#include "Core/MetadataServiceTest.h"
#include "Utils/FilenameUtils.h"
#include "Shared/SharedConfig.h"
#include <sqlite3.h>

//==============================================================================
class ChopsLibraryManagerApplication : public juce::JUCEApplication,
                                       public juce::Timer
{
public:
    ChopsLibraryManagerApplication() {}

    const juce::String getApplicationName() override       { return "Chops Library Manager"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        juce::Logger::writeToLog("SQLite version: " + juce::String(sqlite3_libversion()));
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
        
        juce::File dbFile = ChopsConfig::getDatabaseFile();
        juce::File libraryDir = ChopsConfig::getDefaultLibraryDirectory();
        juce::Logger::writeToLog("Library directory: " + libraryDir.getFullPathName());
        juce::Logger::writeToLog("Database file: " + dbFile.getFullPathName());

        if (!libraryDir.exists()) {
            juce::Logger::writeToLog("Creating library directory...");
            if (!libraryDir.createDirectory()) { showErrorAndQuit("Could not create library directory: " + libraryDir.getFullPathName()); return; }
            initializeDirectoryStructure(libraryDir);
        }
        if (!dbFile.existsAsFile()) {
            juce::Logger::writeToLog("Creating new database...");
            if (!createNewDatabase(dbFile)) { showErrorAndQuit("Could not create database file: " + dbFile.getFullPathName()); return; }
        }
        databaseManager = std::make_unique<DatabaseSyncManager>();
        if (!databaseManager->initialize(dbFile)) { showErrorAndQuit("Could not initialize database manager."); return; }
        juce::Logger::writeToLog("Database initialized successfully");
        
        mainWindow = std::make_unique<MainWindow>(getApplicationName(), databaseManager.get(), this);
        
        startTimer(1000);
        juce::Logger::writeToLog("Application initialized successfully");
    }

    void shutdown() override
    {
        juce::Logger::writeToLog("Shutting down application");
        stopTimer();
        mainWindow = nullptr; 
        databaseManager = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    void systemRequestedQuit() override { quit(); }
    void timerCallback() override { checkIPCMessages(); }

private:
    // Nested MainWindow class definition
    class MainWindow : public juce::DocumentWindow,
                       public juce::Button::Listener,
                       public juce::FileDragAndDropTarget,
                       public DatabaseSyncManager::Listener,
                       public juce::TableListBoxModel
    {
    public:
        MainWindow(juce::String name, DatabaseSyncManager* dbManagerPtr, ChopsLibraryManagerApplication* appPtr)
            : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                                  .findColour(juce::ResizableWindow::backgroundColourId),
                            DocumentWindow::allButtons),
              databaseManager(dbManagerPtr),
              app(appPtr)
        {
            juce::Logger::writeToLog("Creating main window");
            setUsingNativeTitleBar(true);
            setContentOwned(createMainComponent(), true);
            #if JUCE_MAC
                juce::MenuBarModel::setMacMainMenu(nullptr);
            #endif
            centreWithSize(1000, 700);  // Made wider to fit new button
            setVisible(true);
            if (databaseManager) {
                databaseManager->addListener(this);
                updateStatistics();
                loadLibraryData();
            }
            juce::Logger::writeToLog("Main window created successfully");
        }

        ~MainWindow() override {
            if (databaseManager) databaseManager->removeListener(this);
            #if JUCE_MAC
                juce::MenuBarModel::setMacMainMenu(nullptr);
            #endif
        }

        void closeButtonPressed() override {
            if (app) app->systemRequestedQuit();
            else if (auto* currentApp = juce::JUCEApplication::getInstance()) currentApp->systemRequestedQuit();
        }

        juce::Component* createMainComponent() {
            auto* mainComp = new juce::Component(); 
            mainComp->setSize(1000, 700);  // Made wider
            
            toolbar = std::make_unique<juce::Component>(); 
            toolbar->setBounds(0, 0, 1000, 60);  // Made wider
            scanButton = std::make_unique<juce::TextButton>("Scan Library"); 
            scanButton->addListener(this); 
            scanButton->setBounds(20, 15, 150, 30); 
            toolbar->addAndMakeVisible(scanButton.get());
            processButton = std::make_unique<juce::TextButton>("Process Uploads"); 
            processButton->addListener(this); 
            processButton->setBounds(180, 15, 150, 30); 
            toolbar->addAndMakeVisible(processButton.get());
            organizeButton = std::make_unique<juce::TextButton>("Organize Files"); 
            organizeButton->addListener(this); 
            organizeButton->setBounds(340, 15, 150, 30); 
            toolbar->addAndMakeVisible(organizeButton.get());
            
            // NEW: Add metadata test button
            testMetadataButton = std::make_unique<juce::TextButton>("Test Metadata");
            testMetadataButton->addListener(this);
            testMetadataButton->setBounds(500, 15, 150, 30);
            toolbar->addAndMakeVisible(testMetadataButton.get());
            
            settingsButton = std::make_unique<juce::TextButton>("Settings"); 
            settingsButton->addListener(this); 
            settingsButton->setBounds(880, 15, 80, 30);  // Moved right
            toolbar->addAndMakeVisible(settingsButton.get());
            mainComp->addAndMakeVisible(toolbar.get());
            
            tabbedComponent = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop); 
            tabbedComponent->setBounds(10, 70, 980, 560);  // Made wider
            overviewTab.reset(createOverviewTab()); 
            tabbedComponent->addTab("Overview", juce::Colours::lightgrey, overviewTab.get(), false);
            libraryTab.reset(createLibraryTab()); 
            tabbedComponent->addTab("Library", juce::Colours::lightgrey, libraryTab.get(), false);
            uploadTab.reset(createUploadTab()); 
            tabbedComponent->addTab("Upload Queue", juce::Colours::lightgrey, uploadTab.get(), false);
            logTab.reset(createLogTab()); 
            tabbedComponent->addTab("Activity Log", juce::Colours::lightgrey, logTab.get(), false);
            mainComp->addAndMakeVisible(tabbedComponent.get());
            
            statusLabel = std::make_unique<juce::Label>("status", "Ready"); 
            statusLabel->setBounds(10, 640, 980, 30);  // Made wider
            statusLabel->setJustificationType(juce::Justification::centredLeft); 
            mainComp->addAndMakeVisible(statusLabel.get());
            
            return mainComp;
        }
        
        juce::Component* createOverviewTab() {
            auto* tab = new juce::Component(); 
            statsDisplay = std::make_unique<juce::TextEditor>(); 
            statsDisplay->setMultiLine(true); 
            statsDisplay->setReadOnly(true); 
            statsDisplay->setFont(juce::FontOptions(14.0f)); 
            statsDisplay->setBounds(20, 20, 400, 400); 
            tab->addAndMakeVisible(statsDisplay.get());
            
            auto* quickActionsLabel = new juce::Label("", "Quick Actions"); 
            quickActionsLabel->setFont(juce::FontOptions(16.0f, juce::Font::bold)); 
            quickActionsLabel->setBounds(450, 20, 200, 30); 
            tab->addAndMakeVisible(quickActionsLabel);
            
            pieChartArea = std::make_unique<juce::Component>(); 
            pieChartArea->setBounds(450, 60, 400, 360); 
            tab->addAndMakeVisible(pieChartArea.get()); 
            return tab;
        }
        
        juce::Component* createLibraryTab() {
            auto* tab = new juce::Component(); 
            searchLabel = std::make_unique<juce::Label>("", "Search:"); 
            searchLabel->setBounds(20, 20, 60, 30); 
            tab->addAndMakeVisible(searchLabel.get());
            searchBox = std::make_unique<juce::TextEditor>(); 
            searchBox->setBounds(85, 20, 300, 30); 
            searchBox->onTextChange = [this] { filterLibraryView(); }; 
            tab->addAndMakeVisible(searchBox.get());
            libraryTable = std::make_unique<juce::TableListBox>("library", this); 
            libraryTable->setBounds(20, 60, 940, 440);  // Made wider
            libraryTable->getHeader().addColumn("Chord", 1, 100); 
            libraryTable->getHeader().addColumn("Filename", 2, 300); 
            libraryTable->getHeader().addColumn("Tags", 3, 150); 
            libraryTable->getHeader().addColumn("Rating", 4, 80); 
            libraryTable->getHeader().addColumn("Plays", 5, 60); 
            libraryTable->getHeader().addColumn("Date Added", 6, 120);
            tab->addAndMakeVisible(libraryTable.get()); 
            return tab;
        }
        
        juce::Component* createUploadTab() {
            auto* tab = new juce::Component(); 
            dropZone = std::make_unique<DropZoneComponent>(); 
            dropZone->setBounds(20, 20, 940, 200);  // Made wider
            tab->addAndMakeVisible(dropZone.get());
            uploadQueueList = std::make_unique<juce::ListBox>("upload queue", nullptr); 
            uploadQueueList->setBounds(20, 240, 940, 260);  // Made wider
            tab->addAndMakeVisible(uploadQueueList.get()); 
            return tab;
        }
        
        juce::Component* createLogTab() {
            auto* tab = new juce::Component(); 
            logView = std::make_unique<juce::TextEditor>(); 
            logView->setMultiLine(true); 
            logView->setReadOnly(true); 
            logView->setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)); 
            logView->setBounds(20, 20, 940, 480);  // Made wider
            tab->addAndMakeVisible(logView.get()); 
            return tab;
        }
        
        void buttonClicked(juce::Button* b) override { 
            if(b==scanButton.get())
                scanLibrary(); 
            else if(b==processButton.get())
                processUploadFolder(); 
            else if(b==organizeButton.get())
                organizeFiles(); 
            else if(b==testMetadataButton.get())
                testMetadataService();  // NEW: Handle metadata test button
            else if(b==settingsButton.get())
                showSettings(); 
        }
        
        bool isInterestedInFileDrag(const juce::StringArray& files) override { 
            for(const auto& f:files) 
                if(ChopsConfig::isAudioFile(juce::File(f))) 
                    return true; 
            return false; 
        }
        
        void filesDropped(const juce::StringArray& files, int x, int y) override { 
            juce::ignoreUnused(x,y); 
            juce::File ud=ChopsConfig::getDefaultLibraryDirectory().getChildFile(ChopsConfig::FolderNames::chopsRoot).getChildFile(ChopsConfig::FolderNames::uploadFolder);
            if(!ud.isDirectory()&&!ud.createDirectory()){
                // Only log errors
                addLogMessage("ERROR: Cannot create upload dir: "+ud.getFullPathName()); 
                statusLabel->setText("Error upload dir", juce::dontSendNotification); 
                return;
            }
            
            int ac=0; 
            for(const auto& fp:files){
                juce::File f(fp); 
                if(ChopsConfig::isAudioFile(f)){
                    juce::File d=ud.getChildFile(f.getFileName()); 
                    int dc=0; 
                    juce::String oN=d.getFileNameWithoutExtension(), eX=d.getFileExtension(); 
                    while(d.existsAsFile()){
                        dc++; 
                        d=ud.getChildFile(oN+"_"+juce::String(dc)+eX);
                    } 
                    if(f.copyFileTo(d)){
                        ac++;
                        // Silent - no individual file logging during interval debugging
                    }else{
                        // Only log errors
                        addLogMessage("ERROR copying: "+f.getFileName());
                    }
                }
            }
            
            // Only show summary in status bar, not in log
            statusLabel->setText("Added "+juce::String(ac)+" files",juce::dontSendNotification); 
            refreshUploadQueue();
        }
        
        void databaseUpdated() override { 
            // Silent - no logging during interval debugging
            updateStatistics(); 
            loadLibraryData(); 
        }
        
        void sampleMetadataChanged(int id) override { 
            juce::ignoreUnused(id); 
            // Silent - no logging during interval debugging
            loadLibraryData(); 
        }
        
        int getNumRows() override { return (int)currentSamples.size(); }
        
        void paintRowBackground(juce::Graphics& g, int rN, int /*w*/, int /*h*/, bool sel) override {
            if(sel)
                g.fillAll(getLookAndFeel().findColour(juce::ListBox::backgroundColourId).interpolatedWith(juce::Colours::black,0.5f)); 
            else if(rN%2)
                g.fillAll(getLookAndFeel().findColour(juce::ListBox::backgroundColourId).interpolatedWith(juce::Colours::white,0.1f)); 
            else 
                g.fillAll(getLookAndFeel().findColour(juce::ListBox::backgroundColourId));
        }
        
        void paintCell(juce::Graphics&g, int rN, int cId, int w, int h, bool sel) override {
            juce::ignoreUnused(sel); 
            if(rN<0||(size_t)rN>=currentSamples.size())
                return; 
            const auto&s=currentSamples[(size_t)rN];
            g.setColour(getLookAndFeel().findColour(juce::Label::textColourId)); 
            g.setFont(juce::FontOptions(14.0f)); 
            juce::String t;
            switch(cId){
                case 1:t=s.getFullChordName();break; 
                case 2:t=s.currentFilename;break; 
                case 3:t=s.tags.joinIntoString(", ");break; 
                case 4:t=juce::String(s.rating)+"/5";break; 
                case 5:t=juce::String(s.playCount);break; 
                case 6:t=s.dateAdded.toString(true,true,false,true);break;
            }
            g.drawText(t,2,0,w-4,h,juce::Justification::centredLeft,true);
        }
        
    private:
        DatabaseSyncManager* databaseManager; 
        ChopsLibraryManagerApplication* app; 
        
        class DropZoneComponent : public juce::Component { 
        public: 
            void paint(juce::Graphics& g) override { 
                g.fillAll(getLookAndFeel().findColour(juce::TooltipWindow::backgroundColourId).withAlpha(0.3f)); 
                g.setColour(getLookAndFeel().findColour(juce::Label::textColourId)); 
                g.drawRect(getLocalBounds(),2); 
                g.setFont(juce::FontOptions(16.0f)); 
                g.drawText("Drop files here", getLocalBounds(), juce::Justification::centred); 
            }
        };
        
        std::unique_ptr<juce::Component> toolbar; 
        std::unique_ptr<juce::TextButton> scanButton, processButton, organizeButton, settingsButton;
        std::unique_ptr<juce::TextButton> testMetadataButton;  // NEW: Metadata test button
        std::unique_ptr<juce::TabbedComponent> tabbedComponent; 
        std::unique_ptr<juce::Component> overviewTab, libraryTab, uploadTab, logTab;
        std::unique_ptr<juce::TextEditor> statsDisplay; 
        std::unique_ptr<juce::Component> pieChartArea;
        std::unique_ptr<juce::Label> searchLabel; 
        std::unique_ptr<juce::TextEditor> searchBox;
        std::unique_ptr<juce::TableListBox> libraryTable; 
        std::unique_ptr<DropZoneComponent> dropZone;
        std::unique_ptr<juce::ListBox> uploadQueueList; 
        std::unique_ptr<juce::TextEditor> logView;
        std::unique_ptr<juce::Label> statusLabel; 
        std::vector<ChopsDatabase::SampleInfo> currentSamples;
        juce::StringArray uploadQueueDisplayItems;

        void scanLibrary(){
            // Silent - no logging during interval debugging
            statusLabel->setText("Scanning...", juce::dontSendNotification); 
            if(databaseManager) 
                databaseManager->notifyListenersDatabaseUpdated(); 
            updateStatistics();
        }
        
        // NEW: Metadata service test method
        void testMetadataService()
        {
            addLogMessage("=== TESTING METADATA SERVICE ===");
            statusLabel->setText("Running metadata tests...", juce::dontSendNotification);
            
            // Create test directory in the library
            juce::File testDir = ChopsConfig::getDefaultLibraryDirectory().getChildFile("metadata_tests");
            
            if (!testDir.isDirectory() && !testDir.createDirectory())
            {
                addLogMessage("❌ ERROR: Could not create test directory");
                statusLabel->setText("Test failed - no directory", juce::dontSendNotification);
                return;
            }
            
            // Run the tests
            MetadataServiceTest tester;
            bool allPassed = tester.runAllTests(testDir);
            
            if (allPassed)
            {
                addLogMessage("✅ ALL METADATA TESTS PASSED!");
                statusLabel->setText("Metadata tests: ALL PASSED", juce::dontSendNotification);
            }
            else
            {
                addLogMessage("❌ SOME METADATA TESTS FAILED!");
                statusLabel->setText("Metadata tests: SOME FAILED", juce::dontSendNotification);
            }
            
            addLogMessage("=== METADATA TESTS COMPLETE ===");
            addLogMessage("Check the log above for detailed results.");
            addLogMessage("Test files created in: " + testDir.getFullPathName());
        }
        
        void processUploadFolder() 
        {
            addLogMessage("=== PROCESSING SESSION STARTED (INTERVAL DEBUGGING) ===");
            statusLabel->setText("Starting processing...", juce::dontSendNotification);
            
            if (!databaseManager) {
                addLogMessage("ERROR: DatabaseManager not available");
                statusLabel->setText("Error: DBManager N/A", juce::dontSendNotification);
                return;
            }
            
            juce::File libRoot = ChopsConfig::getDefaultLibraryDirectory();
            juce::File cRoot = libRoot.getChildFile(ChopsConfig::FolderNames::chopsRoot);
            juce::File upDir = cRoot.getChildFile(ChopsConfig::FolderNames::uploadFolder);
            juce::File procDir = cRoot.getChildFile(ChopsConfig::FolderNames::processedFolder);
            juce::File misDir = cRoot.getChildFile(ChopsConfig::FolderNames::mismatchFolder);
            
            // Ensure directories exist
            for (auto& d : {procDir, misDir}) {
                if (!d.isDirectory() && !d.createDirectory()) {
                    addLogMessage("ERROR: Could not create directory: " + d.getFullPathName());
                    statusLabel->setText("Error creating directories", juce::dontSendNotification);
                    return;
                }
            }
            
            if (!upDir.isDirectory()) {
                addLogMessage("ERROR: Upload directory missing: " + upDir.getFullPathName());
                statusLabel->setText("Upload dir missing", juce::dontSendNotification);
                return;
            }
            
            ChordParser parser;
            juce::Array<juce::File> files;
            upDir.findChildFiles(files, juce::File::findFiles, false, "*");
            
            // Filter for audio files only
            juce::Array<juce::File> audioFiles;
            for (const auto& f : files) {
                if (ChopsConfig::isAudioFile(f)) {
                    audioFiles.add(f);
                }
            }
            
            addLogMessage("Found " + juce::String(audioFiles.size()) + " audio files to process");
            addLogMessage("=== INTERVAL DETECTION ANALYSIS ===");
            
            if (audioFiles.isEmpty()) {
                addLogMessage("No audio files in upload directory.");
                statusLabel->setText("Upload empty.", juce::dontSendNotification);
                return;
            }
            
            int ok = 0, errCount = 0, intervalCount = 0;
            bool dbChangedByThisRun = false;
            
            for (int i = 0; i < audioFiles.size(); ++i) {
                const auto& f = audioFiles[i];
                
                // Update progress with percentage and current file
                updateProcessingProgress(i + 1, audioFiles.size(), f.getFileName());
                
                // Parse the filename
                ChordParser::ParsedData pd = parser.parseFilename(f.getFileName());
                
                // === INTERVAL DETECTION LOGGING ONLY ===
                
                // Log files interpreted as intervals
                if (pd.standardizedQuality.startsWith("interval_")) {
                    intervalCount++;
                    addLogMessage("🎵 INTERVAL DETECTED #" + juce::String(intervalCount) + ": " + f.getFileName());
                    addLogMessage("  → Root: '" + pd.rootNote + "' | Quality: '" + pd.standardizedQuality + "'");
                    addLogMessage("  → Full chord name: '" + pd.getFullChordName() + "'");
                    addLogMessage("  → Cleaned basename: '" + pd.cleanedBasename + "'");
                    addLogMessage("  → Descriptor part: '" + pd.qualityDescriptorString + "'");
                    addLogMessage("  → Chord notation part: '" + pd.specificChordNotationFull + "'");
                    addLogMessage("  → Inversion text: '" + pd.inversionText + "'");
                    
                    if (!pd.issues.isEmpty()) {
                        addLogMessage("  → Issues: " + pd.issues.joinIntoString(" | "));
                    }
                    addLogMessage("  ────────────────────────────────");
                }
                
                // Log power chord detection (5_ files) - might be incorrectly parsed as intervals
                if (f.getFileName().startsWith("5_") || f.getFileName().startsWith("5 ")) {
                    if (pd.standardizedQuality.startsWith("interval_")) {
                        addLogMessage("⚡ POWER CHORD → INTERVAL: " + f.getFileName());
                        addLogMessage("  → Parsed as: " + pd.rootNote + " " + pd.standardizedQuality);
                        addLogMessage("  ────────────────────────────────");
                    }
                }
                
                // Log files that have "interval" in the name but aren't parsed as intervals
                if (f.getFileName().toLowerCase().contains("interval") && !pd.standardizedQuality.startsWith("interval_")) {
                    addLogMessage("❓ HAS 'INTERVAL' BUT NOT PARSED AS INTERVAL: " + f.getFileName());
                    addLogMessage("  → Actually parsed as: " + pd.rootNote + " " + pd.standardizedQuality);
                    addLogMessage("  ────────────────────────────────");
                }
                
                // === SILENT PROCESSING (no regular logging) ===
                
                // Check if parsing was successful
                if (!FilenameUtils::isValidParsedData(pd)) {
                    // Move to mismatch folder (silently)
                    misDir.createDirectory();
                    juce::File destMismatch = createUniqueDestination(misDir, f.getFileName());
                    f.moveFileTo(destMismatch);
                    errCount++;
                    continue;
                }
                
                // Create sample info
                ChopsDatabase::SampleInfo si;
                si.originalFilename = f.getFileName();
                si.fileSize = f.getSize();
                si.rootNote = pd.rootNote;
                si.chordType = pd.standardizedQuality;
                si.chordTypeDisplay = pd.getFullChordName();
                si.extensions = pd.extensions;
                si.alterations = pd.alterations;
                si.addedNotes = pd.addedNotes;
                si.suspensions = pd.suspensions;
                si.bassNote = pd.determinedBassNote;
                si.inversion = pd.inversionTextParsed;
                
                // Determine destination folder
                juce::String cFKey = pd.standardizedQuality.isNotEmpty() ? pd.standardizedQuality : "unknown";
                juce::String cSubName = ChordTypes::sanitizeChordFolderName(cFKey);
                juce::File destCFolder = procDir.getChildFile(cSubName);
                
                if (!destCFolder.isDirectory() && !destCFolder.createDirectory()) {
                    juce::File destMismatch = createUniqueDestination(misDir, f.getFileName());
                    f.moveFileTo(destMismatch);
                    errCount++;
                    continue;
                }
                
                // Generate new filename
                juce::String newFilenameStr = FilenameUtils::generateNewSampleFilename(pd, f.getFileExtension());
                
                if (newFilenameStr.startsWith("parse_failed_")) {
                    juce::File destMismatch = createUniqueDestination(misDir, f.getFileName());
                    f.moveFileTo(destMismatch);
                    errCount++;
                    continue;
                }
                
                // Create unique destination path
                juce::File destF = createUniqueDestination(destCFolder, newFilenameStr);
                newFilenameStr = destF.getFileName();
                
                // Move file and add to database (silently)
                if (f.moveFileTo(destF)) {
                    si.filePath = destF.getFullPathName();
                    si.currentFilename = newFilenameStr;
                    
                    int newId = databaseManager->insertProcessedSample(si);
                    
                    if (newId > 0) {
                        ok++;
                        dbChangedByThisRun = true;
                    } else {
                        errCount++;
                    }
                } else {
                    errCount++;
                }
                
                // Allow UI updates periodically to prevent freezing
                if (i % 5 == 0) {
                    juce::MessageManager::callAsync([]() {
                        // Empty lambda to trigger message queue processing
                    });
                }
            }
            
            addLogMessage("=== INTERVAL DETECTION SUMMARY ===");
            addLogMessage("📊 Files interpreted as INTERVALS: " + juce::String(intervalCount));
            addLogMessage("✓ Total processed successfully: " + juce::String(ok));
            addLogMessage("✗ Total failed: " + juce::String(errCount));
            if (ok + errCount > 0) {
                addLogMessage("📈 Success rate: " + juce::String((ok * 100) / (ok + errCount)) + "%");
            }
            
            if (intervalCount == 0) {
                addLogMessage("🎉 NO FILES INCORRECTLY INTERPRETED AS INTERVALS!");
            } else {
                addLogMessage("⚠️  " + juce::String(intervalCount) + " files were interpreted as intervals - check details above");
            }
            
            statusLabel->setText("Complete: " + juce::String(ok) + " success, " + juce::String(errCount) + " errors, " + juce::String(intervalCount) + " intervals", 
                                juce::dontSendNotification);
            
            refreshUploadQueue();
            
            if (dbChangedByThisRun && databaseManager != nullptr) {
                databaseManager->notifyListenersDatabaseUpdated();
            }
            
            addLogMessage("=== PROCESSING SESSION FINISHED ===");
        }

        void updateProcessingProgress(int current, int total, const juce::String& currentFile) {
            int percentage = (current * 100) / total;
            
            // Truncate filename if too long for status bar
            juce::String displayName = currentFile;
            if (displayName.length() > 30) {
                displayName = displayName.substring(0, 27) + "...";
            }
            
            juce::String progressText = juce::String::formatted("Processing %d/%d (%d%%): %s", 
                                                              current, total, percentage, 
                                                              displayName.toRawUTF8());
            statusLabel->setText(progressText, juce::dontSendNotification);
        }
        
        juce::File createUniqueDestination(const juce::File& directory, const juce::String& desiredName) {
            juce::File proposedFile = directory.getChildFile(desiredName);
            
            if (!proposedFile.exists()) {
                return proposedFile;
            }
            
            juce::String baseName = proposedFile.getFileNameWithoutExtension();
            juce::String extension = proposedFile.getFileExtension();
            
            int counter = 1;
            do {
                counter++;
                juce::String newName = baseName + "_" + juce::String(counter) + extension;
                proposedFile = directory.getChildFile(newName);
            } while (proposedFile.exists() && counter < 1000);
            
            return proposedFile;
        }
        
        void organizeFiles() { 
            // Silent - no logging during interval debugging
            statusLabel->setText("Organize not implemented", juce::dontSendNotification);
        } 
        
        void showSettings() { 
            // Silent - no logging during interval debugging
            statusLabel->setText("Settings not implemented", juce::dontSendNotification);
        }
        
        void updateStatistics() { 
            if(!databaseManager||!databaseManager->getReadDatabase()){
                if(statsDisplay)
                    statsDisplay->setText("DB N/A");
                return;
            } 
            auto st=databaseManager->getReadDatabase()->getStatistics(); 
            juce::String s;
            s<<"Total: "<<st.totalSamples<<"\nExt: "<<st.withExtensions<<"\nAlt: "<<st.withAlterations<<"\nWeek: "<<st.addedLastWeek<<"\n\nChord Types:\n"; 
            for(int i=0;i<juce::jmin(10,(int)st.byChordType.size());++i)
                s<<st.byChordType[(size_t)i].first << ": " << st.byChordType[(size_t)i].second<<"\n"; 
            if(statsDisplay)
                statsDisplay->setText(s); 
        }
        
        void loadLibraryData() { 
            if(!databaseManager||!databaseManager->getReadDatabase())
                return; 
            currentSamples=databaseManager->getReadDatabase()->searchSamples(); 
            if(libraryTable)
                libraryTable->updateContent(); 
        }
        
        void filterLibraryView() { 
            if(!databaseManager||!databaseManager->getReadDatabase())
                return; 
            juce::String q=searchBox?searchBox->getText():juce::String(); 
            currentSamples=databaseManager->getReadDatabase()->searchSamples(q); 
            if(libraryTable)
                libraryTable->updateContent(); 
        }
        
        void refreshUploadQueue() { 
            uploadQueueDisplayItems.clear(); 
            juce::File ud=ChopsConfig::getDefaultLibraryDirectory().getChildFile(ChopsConfig::FolderNames::chopsRoot).getChildFile(ChopsConfig::FolderNames::uploadFolder); 
            if(ud.isDirectory()){
                juce::Array<juce::File>fs;
                ud.findChildFiles(fs,juce::File::findFiles,false);
                for(const auto&f:fs)
                    if(ChopsConfig::isAudioFile(f))
                        uploadQueueDisplayItems.add(f.getFileName());
            } 
            if(uploadQueueList)
                uploadQueueList->updateContent(); 
            // Silent - no logging during interval debugging
        }
        
        void addLogMessage(const juce::String& msg) { 
            juce::String ts=juce::Time::getCurrentTime().toString(true,true,true,true); 
            if(logView){
                logView->moveCaretToEnd();
                logView->insertTextAtCaret("["+ts+"] "+msg+juce::newLine);
            } 
            juce::Logger::writeToLog(msg); 
        }
    }; // End of nested MainWindow class

    // --- ChopsLibraryManagerApplication private members ---
    std::unique_ptr<DatabaseSyncManager> databaseManager;
    juce::LookAndFeel_V4 lookAndFeel;
    std::unique_ptr<MainWindow> mainWindow; 

    // --- ChopsLibraryManagerApplication private helper method definitions ---
    void showErrorAndQuit(const juce::String& message)
    {
        juce::Logger::writeToLog("FATAL ERROR: " + message);
        juce::MessageManager::callAsync([=]() { 
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Fatal Error", message, "OK", nullptr,
                juce::ModalCallbackFunction::create([](int) { 
                    if (auto* app = juce::JUCEApplication::getInstance())
                        app->quit(); 
                })
            );
        });
    }

    bool createNewDatabase(const juce::File& dbFile)
    {
        sqlite3* tempDb = nullptr; 
        int rc = sqlite3_open(dbFile.getFullPathName().toUTF8(), &tempDb);
        if (rc != SQLITE_OK) { 
            juce::Logger::writeToLog("Failed to create database: " + juce::String(sqlite3_errmsg(tempDb))); 
            if (tempDb) sqlite3_close(tempDb); 
            return false; 
        }
        
        juce::File schemaFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentApplicationFile)
                                    .getParentDirectory().getChildFile("schema.sql");
        if (!schemaFile.existsAsFile()) 
            schemaFile = juce::File::getCurrentWorkingDirectory().getChildFile("Source/Database/schema.sql");
        if (!schemaFile.existsAsFile()) 
            schemaFile = juce::File::getCurrentWorkingDirectory().getChildFile("../Source/Database/schema.sql");
        if (!schemaFile.existsAsFile()) 
            schemaFile = juce::File::getCurrentWorkingDirectory().getChildFile("schema.sql");

        if (schemaFile.existsAsFile()) {
            juce::String schemaSql = schemaFile.loadFileAsString(); 
            char* errMsg = nullptr; 
            rc = sqlite3_exec(tempDb, schemaSql.toUTF8(), nullptr, nullptr, &errMsg);
            if (rc != SQLITE_OK) { 
                juce::Logger::writeToLog("Failed to execute schema: " + juce::String(errMsg)); 
                sqlite3_free(errMsg); 
                sqlite3_close(tempDb); 
                return false; 
            }
        } else {
            juce::Logger::writeToLog("schema.sql not found (final path checked: " + schemaFile.getFullPathName() + "), creating basic schema.");
            const char* basicSchema = "CREATE TABLE IF NOT EXISTS samples (id INTEGER PRIMARY KEY AUTOINCREMENT, original_filename TEXT NOT NULL, current_filename TEXT NOT NULL, file_path TEXT NOT NULL UNIQUE, file_size INTEGER, root_note TEXT, chord_type TEXT, chord_type_display TEXT, extensions TEXT DEFAULT '[]', alterations TEXT DEFAULT '[]', added_notes TEXT DEFAULT '[]', suspensions TEXT DEFAULT '[]', bass_note TEXT, inversion TEXT, date_added TIMESTAMP DEFAULT CURRENT_TIMESTAMP, date_modified TIMESTAMP DEFAULT CURRENT_TIMESTAMP, search_text TEXT, rating INTEGER DEFAULT 0, color_hex TEXT, is_favorite INTEGER DEFAULT 0, play_count INTEGER DEFAULT 0, user_notes TEXT, last_played TIMESTAMP); CREATE TABLE IF NOT EXISTS tags (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE); CREATE TABLE IF NOT EXISTS sample_tags (sample_id INTEGER NOT NULL, tag_id INTEGER NOT NULL, PRIMARY KEY (sample_id, tag_id), FOREIGN KEY (sample_id) REFERENCES samples(id) ON DELETE CASCADE, FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE);";
            char* errMsg = nullptr; 
            rc = sqlite3_exec(tempDb, basicSchema, nullptr, nullptr, &errMsg);
            if (rc != SQLITE_OK) { 
                juce::Logger::writeToLog("Failed to create basic schema: " + juce::String(errMsg)); 
                sqlite3_free(errMsg); 
                sqlite3_close(tempDb); 
                return false; 
            }
        }
        sqlite3_close(tempDb); 
        juce::Logger::writeToLog("Database operations completed for: " + dbFile.getFullPathName()); 
        return true;
    }

    void initializeDirectoryStructure(const juce::File& libraryDir) 
    {
        juce::Logger::writeToLog("Initializing directory structure for: " + libraryDir.getFullPathName());
        juce::File chopsRoot = libraryDir.getChildFile(ChopsConfig::FolderNames::chopsRoot);
        if (!chopsRoot.isDirectory() && !chopsRoot.createDirectory()) { 
            juce::Logger::writeToLog("Failed to create chopsRoot: " + chopsRoot.getFullPathName()); 
            return; 
        }
        
        if (!chopsRoot.getChildFile(ChopsConfig::FolderNames::uploadFolder).createDirectory()) 
            juce::Logger::writeToLog("Failed to create uploadFolder");
        juce::File processedDir = chopsRoot.getChildFile(ChopsConfig::FolderNames::processedFolder);
        if (!processedDir.createDirectory()) 
            juce::Logger::writeToLog("Failed to create processedFolder");
        if (!chopsRoot.getChildFile(ChopsConfig::FolderNames::mismatchFolder).createDirectory()) 
            juce::Logger::writeToLog("Failed to create mismatchFolder");
        
        using namespace ChordTypes; 
        auto chordTypesMap = getStandardizedChordTypes();
        for (const auto& pair : chordTypesMap) {
            juce::String folderName = sanitizeChordFolderName(pair.first);
            if (folderName.isNotEmpty() && folderName != "unknown_chord") {
                if (!processedDir.getChildFile(folderName).createDirectory()) {
                    juce::Logger::writeToLog("Failed to create chord subfolder: " + folderName);
                }
            }
        }
        juce::Logger::writeToLog("Directory structure initialized");
    }

    void checkIPCMessages() { /* Placeholder */ }

}; // End of ChopsLibraryManagerApplication class

//==============================================================================
START_JUCE_APPLICATION (ChopsLibraryManagerApplication)
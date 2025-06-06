#include "UIBridge.h"
#include "../Source/Shared/SharedConfig.h"

//==============================================================================
// CustomWebBrowserComponent Implementation
//==============================================================================

CustomWebBrowserComponent::CustomWebBrowserComponent(UIBridge* bridge)
    : juce::WebBrowserComponent(), uiBridge(bridge)
{
}

bool CustomWebBrowserComponent::pageAboutToLoad(const juce::String& newURL)
{
    if (uiBridge)
        return uiBridge->pageAboutToLoad(newURL);
    return true;
}

void CustomWebBrowserComponent::pageFinishedLoading(const juce::String& url)
{
    if (uiBridge)
        uiBridge->pageFinishedLoading(url);
}

void CustomWebBrowserComponent::handleJavaScriptCall(const juce::String& functionName, 
                                                     const juce::StringArray& parameters)
{
    if (uiBridge) {
        uiBridge->handleJavaScriptCall(functionName, parameters);
    }
}

//==============================================================================
// UIBridge Implementation
//==============================================================================

UIBridge::UIBridge()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== UIBridge Constructor Started ===\n");
    logFile.appendText("Time: " + juce::Time::getCurrentTime().toString(true, true, true, true) + "\n");
    
    juce::Logger::writeToLog("UIBridge: Constructor started");
    
    try {
        if (!juce::WebBrowserComponent::areOptionsSupported(juce::WebBrowserComponent::Options()))
        {
            logFile.appendText("ERROR: WebBrowserComponent not supported on this platform!\n");
            initializationFailed = true;
            return;
        }
        
        webBrowser = std::make_unique<CustomWebBrowserComponent>(this);
        
        if (webBrowser == nullptr) {
            logFile.appendText("ERROR: Failed to create CustomWebBrowserComponent!\n");
            initializationFailed = true;
            return;
        }
        
        addAndMakeVisible(webBrowser.get());
        setVisible(true);
        setOpaque(true);
        setSize(400, 300);
        
        // Load UI immediately with better error handling
        juce::Timer::callAfterDelay(100, [this, logFile]() {
            logFile.appendText("Timer callback: Loading React UI...\n");
            loadReactUIContent();
        });
        
    } catch (const std::exception& e) {
        juce::String error = "Exception in constructor: " + juce::String(e.what());
        logFile.appendText("EXCEPTION: " + error + "\n");
        initializationFailed = true;
    }
    
    logFile.appendText("UIBridge constructor completed\n");
}

UIBridge::~UIBridge()
{
    if (webBrowser) {
        webBrowser.reset();
    }
}

//==============================================================================
void UIBridge::loadReactUIContent()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    if (initializationFailed || !webBrowser) {
        logFile.appendText("loadReactUIContent: Cannot load - initialization failed\n");
        return;
    }
    
    logFile.appendText("=== LOADING REACT UI CONTENT ===\n");
    
    // Try to find the built React UI first
    juce::File reactUIFile = getUIHtmlFile();
    
    if (reactUIFile.existsAsFile()) {
        logFile.appendText("✅ Found React UI file: " + reactUIFile.getFullPathName() + "\n");
        loadUI(reactUIFile);
    } else {
        logFile.appendText("⚠️ React UI file not found, using enhanced test HTML\n");
        loadEnhancedTestHTML();
    }
    
    logFile.appendText("================================\n\n");
}

void UIBridge::loadEnhancedTestHTML()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    if (initializationFailed || !webBrowser) {
        return;
    }
    
    logFile.appendText("=== LOADING ENHANCED TEST HTML ===\n");
    
    // Create enhanced HTML with MUCH better bridge communication
    juce::String enhancedHTML = 
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "    <meta charset=\"UTF-8\">"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "    <title>Chops Browser - Enhanced Communication Test</title>"
        "    <style>"
        "        body { margin: 0; padding: 20px; background: #1a1a1a; color: #e0e0e0; font-family: Arial, sans-serif; }"
        "        .container { max-width: 800px; margin: 0 auto; }"
        "        h1 { color: #4CAF50; text-align: center; margin-bottom: 30px; }"
        "        .status { background: #333; padding: 15px; border-radius: 5px; margin: 10px 0; border-left: 4px solid #4CAF50; }"
        "        .debug { background: #2a2a2a; padding: 10px; border-radius: 3px; font-family: monospace; font-size: 12px; margin: 5px 0; max-height: 200px; overflow-y: auto; }"
        "        .test-btn { background: #4CAF50; color: white; border: none; padding: 12px 24px; margin: 8px; border-radius: 5px; cursor: pointer; }"
        "        .test-btn:hover { background: #45a049; }"
        "        .search-box { padding: 10px; background: #333; border: 1px solid #555; border-radius: 4px; color: white; width: 200px; margin: 10px; }"
        "        .results { background: #2a2a2a; padding: 15px; border-radius: 5px; margin: 10px 0; max-height: 300px; overflow-y: auto; }"
        "        .sample-item { padding: 8px; margin: 5px 0; background: #333; border-radius: 3px; cursor: pointer; }"
        "        .sample-item:hover { background: #444; }"
        "        .communication-log { background: #1a1a2e; padding: 10px; border-radius: 3px; margin: 10px 0; max-height: 150px; overflow-y: auto; }"
        "    </style>"
        "</head>"
        "<body>"
        "    <div class=\"container\">"
        "        <h1>🎵 Chops Browser - Communication Test</h1>"
        "        <div class=\"status\" id=\"status\">Initializing enhanced bridge...</div>"
        "        <div class=\"debug\" id=\"debug\">Debug log will appear here...</div>"
        "        "
        "        <div style=\"margin: 20px 0; text-align: center;\">"
        "            <button class=\"test-btn\" onclick=\"testBridge()\">Test Bridge</button>"
        "            <button class=\"test-btn\" onclick=\"testSearch()\">Test Search (C)</button>"
        "            <input type=\"text\" class=\"search-box\" placeholder=\"Enter chord...\" id=\"searchInput\" onkeypress=\"handleSearchKeyPress(event)\">"
        "            <button class=\"test-btn\" onclick=\"performCustomSearch()\">Custom Search</button>"
        "            <button class=\"test-btn\" onclick=\"clearLogs()\">Clear Logs</button>"
        "        </div>"
        "        "
        "        <div class=\"communication-log\" id=\"commLog\">"
        "            <h4>Communication Log:</h4>"
        "            <div id=\"commLogContent\">Waiting for messages...</div>"
        "        </div>"
        "        "
        "        <div class=\"results\" id=\"results\" style=\"display: none;\">"
        "            <h3>Search Results:</h3>"
        "            <div id=\"resultsList\"></div>"
        "        </div>"
        "    </div>"
        "    "
        "    <script>"
        "        console.log('=== ENHANCED COMMUNICATION TEST PAGE LOADED ===');"
        "        "
        "        let debugLog = [];"
        "        let commLog = [];"
        "        let bridgeReady = false;"
        "        let messageCounter = 0;"
        "        "
        "        function addDebugLog(msg) {"
        "            const timestamp = new Date().toLocaleTimeString();"
        "            const logEntry = timestamp + ': ' + msg;"
        "            debugLog.push(logEntry);"
        "            if (debugLog.length > 20) debugLog.shift();"
        "            document.getElementById('debug').innerHTML = debugLog.join('<br>');"
        "            console.log('DEBUG:', msg);"
        "        }"
        "        "
        "        function addCommLog(msg) {"
        "            const timestamp = new Date().toLocaleTimeString();"
        "            const logEntry = timestamp + ': ' + msg;"
        "            commLog.push(logEntry);"
        "            if (commLog.length > 15) commLog.shift();"
        "            document.getElementById('commLogContent').innerHTML = commLog.join('<br>');"
        "        }"
        "        "
        "        function updateStatus(msg, isError = false) {"
        "            const statusEl = document.getElementById('status');"
        "            statusEl.textContent = msg;"
        "            statusEl.style.borderLeftColor = isError ? '#F44336' : '#4CAF50';"
        "            addDebugLog('STATUS: ' + msg);"
        "        }"
        "        "
        "        function clearLogs() {"
        "            debugLog = [];"
        "            commLog = [];"
        "            document.getElementById('debug').innerHTML = 'Debug log cleared';"
        "            document.getElementById('commLogContent').innerHTML = 'Communication log cleared';"
        "        }"
        "        "
        "        // ENHANCED: Multi-method communication with better error handling"
        "        window.ChopsBridge = {"
        "            sendMessage: function(type, data) {"
        "                messageCounter++;"
        "                const messageId = 'msg_' + messageCounter;"
        "                addCommLog('SENDING[' + messageId + ']: ' + type);"
        "                addDebugLog('Sending message: ' + type + ' (ID: ' + messageId + ')');"
        "                "
        "                try {"
        "                    const message = { "
        "                        id: messageId,"
        "                        type: type, "
        "                        data: data, "
        "                        timestamp: Date.now(),"
        "                        url: window.location.href"
        "                    };"
        "                    const messageStr = JSON.stringify(message);"
        "                    "
        "                    addDebugLog('Message JSON length: ' + messageStr.length);"
        "                    "
        "                    // Method 1: URL scheme (primary for JUCE)"
        "                    addDebugLog('Attempting URL scheme...');"
        "                    const encodedMessage = encodeURIComponent(messageStr);"
        "                    const schemeUrl = 'chops://message/' + encodedMessage;"
        "                    "
        "                    addDebugLog('Scheme URL length: ' + schemeUrl.length);"
        "                    addCommLog('URL SCHEME[' + messageId + ']: ' + schemeUrl.substring(0, 100) + '...');"
        "                    "
        "                    // This should trigger C++ pageAboutToLoad"
        "                    window.location.href = schemeUrl;"
        "                    "
        "                    // Method 2: Global variable backup"
        "                    window.lastChopsBridgeMessage = message;"
        "                    "
        "                    // Method 3: Console markers for C++ to intercept"
        "                    console.log('=== CHOPS_MESSAGE_START[' + messageId + '] ===');"
        "                    console.log(messageStr);"
        "                    console.log('=== CHOPS_MESSAGE_END[' + messageId + '] ===');"
        "                    "
        "                    addDebugLog('All communication methods attempted for: ' + messageId);"
        "                    return 'SUCCESS_' + messageId;"
        "                    "
        "                } catch (e) {"
        "                    addDebugLog('ERROR sending message: ' + e.message);"
        "                    addCommLog('ERROR[' + messageId + ']: ' + e.message);"
        "                    console.error('Bridge send error:', e);"
        "                    return 'ERROR_' + messageId;"
        "                }"
        "            },"
        "            "
        "            callbacks: {},"
        "            "
        "            setCallback: function(name, callback) {"
        "                addDebugLog('Setting callback: ' + name);"
        "                this.callbacks[name] = callback;"
        "            }"
        "        };"
        "        "
        "        // Set up all the callbacks React would use"
        "        window.ChopsBridge.setCallback('onSampleResults', function(samples) {"
        "            addDebugLog('RECEIVED ' + samples.length + ' samples from C++');"
        "            addCommLog('RECEIVED: Sample results (' + samples.length + ' items)');"
        "            displayResults(samples);"
        "        });"
        "        "
        "        window.ChopsBridge.setCallback('onLoadingState', function(loading) {"
        "            updateStatus(loading ? 'Loading from C++...' : 'Ready');"
        "            addCommLog('RECEIVED: Loading state = ' + loading);"
        "        });"
        "        "
        "        window.ChopsBridge.setCallback('onErrorMessage', function(error) {"
        "            updateStatus('C++ Error: ' + error, true);"
        "            addCommLog('RECEIVED: Error = ' + error);"
        "        });"
        "        "
        "        window.ChopsBridge.setCallback('onDatabaseStats', function(stats) {"
        "            addDebugLog('RECEIVED database stats: ' + stats.totalSamples + ' total samples');"
        "            addCommLog('RECEIVED: DB stats (' + stats.totalSamples + ' samples)');"
        "        });"
        "        "
        "        function testBridge() {"
        "            addDebugLog('=== TESTING BRIDGE COMMUNICATION ===');"
        "            updateStatus('Testing bridge...');"
        "            "
        "            const testData = {"
        "                test: true,"
        "                timestamp: Date.now(),"
        "                userAgent: navigator.userAgent,"
        "                location: window.location.href,"
        "                random: Math.random()"
        "            };"
        "            "
        "            const result = window.ChopsBridge.sendMessage('bridgeTest', testData);"
        "            updateStatus('Bridge test sent, result: ' + result);"
        "            addCommLog('BRIDGE TEST sent with result: ' + result);"
        "        }"
        "        "
        "        function testSearch() {"
        "            performSearch('C');"
        "        }"
        "        "
        "        function performCustomSearch() {"
        "            const input = document.getElementById('searchInput');"
        "            const query = input.value.trim() || 'Cmaj7';"
        "            performSearch(query);"
        "        }"
        "        "
        "        function handleSearchKeyPress(event) {"
        "            if (event.key === 'Enter') {"
        "                performCustomSearch();"
        "            }"
        "        }"
        "        "
        "        function performSearch(query) {"
        "            addDebugLog('=== PERFORMING SEARCH: ' + query + ' ===');"
        "            updateStatus('Searching for: ' + query);"
        "            "
        "            const searchData = {"
        "                query: query,"
        "                timestamp: Date.now(),"
        "                source: 'test_html'"
        "            };"
        "            "
        "            const result = window.ChopsBridge.sendMessage('searchRequested', searchData);"
        "            addDebugLog('Search message sent with result: ' + result);"
        "            addCommLog('SEARCH sent for: ' + query + ' (result: ' + result + ')');"
        "        }"
        "        "
        "        function displayResults(samples) {"
        "            const resultsEl = document.getElementById('results');"
        "            const listEl = document.getElementById('resultsList');"
        "            "
        "            if (samples && samples.length > 0) {"
        "                resultsEl.style.display = 'block';"
        "                listEl.innerHTML = '';"
        "                "
        "                samples.forEach((sample, index) => {"
        "                    const item = document.createElement('div');"
        "                    item.className = 'sample-item';"
        "                    item.innerHTML = "
        "                        '<strong>' + (sample.fullChordName || sample.chordTypeDisplay || 'Unknown') + '</strong><br>' +"
        "                        '<small>' + sample.currentFilename + '</small><br>' +"
        "                        '<small>ID: ' + sample.id + ', Root: ' + (sample.rootNote || 'N/A') + ', Type: ' + (sample.chordType || 'N/A') + '</small>';"
        "                    item.onclick = () => selectSample(sample);"
        "                    listEl.appendChild(item);"
        "                });"
        "                "
        "                updateStatus('Received ' + samples.length + ' samples from C++');"
        "                addDebugLog('Displayed ' + samples.length + ' samples in UI');"
        "            } else {"
        "                resultsEl.style.display = 'none';"
        "                updateStatus('No samples received from C++');"
        "                addDebugLog('No samples to display');"
        "            }"
        "        }"
        "        "
        "        function selectSample(sample) {"
        "            addDebugLog('Sample selected: ' + sample.currentFilename + ' (ID: ' + sample.id + ')');"
        "            addCommLog('SELECT: ' + sample.currentFilename);"
        "            "
        "            const result = window.ChopsBridge.sendMessage('sampleSelected', {"
        "                id: sample.id,"
        "                filePath: sample.filePath || sample.currentFilename"
        "            });"
        "            "
        "            addDebugLog('Sample selection sent with result: ' + result);"
        "        }"
        "        "
        "        // Initialize immediately"
        "        addDebugLog('Enhanced test page initialized');"
        "        updateStatus('Enhanced bridge ready - all methods available');"
        "        bridgeReady = true;"
        "        "
        "        // Send bridge ready message"
        "        setTimeout(() => {"
        "            addDebugLog('Sending bridgeReady message...');"
        "            const readyResult = window.ChopsBridge.sendMessage('bridgeReady', {"
        "                timestamp: Date.now(),"
        "                userAgent: navigator.userAgent,"
        "                location: window.location.href,"
        "                enhanced: true,"
        "                version: '2.0'"
        "            });"
        "            addDebugLog('Bridge ready message sent with result: ' + readyResult);"
        "            addCommLog('BRIDGE READY sent (result: ' + readyResult + ')');"
        "        }, 500);"
        "        "
        "        // Auto-test after initialization"
        "        setTimeout(() => {"
        "            addDebugLog('Running automatic bridge test...');"
        "            testBridge();"
        "        }, 2000);"
        "        "
        "        // Global debug functions"
        "        window.chopsBridgeDebug = {"
        "            test: testBridge,"
        "            search: performSearch,"
        "            clear: clearLogs,"
        "            status: () => ({ bridgeReady, messageCounter, debugLog, commLog }),"
        "            send: (type, data) => window.ChopsBridge.sendMessage(type, data)"
        "        };"
        "        "
        "        console.log('=== ENHANCED TEST PAGE FULLY LOADED ===');"
        "    </script>"
        "</body>"
        "</html>";
    
    // Create temp file and load it
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File htmlFile = tempDir.getChildFile("chops_browser_enhanced_test.html");
    
    if (htmlFile.replaceWithText(enhancedHTML)) {
        juce::URL fileUrl(htmlFile);
        juce::String fileUrlString = fileUrl.toString(false);
        logFile.appendText("✅ Enhanced HTML file created: " + htmlFile.getFullPathName() + "\n");
        logFile.appendText("Loading from URL: " + fileUrlString + "\n");
        
        webBrowser->goToURL(fileUrlString);
        contentLoadAttempted = true;
    } else {
        logFile.appendText("❌ Failed to create enhanced HTML file\n");
    }
    
    logFile.appendText("================================\n\n");
}

//==============================================================================
juce::File UIBridge::getUIHtmlFile()
{
    // Always return this exact file, no searching needed:
    return juce::File("/Users/grulf/PROGRAMERING/ChopsBrowser/UI/dist/index.html");
}

//==============================================================================
// Component Interface

void UIBridge::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    if (initializationFailed)
    {
        g.fillAll(juce::Colour(0xff1a1a1a));
        g.setColour(juce::Colours::red);
        g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
        g.drawText("WebBrowser Failed to Initialize", bounds, juce::Justification::centred);
        return;
    }
    
    g.fillAll(juce::Colour(0xff2a2a2a));
    
    if (!uiLoaded || !webBrowser || !contentLoadAttempted)
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(20.0f));
        g.drawText("🎵 Chops Browser Loading...", bounds.reduced(0, 20), 
                   juce::Justification::centred);
        
        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::FontOptions(14.0f));
        
        juce::String statusText;
        if (initializationFailed) {
            statusText = "❌ WebBrowser initialization failed";
        } else if (!webBrowser) {
            statusText = "⚠️ WebBrowser component not created";
        } else if (!contentLoadAttempted) {
            statusText = "⏳ Preparing to load content...";
        } else if (!uiLoaded) {
            statusText = "📄 Loading HTML content...";
        } else {
            statusText = "✅ Ready";
        }
        
        g.drawText(statusText, bounds.translated(0, 20), juce::Justification::centred);
    }
}

void UIBridge::resized()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    auto bounds = getLocalBounds();
    
    logFile.appendText("=== UIBridge RESIZED ===\n");
    logFile.appendText("New bounds: " + bounds.toString() + "\n");
    logFile.appendText("Has webBrowser: " + juce::String(webBrowser ? "yes" : "no") + "\n");
    
    if (webBrowser && !initializationFailed) {
        webBrowser->setBounds(bounds);
        webBrowser->setVisible(true);
        logFile.appendText("✅ WebBrowser bounds set and made visible\n");
    } else {
        logFile.appendText("❌ Cannot resize - no webBrowser or initialization failed\n");
    }
    
    repaint();
    logFile.appendText("===================\n\n");
}

//==============================================================================
// FIXED: Better URL scheme message handling
bool UIBridge::pageAboutToLoad(const juce::String& newURL)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    currentURL = newURL;
    
    logFile.appendText("=== PAGE ABOUT TO LOAD ===\n");
    logFile.appendText("URL: " + newURL + "\n");
    
    // ENHANCED: Better message detection and handling
    if (newURL.startsWith("chops://message/"))
    {
        juce::String encodedMessage = newURL.substring(16); // Remove "chops://message/"
        
        logFile.appendText("🎯 CHOPS MESSAGE DETECTED!\n");
        logFile.appendText("Encoded length: " + juce::String(encodedMessage.length()) + "\n");
        logFile.appendText("First 200 chars: " + encodedMessage.substring(0, 200) + "...\n");
        
        try {
            juce::String decodedMessage = juce::URL::removeEscapeChars(encodedMessage);
            logFile.appendText("Decoded length: " + juce::String(decodedMessage.length()) + "\n");
            logFile.appendText("Decoded preview: " + decodedMessage.substring(0, 300) + "...\n");
            
            // Process the message
            handleJavaScriptMessage(decodedMessage);
            
            logFile.appendText("✅ Message processed successfully\n");
        } catch (const std::exception& e) {
            logFile.appendText("❌ Error processing message: " + juce::String(e.what()) + "\n");
        }
        
        logFile.appendText("========================\n\n");
        return false; // Don't navigate to this URL
    }
    
    logFile.appendText("Normal page load, allowing navigation\n");
    logFile.appendText("=======================\n\n");
    return true;
}

void UIBridge::pageFinishedLoading(const juce::String& url)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== PAGE FINISHED LOADING ===\n");
    logFile.appendText("URL: " + url + "\n");
    
    // ENHANCED: Better JavaScript bridge injection
    juce::String enhancedBridgeScript = 
        "console.log('=== C++ JAVASCRIPT BRIDGE INJECTION ===');"
        "console.log('Page URL:', window.location.href);"
        "console.log('Timestamp:', new Date().toISOString());"
        ""
        "// ENHANCED: Create robust C++ bridge"
        "if (!window.ChopsBridge || !window.ChopsBridge._cppInjected) {"
        "    console.log('🔧 Creating C++ Bridge...');"
        "    window.ChopsBridge = {"
        "        _cppInjected: true,"
        "        _messageCounter: 0,"
        "        "
        "        sendMessage: function(type, data) {"
        "            this._messageCounter++;"
        "            const messageId = 'cpp_' + this._messageCounter;"
        "            console.log('📤 C++ Bridge sending [' + messageId + ']:', type, data);"
        "            "
        "            try {"
        "                const message = { "
        "                    id: messageId,"
        "                    type: type, "
        "                    data: data, "
        "                    timestamp: Date.now(),"
        "                    source: 'cpp_bridge'"
        "                };"
        "                const messageStr = JSON.stringify(message);"
        "                "
        "                console.log('📤 Message JSON [' + messageId + ']:', messageStr.substring(0, 200) + '...');"
        "                "
        "                // Primary: URL scheme communication"
        "                const schemeUrl = 'chops://message/' + encodeURIComponent(messageStr);"
        "                console.log('📤 Sending via URL scheme [' + messageId + ']...');"
        "                window.location.href = schemeUrl;"
        "                "
        "                // Backup: Global variable"
        "                window.lastChopsBridgeMessage = message;"
        "                "
        "                console.log('✅ Message sent successfully [' + messageId + ']');"
        "                return 'CPP_BRIDGE_OK_' + messageId;"
        "            } catch (e) {"
        "                console.error('❌ C++ Bridge send error [' + messageId + ']:', e);"
        "                return 'CPP_BRIDGE_ERROR_' + messageId;"
        "            }"
        "        },"
        "        "
        "        callbacks: {},"
        "        "
        "        setCallback: function(name, callback) {"
        "            console.log('🔗 C++ Bridge setting callback:', name);"
        "            this.callbacks[name] = callback;"
        "        },"
        "        "
        "        getStatus: function() {"
        "            return {"
        "                ready: true,"
        "                messageCounter: this._messageCounter,"
        "                callbackCount: Object.keys(this.callbacks).length,"
        "                injectedBy: 'cpp',"
        "                timestamp: Date.now()"
        "            };"
        "        }"
        "    };"
        "    "
        "    console.log('✅ C++ Bridge created successfully');"
        "} else {"
        "    console.log('♻️ C++ Bridge already exists, reusing...');"
        "}"
        ""
        "// Test the bridge immediately"
        "setTimeout(() => {"
        "    console.log('🧪 Testing C++ Bridge immediately...');"
        "    try {"
        "        const testResult = window.ChopsBridge.sendMessage('immediateTest', {"
        "            timestamp: Date.now(),"
        "            userAgent: navigator.userAgent"
        "        });"
        "        console.log('🧪 Immediate test result:', testResult);"
        "    } catch (e) {"
        "        console.error('🧪 Immediate test failed:', e);"
        "    }"
        "}, 100);"
        ""
        "console.log('=== C++ BRIDGE INJECTION COMPLETE ===');";
    
    logFile.appendText("📝 Injecting enhanced C++ JavaScript bridge...\n");
    executeJavaScript(enhancedBridgeScript);
    
    uiLoaded = true;
    
    // Send any pending messages
    sendPendingMessages();
    
    logFile.appendText("✅ Page load complete, bridge injected, pending messages sent\n");
    logFile.appendText("=============================\n\n");
    
    repaint();
}

//==============================================================================
// Message Handling

void UIBridge::handleJavaScriptCall(const juce::String& functionName, 
                                   const juce::StringArray& parameters)
{
    // This method might not be called in modern JUCE versions
    // The URL scheme approach is more reliable
    juce::Logger::writeToLog("UIBridge: JavaScript call: " + functionName);
}

void UIBridge::handleJavaScriptMessage(const juce::String& message)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("=== PROCESSING JAVASCRIPT MESSAGE ===\n");
    logFile.appendText("Message length: " + juce::String(message.length()) + "\n");
    logFile.appendText("Message preview: " + message.substring(0, 300) + "...\n");
    
    try
    {
        juce::var messageData = juce::JSON::parse(message);
        if (messageData.isObject())
        {
            juce::String messageType = messageData.getProperty("type", "unknown").toString();
            juce::String messageId = messageData.getProperty("id", "no_id").toString();
            
            logFile.appendText("✅ Valid JSON message parsed\n");
            logFile.appendText("Type: " + messageType + "\n");
            logFile.appendText("ID: " + messageId + "\n");
            
            processJavaScriptMessage(messageData);
        }
        else
        {
            logFile.appendText("❌ Invalid message format - not a JSON object\n");
        }
    }
    catch (const std::exception& e)
    {
        logFile.appendText("❌ JSON parsing error: " + juce::String(e.what()) + "\n");
        juce::Logger::writeToLog("UIBridge: Error parsing message: " + juce::String(e.what()));
    }
    
    logFile.appendText("====================================\n\n");
}

void UIBridge::processJavaScriptMessage(const juce::var& messageData)
{
    juce::String type = messageData.getProperty("type", "").toString();
    juce::String messageId = messageData.getProperty("id", "").toString();
    juce::var data = messageData.getProperty("data", juce::var());
    
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("🔄 Processing message [" + messageId + "] type: " + type + "\n");
    juce::Logger::writeToLog("UIBridge: Processing message type: " + type);
    
    // FIXED: Handle bridge ready and React ready messages properly
    if (type == "bridgeReady" || type == "bridgeTest" || type == "immediateTest" || type == "reactReady")
    {
        logFile.appendText("✅ Bridge/React ready message received: " + type + "\n");
        juce::Logger::writeToLog("UIBridge: Bridge/React is ready and communicating");
        
        // FIXED: For ANY ready message, trigger the UI event callback to send initial data
        if (callbacks.onUIEvent)
        {
            logFile.appendText("📤 Sending initial data in response to " + type + "\n");
            callbacks.onUIEvent("bridgeReady", data);
        }
        else
        {
            logFile.appendText("⚠️ No onUIEvent callback available to send initial data\n");
        }
    }
    else if (type == "searchRequested")
    {
        logFile.appendText("🔍 SEARCH REQUEST RECEIVED!\n");
        handleSearchMessage(data);
    }
    else if (type == "chordSelected")
    {
        logFile.appendText("🎵 Chord selection received\n");
        handleChordSelectionMessage(data);
    }
    else if (type == "sampleSelected")
    {
        logFile.appendText("🎧 Sample selection received\n");
        handleSampleSelectionMessage(data);
    }
    else if (type == "preview")
    {
        logFile.appendText("▶️ Preview message received\n");
        handlePreviewMessage(data);
    }
    else if (type == "sampleMetadata")
    {
        logFile.appendText("📝 Sample metadata message received\n");
        handleSampleMetadataMessage(data);
    }
    else if (type == "library")
    {
        logFile.appendText("📚 Library message received\n");
        handleLibraryMessage(data);
    }
    else if (type == "uiEvent")
    {
        logFile.appendText("🎛️ UI event message received\n");
        handleUIEventMessage(data);
    }
    else
    {
        logFile.appendText("❓ Unknown message type: " + type + "\n");
        juce::Logger::writeToLog("UIBridge: Unknown message type: " + type);
    }
}

//==============================================================================
// JavaScript execution

void UIBridge::executeJavaScript(const juce::String& script)
{
    if (webBrowser && !initializationFailed)
    {
        try {
            webBrowser->evaluateJavascript(script);
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("JavaScript execution failed: " + juce::String(e.what()));
        }
    }
    else
    {
        pendingMessages.add(script);
    }
}

void UIBridge::executeJavaScriptWhenReady(const juce::String& script)
{
    if (uiLoaded && !initializationFailed)
    {
        executeJavaScript(script);
    }
    else
    {
        pendingMessages.add(script);
    }
}

void UIBridge::sendPendingMessages()
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("📤 Sending " + juce::String(pendingMessages.size()) + " pending messages\n");
    
    for (const auto& script : pendingMessages)
    {
        executeJavaScript(script);
    }
    pendingMessages.clear();
}

//==============================================================================
// C++ to React Communication

void UIBridge::sendChordData(const ChordParser::ParsedData& chordData)
{
    juce::var data = chordDataToVar(chordData);
    juce::String script = "if (window.ChopsBridge && window.ChopsBridge.callbacks.onChordData) { "
                         "window.ChopsBridge.callbacks.onChordData(" + 
                         juce::JSON::toString(data) + "); }";
    executeJavaScriptWhenReady(script);
}

void UIBridge::sendSampleResults(const std::vector<ChopsDatabase::SampleInfo>& samples)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("📤 SENDING SAMPLE RESULTS TO UI\n");
    logFile.appendText("Sample count: " + juce::String(samples.size()) + "\n");
    
    if (samples.size() > 0) {
        logFile.appendText("Sample examples:\n");
        for (int i = 0; i < juce::jmin(3, (int)samples.size()); ++i) {
            const auto& sample = samples[i];
            logFile.appendText("  " + juce::String(i+1) + ". " + sample.currentFilename + 
                             " (Root: " + sample.rootNote + ", Type: " + sample.chordType + ")\n");
        }
    }
    
    juce::var data = sampleArrayToVar(samples);
    
    // --- START OF FIX ---
    // 1. Convert to a JSON string.
    juce::String jsonString = juce::JSON::toString(data, true);

    // 2. Escape the string to make it safe for JavaScript.
    jsonString.replace("\\", "\\\\");
    jsonString.replace("'", "\\'"); // Escape single quotes
    jsonString.replace("\n", "\\n");
    jsonString.replace("\r", "\\r");

    // 3. Build the robust script.
    juce::String script = 
        "console.log('📦 C++ sending ' + " + juce::String(samples.size()) + " + ' samples to React'); "
        "try { "
        "    const samplesData = JSON.parse('" + jsonString + "'); " // <-- Use JSON.parse with the safe string
        "    console.log('📦 Sample data prepared:', samplesData.length, 'items'); "
        "    let delivered = false; "
        "    if (window.ChopsBridge && window.ChopsBridge.callbacks && window.ChopsBridge.callbacks.onSampleResults) { "
        "        console.log('✅ Using Method 1: Direct callback'); "
        "        window.ChopsBridge.callbacks.onSampleResults(samplesData); "
        "        delivered = true; "
        "    } "
        "    if (window.reactAppCallbacks && window.reactAppCallbacks.onSampleResults) { "
        "        console.log('✅ Using Method 2: React app callbacks'); "
        "        window.reactAppCallbacks.onSampleResults(samplesData); "
        "        delivered = true; "
        "    } "
        "    if (!delivered) { "
        "        console.error('❌ No sample result callbacks found!'); "
        "        window.pendingSampleResults = samplesData; "
        "    } else { "
        "        console.log('✅ Sample results delivered successfully'); "
        "    } "
        "} catch (error) { "
        "    console.error('❌ Error delivering sample results:', error); "
        "    console.log('Problematic JSON string was:', '" + jsonString + "'); "
        "}";
    // --- END OF FIX ---
    
    executeJavaScriptWhenReady(script);
    
    logFile.appendText("✅ Sample results script sent to UI\n");
    juce::Logger::writeToLog("Sent " + juce::String(samples.size()) + " samples to UI");
}

void UIBridge::sendSelectedSample(const ChopsDatabase::SampleInfo& sample)
{
    juce::var data = sampleInfoToVar(sample);
    juce::String script = "if (window.ChopsBridge && window.ChopsBridge.callbacks.onSelectedSample) { "
                         "window.ChopsBridge.callbacks.onSelectedSample(" + 
                         juce::JSON::toString(data) + "); }";
    executeJavaScriptWhenReady(script);
}

void UIBridge::sendLoadingState(bool isLoading)
{
    juce::var data = juce::var(isLoading);
    juce::String script = 
        "console.log('⏳ C++ sending loading state: " + juce::String(isLoading ? "true" : "false") + "'); "
        "try { "
        "    const loadingState = " + juce::JSON::toString(data) + "; "
        "    let delivered = false; "
        "    "
        "    // Try multiple callback methods "
        "    if (window.ChopsBridge && window.ChopsBridge.callbacks && window.ChopsBridge.callbacks.onLoadingState) { "
        "        window.ChopsBridge.callbacks.onLoadingState(loadingState); "
        "        delivered = true; "
        "    } "
        "    if (window.reactAppCallbacks && window.reactAppCallbacks.onLoadingState) { "
        "        window.reactAppCallbacks.onLoadingState(loadingState); "
        "        delivered = true; "
        "    } "
        "    if (window.reactCallbacks && window.reactCallbacks.onLoadingState) { "
        "        window.reactCallbacks.onLoadingState(loadingState); "
        "        delivered = true; "
        "    } "
        "    "
        "    if (!delivered) { "
        "        console.warn('⚠️ No loading state callbacks found'); "
        "    } "
        "} catch (error) { "
        "    console.error('❌ Error sending loading state:', error); "
        "}";
    executeJavaScriptWhenReady(script);
}

void UIBridge::sendErrorMessage(const juce::String& error)
{
    juce::var data = juce::var(error);
    juce::String script = 
        "console.error('❌ C++ Error:', '" + error + "'); "
        "try { "
        "    const errorMsg = " + juce::JSON::toString(data) + "; "
        "    let delivered = false; "
        "    "
        "    // Try multiple callback methods "
        "    if (window.ChopsBridge && window.ChopsBridge.callbacks && window.ChopsBridge.callbacks.onErrorMessage) { "
        "        window.ChopsBridge.callbacks.onErrorMessage(errorMsg); "
        "        delivered = true; "
        "    } "
        "    if (window.reactAppCallbacks && window.reactAppCallbacks.onErrorMessage) { "
        "        window.reactAppCallbacks.onErrorMessage(errorMsg); "
        "        delivered = true; "
        "    } "
        "    if (window.reactCallbacks && window.reactCallbacks.onErrorMessage) { "
        "        window.reactCallbacks.onErrorMessage(errorMsg); "
        "        delivered = true; "
        "    } "
        "    "
        "    if (!delivered) { "
        "        console.warn('⚠️ No error message callbacks found'); "
        "    } "
        "} catch (error) { "
        "    console.error('❌ Error sending error message:', error); "
        "}";
    executeJavaScriptWhenReady(script);
}

void UIBridge::sendPreviewState(bool isPlaying, float progress)
{
    juce::var data = new juce::DynamicObject();
    data.getDynamicObject()->setProperty("isPlaying", isPlaying);
    data.getDynamicObject()->setProperty("progress", progress);
    
    juce::String script = "if (window.ChopsBridge && window.ChopsBridge.callbacks.onPreviewState) { "
                         "window.ChopsBridge.callbacks.onPreviewState(" + 
                         juce::JSON::toString(data) + "); }";
    executeJavaScriptWhenReady(script);
}

void UIBridge::sendDatabaseStats(const ChopsDatabase::Statistics& stats)
{
    juce::var data = statsToVar(stats);
    
    // --- START OF FIX ---
    juce::String jsonString = juce::JSON::toString(data, true);
    jsonString.replace("\\", "\\\\");
    jsonString.replace("'", "\\'");
    jsonString.replace("\n", "\\n");
    jsonString.replace("\r", "\\r");

    juce::String script = 
        "console.log('📊 C++ sending database stats...'); "
        "try { "
        "    const statsData = JSON.parse('" + jsonString + "'); "
        "    let delivered = false; "
        "    if (window.ChopsBridge && window.ChopsBridge.callbacks && window.ChopsBridge.callbacks.onDatabaseStats) { "
        "        window.ChopsBridge.callbacks.onDatabaseStats(statsData); "
        "        delivered = true; "
        "    } "
        "    if (!delivered) { "
        "        console.warn('⚠️ No database stats callbacks found'); "
        "    } "
        "} catch (error) { "
        "    console.error('❌ Error sending database stats:', error); "
        "}";
    // --- END OF FIX ---
    
    executeJavaScriptWhenReady(script);
}

void UIBridge::sendLibraryPath(const juce::String& path)
{
    // --- START OF FIX ---
    // No need for full JSON, but the path itself must be escaped.
    juce::String safePath = path;
    safePath.replace("\\", "\\\\");
    safePath.replace("'", "\\'");

    juce::String script = "if (window.ChopsBridge && window.ChopsBridge.callbacks.onLibraryPath) { "
                         "window.ChopsBridge.callbacks.onLibraryPath('" + safePath + "'); }";
    // --- END OF FIX ---

    executeJavaScriptWhenReady(script);
}

//==============================================================================
// React to C++ Communication Handlers

void UIBridge::handleSearchMessage(const juce::var& data)
{
    juce::String query = data.getProperty("query", "").toString();
    
    auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                   .getChildFile("ChopsBrowser_VST_Debug.log");
    
    logFile.appendText("🔍 SEARCH MESSAGE HANDLER\n");
    logFile.appendText("Query: '" + query + "'\n");
    logFile.appendText("Callback available: " + juce::String(callbacks.onSearchRequested ? "YES" : "NO") + "\n");
    
    juce::Logger::writeToLog("UIBridge: Received search request: " + query);
    
    if (callbacks.onSearchRequested) {
        logFile.appendText("✅ Calling search callback\n");
        callbacks.onSearchRequested(query);
    } else {
        logFile.appendText("❌ No search callback set!\n");
    }
    
    logFile.appendText("=========================\n\n");
}

void UIBridge::handleChordSelectionMessage(const juce::var& data)
{
    ChordParser::ParsedData chordData = varToChordData(data);
    if (callbacks.onChordSelected)
        callbacks.onChordSelected(chordData);
}

void UIBridge::handleSampleSelectionMessage(const juce::var& data)
{
    int sampleId = static_cast<int>(data.getProperty("id", 0));
    if (callbacks.onSampleSelected)
        callbacks.onSampleSelected(sampleId);
}

void UIBridge::handlePreviewMessage(const juce::var& data)
{
    juce::String action = data.getProperty("action", "").toString();
    
    if (action == "play")
    {
        int sampleId = static_cast<int>(data.getProperty("sampleId", 0));
        
        if (sampleId > 0 && callbacks.onSampleSelected)
        {
            callbacks.onSampleSelected(sampleId);
        }
        
        if (callbacks.onPreviewPlay)
            callbacks.onPreviewPlay();
    }
    else if (action == "stop" && callbacks.onPreviewStop)
    {
        callbacks.onPreviewStop();
    }
    else if (action == "seek" && callbacks.onPreviewSeek)
    {
        float position = static_cast<float>(data.getProperty("position", 0.0));
        callbacks.onPreviewSeek(position);
    }
}

void UIBridge::handleSampleMetadataMessage(const juce::var& data)
{
    juce::String action = data.getProperty("action", "").toString();
    int sampleId = static_cast<int>(data.getProperty("sampleId", 0));
    
    if (action == "setRating" && callbacks.onSampleRatingChanged)
    {
        int rating = static_cast<int>(data.getProperty("rating", 0));
        callbacks.onSampleRatingChanged(sampleId, rating);
    }
    else if (action == "addTag" && callbacks.onSampleTagAdded)
    {
        juce::String tag = data.getProperty("tag", "").toString();
        callbacks.onSampleTagAdded(sampleId, tag);
    }
    else if (action == "toggleFavorite" && callbacks.onSampleFavoriteToggled)
    {
        callbacks.onSampleFavoriteToggled(sampleId);
    }
}

void UIBridge::handleLibraryMessage(const juce::var& data)
{
    juce::String action = data.getProperty("action", "").toString();
    
    if (action == "setPath" && callbacks.onLibraryPathChanged)
    {
        juce::String path = data.getProperty("path", "").toString();
        callbacks.onLibraryPathChanged(path);
    }
    else if (action == "rescan" && callbacks.onLibraryRescanRequested)
    {
        callbacks.onLibraryRescanRequested();
    }
}

void UIBridge::handleUIEventMessage(const juce::var& data)
{
    juce::String eventType = data.getProperty("eventType", "").toString();
    juce::var eventData = data.getProperty("eventData", juce::var());
    
    if (callbacks.onUIEvent)
        callbacks.onUIEvent(eventType, eventData);
}

//==============================================================================
// Data Serialization Helpers

juce::var UIBridge::chordDataToVar(const ChordParser::ParsedData& data)
{
    auto obj = new juce::DynamicObject();
    
    obj->setProperty("rootNote", data.rootNote);
    obj->setProperty("standardizedQuality", data.standardizedQuality);
    obj->setProperty("originalFilename", data.originalFilename);
    obj->setProperty("determinedBassNote", data.determinedBassNote);
    obj->setProperty("isInterval", data.standardizedQuality.startsWith("interval_"));
    
    juce::Array<juce::var> extensions, alterations, addedNotes, suspensions;
    for (const auto& ext : data.extensions) extensions.add(ext);
    for (const auto& alt : data.alterations) alterations.add(alt);
    for (const auto& add : data.addedNotes) addedNotes.add(add);
    for (const auto& sus : data.suspensions) suspensions.add(sus);
    
    obj->setProperty("extensions", extensions);
    obj->setProperty("alterations", alterations);
    obj->setProperty("addedNotes", addedNotes);
    obj->setProperty("suspensions", suspensions);
    
    return juce::var(obj);
}

juce::var UIBridge::sampleInfoToVar(const ChopsDatabase::SampleInfo& sample)
{
    auto obj = new juce::DynamicObject();
    
    obj->setProperty("id", sample.id);
    obj->setProperty("originalFilename", sample.originalFilename);
    obj->setProperty("currentFilename", sample.currentFilename);
    obj->setProperty("filePath", sample.filePath);
    obj->setProperty("fileSize", static_cast<int64>(sample.fileSize));
    obj->setProperty("rootNote", sample.rootNote);
    obj->setProperty("chordType", sample.chordType);
    obj->setProperty("chordTypeDisplay", sample.chordTypeDisplay);
    obj->setProperty("bassNote", sample.bassNote);
    obj->setProperty("inversion", sample.inversion);
    obj->setProperty("rating", sample.rating);
    obj->setProperty("isFavorite", sample.isFavorite);
    obj->setProperty("playCount", sample.playCount);
    obj->setProperty("userNotes", sample.userNotes);
    
    obj->setProperty("fullChordName", sample.getFullChordName());
    
    juce::Array<juce::var> tags, extensions, alterations, addedNotes, suspensions;
    for (const auto& tag : sample.tags) tags.add(tag);
    for (const auto& ext : sample.extensions) extensions.add(ext);
    for (const auto& alt : sample.alterations) alterations.add(alt);
    for (const auto& add : sample.addedNotes) addedNotes.add(add);
    for (const auto& sus : sample.suspensions) suspensions.add(sus);
    
    obj->setProperty("tags", tags);
    obj->setProperty("extensions", extensions);
    obj->setProperty("alterations", alterations);
    obj->setProperty("addedNotes", addedNotes);
    obj->setProperty("suspensions", suspensions);
    
    obj->setProperty("dateAdded", sample.dateAdded.toISO8601(false));
    obj->setProperty("dateModified", sample.dateModified.toISO8601(false));
    if (sample.lastPlayed.toMilliseconds() > 0)
        obj->setProperty("lastPlayed", sample.lastPlayed.toISO8601(false));
    
    obj->setProperty("color", sample.color.toDisplayString(true));
    
    return juce::var(obj);
}

juce::var UIBridge::sampleArrayToVar(const std::vector<ChopsDatabase::SampleInfo>& samples)
{
    juce::Array<juce::var> sampleArray;
    for (const auto& sample : samples)
    {
        sampleArray.add(sampleInfoToVar(sample));
    }
    return juce::var(sampleArray);
}

juce::var UIBridge::statsToVar(const ChopsDatabase::Statistics& stats)
{
    auto obj = new juce::DynamicObject();
    
    obj->setProperty("totalSamples", stats.totalSamples);
    obj->setProperty("withExtensions", stats.withExtensions);
    obj->setProperty("withAlterations", stats.withAlterations);
    obj->setProperty("addedLastWeek", stats.addedLastWeek);
    
    juce::Array<juce::var> chordTypes, rootNotes;
    for (const auto& [name, count] : stats.byChordType)
    {
        auto item = new juce::DynamicObject();
        item->setProperty("name", name);
        item->setProperty("count", count);
        chordTypes.add(juce::var(item));
    }
    for (const auto& [note, count] : stats.byRootNote)
    {
        auto item = new juce::DynamicObject();
        item->setProperty("note", note);
        item->setProperty("count", count);
        rootNotes.add(juce::var(item));
    }
    
    obj->setProperty("byChordType", chordTypes);
    obj->setProperty("byRootNote", rootNotes);
    
    return juce::var(obj);
}

ChordParser::ParsedData UIBridge::varToChordData(const juce::var& data)
{
    ChordParser::ParsedData chordData;
    
    chordData.rootNote = data.getProperty("rootNote", "").toString();
    chordData.standardizedQuality = data.getProperty("standardizedQuality", "").toString();
    chordData.determinedBassNote = data.getProperty("determinedBassNote", "").toString();
    
    if (auto* extArray = data.getProperty("extensions", juce::var()).getArray())
    {
        for (auto& ext : *extArray)
            chordData.extensions.add(ext.toString());
    }
    if (auto* altArray = data.getProperty("alterations", juce::var()).getArray())
    {
        for (auto& alt : *altArray)
            chordData.alterations.add(alt.toString());
    }
    if (auto* addArray = data.getProperty("addedNotes", juce::var()).getArray())
    {
        for (auto& add : *addArray)
            chordData.addedNotes.add(add.toString());
    }
    if (auto* susArray = data.getProperty("suspensions", juce::var()).getArray())
    {
        for (auto& sus : *susArray)
            chordData.suspensions.add(sus.toString());
    }
    
    return chordData;
}

//==============================================================================
// Configuration and Management

void UIBridge::setCallbacks(const Callbacks& newCallbacks)
{
    callbacks = newCallbacks;
}

void UIBridge::loadUI(const juce::File& htmlFile)
{
    if (webBrowser && htmlFile.existsAsFile() && !initializationFailed)
    {
        juce::URL fileUrl(htmlFile);
        juce::String urlString = fileUrl.toString(false);
        webBrowser->goToURL(urlString);
        juce::Logger::writeToLog("UIBridge: Loading UI from file: " + urlString);
        contentLoadAttempted = true;
    }
}

void UIBridge::loadUI(const juce::String& htmlContent)
{
    if (webBrowser && !initializationFailed)
    {
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        juce::File htmlFile = tempDir.getChildFile("chops_browser_ui_" + 
                                                   juce::String(juce::Time::getCurrentTime().toMilliseconds()) + 
                                                   ".html");
        
        if (htmlFile.replaceWithText(htmlContent))
        {
            juce::URL fileUrl(htmlFile);
            juce::String urlString = fileUrl.toString(false);
            webBrowser->goToURL(urlString);
            juce::Logger::writeToLog("UIBridge: Loading UI from HTML content via temp file");
            contentLoadAttempted = true;
        }
    }
}

void UIBridge::reloadUI()
{
    if (webBrowser && currentURL.isNotEmpty() && !initializationFailed)
    {
        webBrowser->goToURL(currentURL);
    }
}

void UIBridge::enableDevMode(bool enabled)
{
    devModeEnabled = enabled;
    if (enabled)
    {
        juce::Logger::writeToLog("UIBridge: Development mode enabled");
    }
}

void UIBridge::openDevTools()
{
    if (devModeEnabled && webBrowser)
    {
        juce::Logger::writeToLog("UIBridge: Opening dev tools");
    }
}

void UIBridge::injectTestData()
{
    if (!devModeEnabled) return;
    
    juce::Logger::writeToLog("UIBridge: Test data injection disabled - using real data");
}
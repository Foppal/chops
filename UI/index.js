import React from "react";
import ReactDOM from "react-dom/client";
import ChopsBrowserUI from "./ChopsBrowserUI";

// Import styles
import "./styles/ChopsBrowserUI.css";

// Initialize error handling for development
if (process.env.NODE_ENV === "development") {
  window.addEventListener("error", (e) => {
    console.error("React Error:", e.error);
  });

  window.addEventListener("unhandledrejection", (e) => {
    console.error("Unhandled Promise Rejection:", e.reason);
  });
}

// FIXED: Enhanced C++ Bridge Detection and Setup
function initializeChopsBridge() {
  console.log("=== INITIALIZING CHOPS BRIDGE ===");
  console.log("Environment:", process.env.NODE_ENV);
  console.log("User Agent:", navigator.userAgent);
  console.log("Window location:", window.location.href);

  // Wait for C++ bridge to be injected (give it time)
  let bridgeCheckAttempts = 0;
  const maxAttempts = 50; // 5 seconds total

  function checkForCppBridge() {
    bridgeCheckAttempts++;
    console.log(`Bridge check attempt ${bridgeCheckAttempts}/${maxAttempts}`);

    // Check if we're in the JUCE WebBrowserComponent environment
    const isInJuce =
      window.location.href.startsWith("file://") &&
      (window.location.href.includes("chops_browser_ui") ||
        window.location.href.includes("ChopsBrowser"));

    console.log("Is in JUCE environment:", isInJuce);

    if (isInJuce) {
      // We're in JUCE - wait for C++ to inject the bridge
      if (
        window.ChopsBridge &&
        typeof window.ChopsBridge.sendMessage === "function"
      ) {
        // C++ bridge is ready
        console.log("✅ C++ Bridge detected and ready!");

        // Test the bridge
        try {
          const testResult = window.ChopsBridge.sendMessage("bridgeTest", {
            timestamp: Date.now(),
            userAgent: navigator.userAgent,
          });
          console.log("Bridge test result:", testResult);

          // Set up the React app with the real bridge
          startReactApp(true);
          return;
        } catch (e) {
          console.error("Bridge test failed:", e);
        }
      }

      // If we haven't found the bridge yet and haven't exceeded max attempts, try again
      if (bridgeCheckAttempts < maxAttempts) {
        setTimeout(checkForCppBridge, 100);
        return;
      } else {
        console.warn("⚠️ Timeout waiting for C++ bridge, using fallback");
        createFallbackBridge();
        startReactApp(false);
      }
    } else {
      // Not in JUCE environment - use development mock
      console.log("🔧 Not in JUCE environment, using development mock");
      createMockBridge();
      startReactApp(false);
    }
  }

  // Start checking for the bridge
  checkForCppBridge();
}

function createFallbackBridge() {
  console.log("Creating fallback bridge for JUCE environment");

  window.ChopsBridge = {
    sendMessage: function (type, data) {
      console.log("Fallback Bridge - Message to C++:", type, data);

      // Try to communicate via URL scheme as last resort
      try {
        const message = { type: type, data: data };
        const messageStr = JSON.stringify(message);
        console.log("Attempting URL scheme communication:", messageStr);
        window.location.href =
          "chops://message/" + encodeURIComponent(messageStr);
        return "URL_SCHEME_ATTEMPTED";
      } catch (e) {
        console.error("Fallback communication failed:", e);
        return "FALLBACK_FAILED";
      }
    },

    callbacks: {},

    setCallback: function (name, callback) {
      console.log("Fallback Bridge - Setting callback:", name);
      this.callbacks[name] = callback;
    },
  };
}

function createMockBridge() {
  console.log("Creating mock bridge for development");

  window.ChopsBridge = {
    sendMessage: function (type, data) {
      console.log("Mock Bridge - Sending message to C++:", type, data);

      // Simulate some responses for development
      if (type === "searchRequested") {
        setTimeout(() => {
          if (this.callbacks.onSampleResults) {
            // Create more realistic mock data
            const query = data.query || "";
            const mockSamples = [
              {
                id: 1,
                currentFilename: `Mock_${query || "Sample"}_Piano.wav`,
                fullChordName: query || "C",
                chordTypeDisplay: query || "C",
                tags: ["Mock", "Piano"],
                rating: 4,
                playCount: 0,
                isFavorite: false,
                filePath: "/mock/path/sample1.wav",
                extensions: [],
                alterations: [],
                addedNotes: [],
                suspensions: [],
                rootNote: query.charAt(0) || "C",
                chordType: "maj",
              },
              {
                id: 2,
                currentFilename: `Mock_${query || "Sample"}_Guitar.wav`,
                fullChordName: `${query || "C"}m`,
                chordTypeDisplay: `${query || "C"}m`,
                tags: ["Mock", "Guitar"],
                rating: 3,
                playCount: 5,
                isFavorite: true,
                filePath: "/mock/path/sample2.wav",
                extensions: [],
                alterations: [],
                addedNotes: [],
                suspensions: [],
                rootNote: query.charAt(0) || "C",
                chordType: "min",
              },
            ];

            this.callbacks.onSampleResults(mockSamples);
          }

          if (this.callbacks.onLoadingState) {
            this.callbacks.onLoadingState(false);
          }
        }, 300);
      } else if (type === "bridgeTest") {
        console.log("Mock bridge test received");
      }

      return "MOCK_OK";
    },

    callbacks: {},

    setCallback: function (name, callback) {
      console.log("Mock Bridge - Setting callback:", name);
      this.callbacks[name] = callback;
    },
  };
}

function startReactApp(usingRealBridge) {
  console.log("=== STARTING REACT APP ===");
  console.log("Using real C++ bridge:", usingRealBridge);

  // Initialize the app when DOM is ready
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", function () {
      renderReactApp(usingRealBridge);
    });
  } else {
    renderReactApp(usingRealBridge);
  }
}

function renderReactApp(usingRealBridge) {
  console.log("Chops Browser UI initializing...");
  console.log(
    "Bridge type:",
    usingRealBridge ? "Real C++ Bridge" : "Mock/Fallback Bridge"
  );

  // Get or create root element
  let rootElement = document.getElementById("root");
  if (!rootElement) {
    rootElement = document.createElement("div");
    rootElement.id = "root";
    document.body.appendChild(rootElement);
  }

  // Render the React app
  const root = ReactDOM.createRoot(rootElement);

  root.render(
    <React.StrictMode>
      <ChopsBrowserUI />
    </React.StrictMode>
  );

  console.log("Chops Browser UI initialized successfully");

  // If using real bridge, send ready message
  if (usingRealBridge && window.ChopsBridge) {
    setTimeout(() => {
      console.log("Sending bridgeReady message to C++");
      window.ChopsBridge.sendMessage("bridgeReady", {
        timestamp: Date.now(),
        reactVersion: React.version,
      });
    }, 500);
  }
}

// Start the initialization process
initializeChopsBridge();

// Export for potential use by the C++ bridge
export { ChopsBrowserUI };

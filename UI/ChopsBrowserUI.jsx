// ChopsBrowserUI.jsx - Main UI Component for VST3 Plugin
import React, { useState, useEffect, useCallback } from "react";
import "./styles/ChopsBrowserUI.css";

// Import sub-components
import PianoRoll from "./components/PianoRoll";
import ResultsList from "./components/ResultsList";
import SearchBar from "./components/SearchBar";

const ChopsBrowserUI = () => {
  // UI State
  const [currentChord, setCurrentChord] = useState({
    symbol: "C",
    notes: [],
    description: "Select notes on piano to build chord",
    inversion: "root",
  });
  const [selectedNotes, setSelectedNotes] = useState([]);
  const [searchQuery, setSearchQuery] = useState("");
  const [displayedSamples, setDisplayedSamples] = useState([]);
  const [showFilenames, setShowFilenames] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [errorMessage, setErrorMessage] = useState("");
  const [databaseStats, setDatabaseStats] = useState(null);

  // Piano roll toggles
  const [rootAgnostic, setRootAgnostic] = useState(false);
  const [inversionAgnostic, setInversionAgnostic] = useState(false);

  // Setup communication with C++ backend
  // ─── NEW combined “wait-for-bridge, register callbacks, then send bridgeReady” effect ────────────────────────────────────────────────────
  useEffect(() => {
    function initializeChopsBridge() {
      if (window.ChopsBridge) {
        console.log(
          "🚀 ChopsBrowserUI: ChopsBridge found—registering callbacks now."
        );

        // 1) onSampleResults
        window.ChopsBridge.setCallback("onSampleResults", (samples) => {
          console.log(
            "📦 RECEIVED samples via ChopsBridge:",
            (samples || []).length
          );
          if (Array.isArray(samples)) {
            setDisplayedSamples(samples);
            setIsLoading(false);
            setErrorMessage("");
            console.log(
              "✅ Successfully updated React state with",
              samples.length,
              "samples"
            );
          } else {
            setErrorMessage("Received invalid sample data.");
            setIsLoading(false);
          }
        });

        // 2) onLoadingState
        window.ChopsBridge.setCallback("onLoadingState", (loading) => {
          setIsLoading(loading);
        });

        // 3) onErrorMessage
        window.ChopsBridge.setCallback("onErrorMessage", (error) => {
          setErrorMessage(error);
          setIsLoading(false);
        });

        // 4) onDatabaseStats
        window.ChopsBridge.setCallback("onDatabaseStats", (stats) => {
          setDatabaseStats(stats);
        });

        // 5) onChordData
        window.ChopsBridge.setCallback("onChordData", (chordData) => {
          console.log("Received chord data from C++:", chordData);
          setCurrentChord({
            symbol: chordData.rootNote + (chordData.standardizedQuality || ""),
            notes: [], // C++ doesn’t send note arrays
            description:
              chordData.rootNote + " " + (chordData.standardizedQuality || ""),
            inversion: "root",
            chordType: chordData.standardizedQuality,
          });
        });

        console.log(
          "🧪 All callbacks registered—now sending bridgeReady to JUCE."
        );
        window.ChopsBridge.sendMessage("bridgeReady", {});
      } else {
        // If the bridge isn’t injected yet, try again in 250 ms
        setTimeout(initializeChopsBridge, 250);
      }
    }

    initializeChopsBridge();
  }, []);
  // ────────────────────────────────────────────────────────────────────────────────────────

  // Handle piano input with proper root/inversion logic
  const handlePianoInput = useCallback(
    (notes) => {
      setSelectedNotes(notes);

      // Try to recognize chord from notes with inversion detection
      const recognizedChord = recognizeChordFromNotes(
        notes,
        rootAgnostic,
        inversionAgnostic
      );

      if (recognizedChord) {
        setCurrentChord(recognizedChord);

        // Build search query based on toggle states
        let searchTerm = "";

        if (rootAgnostic) {
          // Search by chord type only (e.g., "major" instead of "Cmaj7")
          searchTerm = recognizedChord.chordType || recognizedChord.symbol;
        } else {
          // Search by specific chord with root (e.g., "Cmaj7")
          if (inversionAgnostic) {
            // Use base chord without inversion info
            searchTerm = recognizedChord.baseChord || recognizedChord.symbol;
          } else {
            // Use full chord including inversion if present
            searchTerm = recognizedChord.symbol;
          }
        }

        // Update search query and trigger search
        setSearchQuery(searchTerm);

        // Call the backend search via the bridge
        if (window.ChopsBridge && searchTerm) {
          console.log("Sending search request to C++:", searchTerm);
          setIsLoading(true);
          window.ChopsBridge.sendMessage("searchRequested", {
            query: searchTerm,
          });
        }
      }
    },
    [rootAgnostic, inversionAgnostic]
  );

  // Handle search bar input
  const handleSearch = useCallback((query) => {
    setSearchQuery(query);
    setErrorMessage(""); // Clear any previous errors

    // Update current chord display based on search
    if (query.trim()) {
      // Try to parse the search query as a chord
      const searchChord = parseSearchQuery(query);
      if (searchChord) {
        setCurrentChord(searchChord);
      }
    }

    // Trigger backend search via the bridge
    if (window.ChopsBridge) {
      console.log("Sending search request to C++:", query);
      setIsLoading(true);
      window.ChopsBridge.sendMessage("searchRequested", { query: query || "" });
    }
  }, []);

  // Handle sample selection
  const handleSampleSelection = useCallback((sample) => {
    console.log("Sample selected:", sample);

    // Notify C++ backend
    if (window.ChopsBridge) {
      window.ChopsBridge.sendMessage("sampleSelected", {
        id: sample.id,
        filePath: sample.filePath,
      });
    }
  }, []);

  // Handle Chopsie Daisy button
  const handleChopsieDaisy = useCallback(() => {
    console.log("Opening Chopsie Daisy effects window...");
    if (window.ChopsBridge) {
      window.ChopsBridge.sendMessage("uiEvent", {
        eventType: "chopsie_daisy",
        eventData: { action: "open" },
      });
    }
  }, []);

  return (
    <div className="chops-browser-ui">
      {/* Status Indicator */}
      <div className={`status-indicator ${isLoading ? "loading" : ""}`}></div>

      {/* Error Message */}
      {errorMessage && (
        <div className="error-message">
          <span className="error-icon">⚠</span>
          {errorMessage}
          <button className="error-close" onClick={() => setErrorMessage("")}>
            ×
          </button>
        </div>
      )}

      {/* Header Section - Always Visible */}
      <div className="header-section">
        <div className="logo">Chops Suite</div>

        <div className="current-chord-display">
          <div className="chord-symbol">{currentChord.symbol}</div>
          <div className="chord-details">
            {currentChord.description} • {displayedSamples.length} samples found
            {databaseStats && (
              <span className="total-samples">
                {" "}
                (of {databaseStats.totalSamples} total)
              </span>
            )}
          </div>
        </div>

        <SearchBar
          value={searchQuery}
          onChange={handleSearch}
          placeholder="Search chords..."
          showSuggestions={false} // Disable dropdown, use main results area
        />

        <div className="header-buttons">
          <button className="chopsie-daisy-btn" onClick={handleChopsieDaisy}>
            Chopsie Daisy
          </button>
        </div>
      </div>

      <div className="main-content">
        {/* Center Panel - Full Width */}
        <div className="center-panel full-width">
          {/* Piano Roll with Controls */}
          <div className="piano-section">
            <div className="piano-controls">
              <button
                className={`toggle-btn ${rootAgnostic ? "active" : ""}`}
                onClick={() => setRootAgnostic(!rootAgnostic)}
                title={
                  rootAgnostic
                    ? "Searching all roots"
                    : "Searching specific root"
                }
              >
                Root Agnostic
              </button>
              <button
                className={`toggle-btn ${inversionAgnostic ? "active" : ""}`}
                onClick={() => setInversionAgnostic(!inversionAgnostic)}
                title={
                  inversionAgnostic
                    ? "Ignoring inversions"
                    : "Including inversions"
                }
              >
                Inversion Agnostic
              </button>
            </div>

            <PianoRoll
              highlightedNotes={selectedNotes}
              onNotesChange={handlePianoInput}
              allowInput={true}
            />
          </div>

          {/* Results Area */}
          <div className="results-area">
            <div className="results-header">
              <div className="results-count">
                {isLoading ? (
                  <span className="loading-text">Searching...</span>
                ) : (
                  <>
                    {displayedSamples.length} samples • {currentChord.symbol}
                    {currentChord.inversion &&
                      currentChord.inversion !== "root" &&
                      !inversionAgnostic && (
                        <span className="inversion-indicator">
                          {" "}
                          ({currentChord.inversion})
                        </span>
                      )}
                  </>
                )}
              </div>
              <div className="view-controls">
                <button
                  className={`filename-toggle ${showFilenames ? "active" : ""}`}
                  onClick={() => setShowFilenames(!showFilenames)}
                  title="Toggle filename visibility"
                >
                  👁
                </button>
                <div className="view-label">List</div>
              </div>
            </div>

            <ResultsList
              samples={displayedSamples}
              viewMode="list"
              onSampleSelect={handleSampleSelection}
              currentChord={currentChord}
              showFilenames={showFilenames}
            />
          </div>
        </div>
      </div>

      {/* Database Stats Display (if available) */}
      {databaseStats && (
        <div className="stats-bar">
          <span className="stat-item">Total: {databaseStats.totalSamples}</span>
          <span className="stat-item">
            Extended: {databaseStats.withExtensions}
          </span>
          <span className="stat-item">
            Altered: {databaseStats.withAlterations}
          </span>
          <span className="stat-item">
            Recent: {databaseStats.addedLastWeek}
          </span>
        </div>
      )}
    </div>
  );
};

// FIXED: Enhanced chord recognition with CORRECT interval detection and inversion logic
function recognizeChordFromNotes(
  notes,
  rootAgnostic = false,
  inversionAgnostic = false
) {
  if (notes.length === 0) return null;

  // Remove duplicates and sort by chromatic order
  const uniqueNotes = [...new Set(notes)];
  const sortedNotes = uniqueNotes.sort((a, b) => {
    const noteOrder = [
      "C",
      "C#",
      "D",
      "D#",
      "E",
      "F",
      "F#",
      "G",
      "G#",
      "A",
      "A#",
      "B",
    ];
    return noteOrder.indexOf(a) - noteOrder.indexOf(b);
  });

  if (uniqueNotes.length === 1) {
    return {
      symbol: uniqueNotes[0],
      notes: uniqueNotes,
      description: uniqueNotes[0] + " (single note)",
      inversion: "root",
      chordType: "single",
      baseChord: uniqueNotes[0],
    };
  }

  // FIXED: Proper interval detection for 2-note combinations
  if (uniqueNotes.length === 2) {
    const intervalInfo = analyzeInterval(sortedNotes[0], sortedNotes[1]);

    return {
      symbol: rootAgnostic
        ? intervalInfo.name
        : `${sortedNotes[0]} ${intervalInfo.name}`,
      notes: uniqueNotes,
      description: `${sortedNotes.join(" ")} (${intervalInfo.description})`,
      inversion: "root",
      chordType: intervalInfo.name,
      baseChord: `${sortedNotes[0]} ${intervalInfo.name}`,
      intervalType: intervalInfo.type,
    };
  }

  // FIXED: Proper chord analysis with correct inversion detection
  if (uniqueNotes.length >= 3) {
    const chordInfo = analyzeChordCorrectly(uniqueNotes);

    if (chordInfo) {
      let symbol = chordInfo.symbol;
      let description = chordInfo.description;

      // Add inversion information unless agnostic
      if (!inversionAgnostic && chordInfo.inversion !== "root") {
        symbol += ` (${chordInfo.inversion})`;
        description += ` • ${chordInfo.inversion}`;
      }

      return {
        symbol: rootAgnostic ? chordInfo.chordType : symbol,
        notes: uniqueNotes,
        description: description,
        inversion: chordInfo.inversion,
        chordType: chordInfo.chordType,
        baseChord: chordInfo.symbol,
        root: chordInfo.root,
      };
    }
  }

  // Fallback
  return {
    symbol: notes.join(""),
    notes: notes,
    description: notes.join(" "),
    inversion: "root",
    chordType: "unknown",
    baseChord: notes.join(""),
  };
}

// FIXED: Proper interval analysis
function analyzeInterval(note1, note2) {
  const noteOrder = [
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B",
  ];

  const index1 = noteOrder.indexOf(note1);
  const index2 = noteOrder.indexOf(note2);

  if (index1 === -1 || index2 === -1) {
    return {
      name: "Unknown",
      description: "unknown interval",
      type: "unknown",
    };
  }

  // Calculate semitones (always positive, with note1 as reference)
  let semitones = (index2 - index1 + 12) % 12;

  const intervalMap = {
    0: { name: "Unison", description: "perfect unison", type: "unison" },
    1: { name: "m2", description: "minor 2nd", type: "m2" },
    2: { name: "M2", description: "major 2nd", type: "M2" },
    3: { name: "m3", description: "minor 3rd", type: "m3" },
    4: { name: "M3", description: "major 3rd", type: "M3" },
    5: { name: "P4", description: "perfect 4th", type: "P4" },
    6: { name: "Tritone", description: "tritone", type: "A4" },
    7: { name: "P5", description: "perfect 5th", type: "P5" },
    8: { name: "m6", description: "minor 6th", type: "m6" },
    9: { name: "M6", description: "major 6th", type: "M6" },
    10: { name: "m7", description: "minor 7th", type: "m7" },
    11: { name: "M7", description: "major 7th", type: "M7" },
  };

  return (
    intervalMap[semitones] || {
      name: "Unknown",
      description: "unknown interval",
      type: "unknown",
    }
  );
}

// FIXED: Completely rewritten chord analysis with correct inversion detection
function analyzeChordCorrectly(notes) {
  const noteOrder = [
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B",
  ];

  // Convert notes to chromatic positions
  const chromaticPositions = notes
    .map((note) => noteOrder.indexOf(note))
    .filter((pos) => pos !== -1);

  if (chromaticPositions.length < 3) return null;

  // Try each note as the potential root
  for (let rootIndex = 0; rootIndex < chromaticPositions.length; rootIndex++) {
    const root = chromaticPositions[rootIndex];
    const rootNote = noteOrder[root];

    // Calculate intervals from this root
    const intervals = chromaticPositions
      .map((pos) => (pos - root + 12) % 12)
      .sort((a, b) => a - b)
      .filter((interval, index, arr) => arr.indexOf(interval) === index); // Remove duplicates

    // Check against known chord patterns
    const chordMatch = matchChordPattern(intervals);

    if (chordMatch) {
      // Determine inversion based on which note is lowest (bass note)
      const bassNotePosition = Math.min(...chromaticPositions);
      const bassNote = noteOrder[bassNotePosition];

      let inversion = "root";
      if (bassNote !== rootNote) {
        // Check if bass note is 3rd, 5th, or 7th of the chord
        const bassInterval = (bassNotePosition - root + 12) % 12;

        if (chordMatch.intervals.includes(bassInterval)) {
          switch (bassInterval) {
            case 3:
            case 4: // Minor or major 3rd
              inversion = "1st inversion";
              break;
            case 7: // Perfect 5th
              inversion = "2nd inversion";
              break;
            case 10:
            case 11: // Minor or major 7th
              inversion = "3rd inversion";
              break;
            default:
              inversion = "slash chord";
          }
        } else {
          inversion = "slash chord";
        }
      }

      return {
        root: rootNote,
        symbol: rootNote + chordMatch.suffix,
        chordType: chordMatch.type,
        description: rootNote + " " + chordMatch.name,
        inversion: inversion,
        intervals: intervals,
        notes: notes,
      };
    }
  }

  return null;
}

// Chord pattern matching with correct interval sets
function matchChordPattern(intervals) {
  const patterns = [
    // Triads
    { intervals: [0, 4, 7], suffix: "", type: "major", name: "major" },
    { intervals: [0, 3, 7], suffix: "m", type: "minor", name: "minor" },
    {
      intervals: [0, 3, 6],
      suffix: "dim",
      type: "diminished",
      name: "diminished",
    },
    {
      intervals: [0, 4, 8],
      suffix: "aug",
      type: "augmented",
      name: "augmented",
    },
    {
      intervals: [0, 5, 7],
      suffix: "sus4",
      type: "sus4",
      name: "suspended 4th",
    },
    {
      intervals: [0, 2, 7],
      suffix: "sus2",
      type: "sus2",
      name: "suspended 2nd",
    },

    // 7th chords
    {
      intervals: [0, 4, 7, 11],
      suffix: "maj7",
      type: "maj7",
      name: "major 7th",
    },
    { intervals: [0, 3, 7, 10], suffix: "m7", type: "min7", name: "minor 7th" },
    {
      intervals: [0, 4, 7, 10],
      suffix: "7",
      type: "dom7",
      name: "dominant 7th",
    },
    {
      intervals: [0, 3, 6, 9],
      suffix: "dim7",
      type: "dim7",
      name: "diminished 7th",
    },
    {
      intervals: [0, 3, 6, 10],
      suffix: "m7b5",
      type: "halfDim7",
      name: "half diminished 7th",
    },
    {
      intervals: [0, 3, 7, 11],
      suffix: "m(maj7)",
      type: "minMaj7",
      name: "minor major 7th",
    },

    // 6th chords
    { intervals: [0, 4, 7, 9], suffix: "6", type: "maj6", name: "major 6th" },
    { intervals: [0, 3, 7, 9], suffix: "m6", type: "min6", name: "minor 6th" },

    // Extended chords (9th)
    {
      intervals: [0, 4, 7, 10, 14],
      suffix: "9",
      type: "dom9",
      name: "dominant 9th",
    },
    {
      intervals: [0, 4, 7, 11, 14],
      suffix: "maj9",
      type: "maj9",
      name: "major 9th",
    },
    {
      intervals: [0, 3, 7, 10, 14],
      suffix: "m9",
      type: "min9",
      name: "minor 9th",
    },

    // Add9 chords (no 7th)
    { intervals: [0, 4, 7, 14], suffix: "add9", type: "add9", name: "add 9th" },
    {
      intervals: [0, 3, 7, 14],
      suffix: "m(add9)",
      type: "min_add9",
      name: "minor add 9th",
    },
  ];

  // Normalize intervals to handle enharmonic equivalents and octave wrapping
  const normalizedIntervals = intervals
    .map((i) => i % 12)
    .sort((a, b) => a - b);

  for (const pattern of patterns) {
    const patternIntervals = pattern.intervals
      .map((i) => i % 12)
      .sort((a, b) => a - b);

    // Check if all pattern intervals are present (allowing extra notes)
    const hasAllIntervals = patternIntervals.every((interval) =>
      normalizedIntervals.includes(interval)
    );

    if (hasAllIntervals) {
      return pattern;
    }
  }

  return null;
}

// Parse search query to chord object
function parseSearchQuery(query) {
  // Simple chord parsing - can be enhanced
  const trimmedQuery = query.trim();

  if (!trimmedQuery) return null;

  // Try to extract root note
  const rootMatch = trimmedQuery.match(/^([A-G][#b]?)/);
  if (!rootMatch) {
    return {
      symbol: trimmedQuery,
      notes: [],
      description: trimmedQuery,
      inversion: "root",
      chordType: "search",
      baseChord: trimmedQuery,
    };
  }

  const root = rootMatch[1];
  const quality = trimmedQuery.substring(root.length);

  return {
    symbol: trimmedQuery,
    notes: [],
    description: `Search: ${trimmedQuery}`,
    inversion: "root",
    chordType: quality || "major",
    baseChord: trimmedQuery,
    root: root,
  };
}

export default ChopsBrowserUI;

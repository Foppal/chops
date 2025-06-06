// ChordTaxonomy.js - Core chord theory logic and UI filtering
// This implements the recommendations: standardized naming, proper categorization, progressive filtering

// === CHORD TAXONOMY IMPLEMENTATION ===
export const ChordFinderTaxonomy = {
  // Standardized root notes with consistent sharp preference
  rootNotes: ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"],
  
  // Type categories (Intervals OR Chords)
  typeCategories: {
    interval: {
      name: "Interval",
      description: "Two-note relationship",
      workflow: "simplified",
      options: {
        unison: { symbol: "P1", semitones: 0, description: "Perfect Unison" },
        minor2nd: { symbol: "m2", semitones: 1, description: "Minor 2nd" },
        major2nd: { symbol: "M2", semitones: 2, description: "Major 2nd" },
        minor3rd: { symbol: "m3", semitones: 3, description: "Minor 3rd" },
        major3rd: { symbol: "M3", semitones: 4, description: "Major 3rd" },
        perfect4th: { symbol: "P4", semitones: 5, description: "Perfect 4th" },
        tritone: { symbol: "A4", semitones: 6, description: "Tritone" },
        perfect5th: { symbol: "P5", semitones: 7, description: "Perfect 5th" },
        minor6th: { symbol: "m6", semitones: 8, description: "Minor 6th" },
        major6th: { symbol: "M6", semitones: 9, description: "Major 6th" },
        minor7th: { symbol: "m7", semitones: 10, description: "Minor 7th" },
        major7th: { symbol: "M7", semitones: 11, description: "Major 7th" },
        octave: { symbol: "P8", semitones: 12, description: "Perfect Octave" }
      }
    },
    
    chord: {
      name: "Chord",
      description: "Three or more notes",
      workflow: "full",
      
      // IMPROVED: Base triad qualities following proper categorization
      triadQualities: {
        major: {
          symbol: "",
          intervals: ["1", "3", "5"],
          displayName: "Major",
          availableExtensions: ["none", "6th", "7th", "extended", "added"],
          isDefault: true
        },
        minor: {
          symbol: "m",
          intervals: ["1", "b3", "5"], 
          displayName: "Minor",
          availableExtensions: ["none", "6th", "7th", "extended", "added"]
        },
        augmented: {
          symbol: "aug",
          intervals: ["1", "3", "#5"],
          displayName: "Augmented", 
          availableExtensions: ["none", "7th", "added"],
          advancedOnly: true
        },
        diminished: {
          symbol: "dim",
          intervals: ["1", "b3", "b5"],
          displayName: "Diminished",
          availableExtensions: ["none", "7th"],
          advancedOnly: true
        },
        suspended4: {
          symbol: "sus4",
          intervals: ["1", "4", "5"],
          displayName: "Suspended 4th",
          availableExtensions: ["none", "7th", "extended", "added"]
        },
        suspended2: {
          symbol: "sus2",
          intervals: ["1", "2", "5"],
          displayName: "Suspended 2nd", 
          availableExtensions: ["none", "7th", "extended", "added"],
          advancedOnly: true
        }
      }
    }
  },

  // Simple vs Advanced mode configuration
  modes: {
    simple: {
      name: "Simple",
      description: "Common chords only",
      availableTriads: ["major", "minor", "suspended4"],
      availableExtensions: ["none", "7th", "added"],
      available7ths: ["dominant7", "major7", "minor7"],
      availableAdded: ["add9"],
      availableIntervals: ["perfect5th", "major3rd", "perfect4th", "minor7th", "major7th"]
    },
    
    advanced: {
      name: "Advanced",
      description: "Full chord vocabulary", 
      // "all" means no filtering
      availableTriads: "all",
      availableExtensions: "all",
      available7ths: "all",
      availableAdded: "all",
      availableIntervals: "all"
    }
  },

  // IMPROVED: Extension categories with proper hierarchy (Quality → Extensions → Alterations)
  extensionCategories: {
    none: {
      name: "Triad Only",
      description: "Just the basic triad",
      result: "triad"
    },
    
    "6th": {
      name: "6th Chords",
      description: "Add the 6th",
      availableFor: ["major", "minor"],
      options: {
        sixth: { 
          symbol: "6", 
          intervals: ["6"],
          description: "Major or minor 6th chord"
        }
      }
    },
    
    "7th": {
      name: "7th Chords", 
      description: "Add a 7th",
      options: {
        // IMPROVED: Proper defaults and availability
        dominant7: {
          symbol: "7",
          intervals: ["b7"],
          displayName: "Dominant 7th",
          availableFor: ["major", "suspended4", "suspended2"],
          isDefault: true // Show first in lists
        },
        major7: {
          symbol: "maj7", 
          intervals: ["7"],
          displayName: "Major 7th",
          availableFor: ["major"]
        },
        minor7: {
          symbol: "m7",
          intervals: ["b7"],
          displayName: "Minor 7th", 
          availableFor: ["minor"]
        },
        minorMajor7: {
          symbol: "m(maj7)",
          intervals: ["7"],
          displayName: "Minor Major 7th",
          availableFor: ["minor"],
          advancedOnly: true
        },
        diminished7: {
          symbol: "dim7",
          intervals: ["bb7"],
          displayName: "Diminished 7th",
          availableFor: ["diminished"],
          advancedOnly: true
        },
        halfDiminished7: {
          symbol: "m7b5",
          intervals: ["b7"],
          displayName: "Half Diminished 7th",
          availableFor: ["diminished"]
        },
        augmented7: {
          symbol: "aug7", 
          intervals: ["b7"],
          displayName: "Augmented 7th",
          availableFor: ["augmented"],
          advancedOnly: true
        },
        augmentedMajor7: {
          symbol: "maj7#5",
          intervals: ["7"],
          displayName: "Augmented Major 7th",
          availableFor: ["augmented"],
          advancedOnly: true
        }
      }
    },
    
    extended: {
      name: "Extended Chords",
      description: "9th, 11th, 13th (imply lower extensions)",
      availableFor: ["major", "minor", "suspended4", "suspended2"],
      advancedOnly: true,
      options: {
        ninth: {
          baseIntervals: ["b7", "9"], // Implies 7th + 9th
          displayName: "9th Chord",
          availableFor: ["major", "minor"],
          // IMPROVED: Multiple variants with proper defaults (your requirement #2)
          variants: {
            dominant: { 
              symbol: "9", 
              baseChord: "dom7",
              isDefault: true // Default shown first
            },
            major: { 
              symbol: "maj9", 
              baseChord: "maj7" 
            },
            minor: { 
              symbol: "m9", 
              baseChord: "min7" 
            }
          }
        },
        eleventh: {
          baseIntervals: ["b7", "9", "11"], // Implies 7th + 9th + 11th
          displayName: "11th Chord", 
          availableFor: ["major", "minor", "suspended4"],
          variants: {
            dominant: { 
              symbol: "11", 
              baseChord: "dom7",
              isDefault: true 
            },
            major: { 
              symbol: "maj11", 
              baseChord: "maj7" 
            },
            minor: { 
              symbol: "m11", 
              baseChord: "min7" 
            }
          }
        },
        thirteenth: {
          baseIntervals: ["b7", "9", "11", "13"], // Implies all lower extensions
          displayName: "13th Chord",
          availableFor: ["major", "minor"],
          variants: {
            dominant: { 
              symbol: "13", 
              baseChord: "dom7",
              isDefault: true 
            },
            major: { 
              symbol: "maj13", 
              baseChord: "maj7" 
            },
            minor: { 
              symbol: "m13", 
              baseChord: "min7" 
            }
          }
        }
      }
    },
    
    added: {
      name: "Added Notes",
      description: "Add specific notes without implying others", 
      availableFor: ["major", "minor", "augmented", "suspended4", "suspended2"],
      options: {
        add9: { 
          symbol: "add9", 
          intervals: ["9"],
          description: "Add 9th without 7th",
          isDefault: true 
        },
        add11: { 
          symbol: "add11", 
          intervals: ["11"],
          description: "Add 11th without 7th or 9th",
          advancedOnly: true 
        },
        add13: { 
          symbol: "add13", 
          intervals: ["13"],
          description: "Add 13th without other extensions",
          advancedOnly: true 
        },
        add2: { 
          symbol: "add2", 
          intervals: ["2"],
          description: "Add 2nd (same as add9 but lower octave)",
          advancedOnly: true 
        },
        add4: { 
          symbol: "add4", 
          intervals: ["4"],
          description: "Add 4th without suspension", 
          advancedOnly: true 
        }
      }
    }
  },

  // IMPROVED: Alterations for advanced chord building (separate categories for altered chords)
  alterations: {
    fifth: {
      name: "5th Alterations",
      options: {
        sharp5: { symbol: "#5", interval: "#5", description: "Raised 5th" },
        flat5: { symbol: "b5", interval: "b5", description: "Lowered 5th" }
      }
    },
    ninth: {
      name: "9th Alterations", 
      availableFor: ["extended", "added"],
      options: {
        sharp9: { symbol: "#9", interval: "#9", description: "Raised 9th" },
        flat9: { symbol: "b9", interval: "b9", description: "Lowered 9th" }
      }
    },
    eleventh: {
      name: "11th Alterations",
      options: {
        sharp11: { symbol: "#11", interval: "#11", description: "Raised 11th" }
      }
    },
    thirteenth: {
      name: "13th Alterations",
      options: {
        flat13: { symbol: "b13", interval: "b13", description: "Lowered 13th" }
      }
    }
  },

  // Inversion options
  inversions: {
    root: { symbol: "", description: "Root position" },
    first: { symbol: "/3", description: "1st inversion (3rd in bass)" },
    second: { symbol: "/5", description: "2nd inversion (5th in bass)" },
    third: { symbol: "/7", description: "3rd inversion (7th in bass)", availableFor: ["7th", "extended"] },
    slash: { symbol: "/X", description: "Slash chord (specify bass note)" }
  }
};

// === UI HELPER FUNCTIONS ===
export const ChordFinderUI = {
  
  // Filter options based on Simple vs Advanced mode
  filterByMode(options, mode, category) {
    if (mode === "simple") {
      const simpleConfig = ChordFinderTaxonomy.modes.simple;
      
      switch(category) {
        case "triads":
          if (simpleConfig.availableTriads === "all") return options;
          return Object.fromEntries(
            Object.entries(options).filter(([key]) => 
              simpleConfig.availableTriads.includes(key)
            )
          );
          
        case "intervals": 
          if (simpleConfig.availableIntervals === "all") return options;
          return Object.fromEntries(
            Object.entries(options).filter(([key]) => 
              simpleConfig.availableIntervals.includes(key)
            )
          );
          
        case "extensions":
          if (simpleConfig.availableExtensions === "all") return options;
          return simpleConfig.availableExtensions;
          
        default:
          // Filter out advancedOnly items
          return Object.fromEntries(
            Object.entries(options).filter(([key, option]) => 
              !option.advancedOnly
            )
          );
      }
    }
    return options; // Advanced mode shows everything
  },

  // Get available extensions based on triad choice
  getAvailableExtensions(triadQuality) {
    const triad = ChordFinderTaxonomy.typeCategories.chord.triadQualities[triadQuality];
    return triad ? triad.availableExtensions : [];
  },

  // Get available 7th types based on triad
  getAvailable7thTypes(triadQuality) {
    const seventhOptions = ChordFinderTaxonomy.extensionCategories["7th"].options;
    return Object.entries(seventhOptions)
      .filter(([key, option]) => option.availableFor.includes(triadQuality))
      .map(([key, option]) => ({ key, ...option }));
  },

  // Get available extended chord types  
  getAvailableExtendedTypes(triadQuality) {
    const extendedOptions = ChordFinderTaxonomy.extensionCategories.extended.options;
    return Object.entries(extendedOptions)
      .filter(([key, option]) => option.availableFor.includes(triadQuality))
      .map(([key, option]) => ({ key, ...option }));
  },

  // IMPROVED: Build chord symbol with proper variant support
  buildChordSymbol(selections) {
    if (selections.type === "interval") {
      const intervalData = ChordFinderTaxonomy.typeCategories.interval.options[selections.intervalType];
      return `${selections.root} ${intervalData?.symbol || ""}`;
    }
    
    let symbol = selections.root || "";
    
    // Add base triad quality (skip for major)
    if (selections.triad && selections.triad !== "major") {
      const triadData = ChordFinderTaxonomy.typeCategories.chord.triadQualities[selections.triad];
      symbol += triadData?.symbol || "";
    }
    
    // Handle extensions with proper variant support (your requirement #2)
    if (selections.extensionCategory === "extended" && selections.specificExtension) {
      const extendedOption = ChordFinderTaxonomy.extensionCategories.extended.options[selections.specificExtension];
      if (extendedOption?.variants) {
        const variant = selections.extensionVariant || Object.keys(extendedOption.variants).find(
          key => extendedOption.variants[key].isDefault
        ) || Object.keys(extendedOption.variants)[0];
        
        // Replace the root and rebuild with variant symbol
        symbol = selections.root + extendedOption.variants[variant].symbol;
      }
    } else if (selections.extensionCategory && selections.specificExtension) {
      // Handle regular extensions
      const categoryData = ChordFinderTaxonomy.extensionCategories[selections.extensionCategory];
      const extensionData = categoryData?.options?.[selections.specificExtension];
      if (extensionData?.symbol) {
        symbol += extensionData.symbol;
      }
    }
    
    // Add alterations
    if (selections.alterations && selections.alterations.length > 0) {
      selections.alterations.forEach(alteration => {
        symbol += alteration;
      });
    }
    
    // Add inversion/slash bass
    if (selections.inversion && selections.inversion !== "root") {
      const inversionData = ChordFinderTaxonomy.inversions[selections.inversion];
      if (inversionData?.symbol) {
        symbol += inversionData.symbol;
      }
    }
    
    return symbol;
  },

  // IMPROVED: Generate standardized filename following Cmaj7add9 format
  generateStandardizedFilename(selections, originalExtension = ".wav") {
    const chordSymbol = this.buildChordSymbol(selections);
    
    // Sanitize for filename (keep # and b as they're essential for music)
    let filename = chordSymbol
      .replace(/[<>:"|?*\/\\]/g, "_") // Only replace truly problematic characters
      .replace(/\s+/g, "_") // Replace spaces with underscores
      .trim();
    
    return filename + originalExtension;
  },

  // Generate folder path with proper categorization for altered chords
  generateFolderPath(selections) {
    let basePath = "";
    
    if (selections.type === "interval") {
      basePath = "intervals";
    } else {
      // Use standardized chord type for folder
      const triad = selections.triad || "major";
      const extension = selections.extensionCategory || "none";
      
      if (extension === "none") {
        basePath = `triads/${triad}`;
      } else if (extension === "7th") {
        basePath = `seventh_chords/${triad}`;
      } else if (extension === "extended") {
        basePath = `extended_chords/${triad}`;
      } else if (extension === "added") {
        basePath = `added_note_chords/${triad}`;
      }
    }
    
    // IMPROVED: Separate altered chords into subfolders (your categorization requirement)
    const hasAlterations = selections.alterations && selections.alterations.length > 0;
    if (hasAlterations) {
      basePath += "/altered";
    }
    
    return basePath;
  }
};

// === CHORD RECOGNITION ===
export const ChordRecognition = {
  
  // Recognize chord from MIDI note array
  recognizeChord(midiNotes) {
    if (!midiNotes || midiNotes.length < 2) return null;
    
    // Convert MIDI to note names
    const noteNames = midiNotes.map(midi => this.midiToNoteName(midi));
    
    // Remove duplicates and sort
    const uniqueNotes = [...new Set(noteNames)].sort();
    
    if (uniqueNotes.length === 2) {
      return this.recognizeInterval(uniqueNotes);
    } else if (uniqueNotes.length >= 3) {
      return this.recognizeChordFromNotes(uniqueNotes);
    }
    
    return null;
  },

  // Convert MIDI note number to note name (C4 = 60)
  midiToNoteName(midiNote) {
    const notes = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"];
    return notes[midiNote % 12];
  },

  // Recognize interval from two notes
  recognizeInterval(notes) {
    // Simple interval recognition logic
    const root = notes[0];
    const interval = notes[1];
    
    // Calculate semitone distance
    const rootIndex = ChordFinderTaxonomy.rootNotes.indexOf(root);
    const intervalIndex = ChordFinderTaxonomy.rootNotes.indexOf(interval);
    const semitones = (intervalIndex - rootIndex + 12) % 12;
    
    // Find matching interval
    const intervalType = Object.entries(ChordFinderTaxonomy.typeCategories.interval.options)
      .find(([key, data]) => data.semitones === semitones);
    
    if (intervalType) {
      return {
        type: "interval",
        root,
        intervalType: intervalType[0],
        symbol: `${root} ${intervalType[1].symbol}`,
        notes
      };
    }
    
    return null;
  },

  // Recognize chord from note array
  recognizeChordFromNotes(notes) {
    // This is a simplified implementation
    // A full implementation would analyze intervals and match against known chord patterns
    
    const root = notes[0]; // Assume first note is root
    
    // Basic chord recognition patterns
    if (notes.length === 3) {
      // Triad recognition
      if (this.hasInterval(notes, root, "M3") && this.hasInterval(notes, root, "P5")) {
        return { type: "chord", root, triad: "major", symbol: root };
      }
      if (this.hasInterval(notes, root, "m3") && this.hasInterval(notes, root, "P5")) {
        return { type: "chord", root, triad: "minor", symbol: root + "m" };
      }
    } else if (notes.length === 4) {
      // 7th chord recognition
      if (this.hasInterval(notes, root, "M3") && this.hasInterval(notes, root, "P5") && this.hasInterval(notes, root, "M7")) {
        return { type: "chord", root, triad: "major", extensionCategory: "7th", specificExtension: "major7", symbol: root + "maj7" };
      }
      if (this.hasInterval(notes, root, "M3") && this.hasInterval(notes, root, "P5") && this.hasInterval(notes, root, "m7")) {
        return { type: "chord", root, triad: "major", extensionCategory: "7th", specificExtension: "dominant7", symbol: root + "7" };
      }
    }
    
    return null;
  },

  // Check if note array contains specific interval from root
  hasInterval(notes, root, interval) {
    const targetNote = this.getIntervalNote(root, interval);
    return notes.includes(targetNote);
  },

  // Calculate note at specific interval from root
  getIntervalNote(root, interval) {
    // Simplified interval calculation
    const intervalMap = {
      "m3": { "C": "Eb", "D": "F", "E": "G", "F": "Ab", "G": "Bb", "A": "C", "B": "D" },
      "M3": { "C": "E", "D": "F#", "E": "G#", "F": "A", "G": "B", "A": "C#", "B": "D#" },
      "P5": { "C": "G", "D": "A", "E": "B", "F": "C", "G": "D", "A": "E", "B": "F#" },
      "m7": { "C": "Bb", "D": "C", "E": "D", "F": "Eb", "G": "F", "A": "G", "B": "A" },
      "M7": { "C": "B", "D": "C#", "E": "D#", "F": "E", "G": "F#", "A": "G#", "B": "A#" }
    };
    
    return intervalMap[interval]?.[root] || root;
  }
};

export default { ChordFinderTaxonomy, ChordFinderUI, ChordRecognition };
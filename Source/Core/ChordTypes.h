#pragma once

#include <JuceHeader.h>
#include <unordered_map>
#include <vector>

namespace ChordTypes
{
    // Chord type structure
    struct ChordType
    {
        juce::String key;
        juce::StringArray intervals;
        juce::String displayName;
        juce::String family;
        int complexity;
        juce::String symbol; // Added for compatibility
    };
    
    // Chord alias structure
    struct ChordAlias
    {
        juce::String standardizedKey;
        juce::StringArray impliedExtensions;
        juce::StringArray impliedAlterations;
    };
    
    // Initialize chord type definitions
    inline std::unordered_map<juce::String, ChordType> getStandardizedChordTypes()
    {
        std::unordered_map<juce::String, ChordType> types;
        
        // Triads
        types["maj"] = {"maj", {"1", "3", "5"}, "Major", "major", 2, "maj"};
        types["min"] = {"min", {"1", "b3", "5"}, "Minor", "minor", 2, "min"};
        types["aug"] = {"aug", {"1", "3", "#5"}, "Augmented", "augmented", 2, "aug"};
        types["dim"] = {"dim", {"1", "b3", "b5"}, "Diminished", "diminished", 2, "dim"};
        types["sus4"] = {"sus4", {"1", "4", "5"}, "Suspended 4", "suspended", 2, "sus4"};
        types["sus2"] = {"sus2", {"1", "2", "5"}, "Suspended 2", "suspended", 2, "sus2"};
        types["flat5"] = {"flat5", {"1", "3", "b5"}, "Flat 5", "major", 2, "b5"};
        
        // 7th chords
        types["maj7"] = {"maj7", {"1", "3", "5", "7"}, "Major 7", "major", 3, "maj7"};
        types["min7"] = {"min7", {"1", "b3", "5", "b7"}, "Minor 7", "minor", 3, "m7"};
        types["dom7"] = {"dom7", {"1", "3", "5", "b7"}, "Dominant 7", "dominant", 3, "7"};
        types["dim7"] = {"dim7", {"1", "b3", "b5", "bb7"}, "Diminished 7", "diminished", 3, "dim7"};
        types["halfDim7"] = {"halfDim7", {"1", "b3", "b5", "b7"}, "Half Diminished 7", "diminished", 3, "m7b5"};
        types["aug7"] = {"aug7", {"1", "3", "#5", "b7"}, "Augmented 7", "augmented", 3, "aug7"};
        types["minMaj7"] = {"minMaj7", {"1", "b3", "5", "7"}, "Minor Major 7", "minor", 3, "m(maj7)"};
        types["augMaj7"] = {"augMaj7", {"1", "3", "#5", "7"}, "Augmented Major 7", "augmented", 3, "maj7#5"};
        
        // 6th chords
        types["maj6"] = {"maj6", {"1", "3", "5", "6"}, "Major 6", "major", 3, "6"};
        types["min6"] = {"min6", {"1", "b3", "5", "6"}, "Minor 6", "minor", 3, "m6"};
        // Special 6th chord variations
        types["6b5"] = {"6b5", {"1", "3", "b5", "6"}, "6 Flat 5", "major", 3, "6b5"};
        types["aug6"] = {"aug6", {"1", "3", "#5", "6"}, "Augmented 6", "augmented", 3, "aug6"};
        
        // Extended chords
        types["maj9"] = {"maj9", {"1", "3", "5", "7", "9"}, "Major 9", "major", 4, "maj9"};
        types["min9"] = {"min9", {"1", "b3", "5", "b7", "9"}, "Minor 9", "minor", 4, "m9"};
        types["dom9"] = {"dom9", {"1", "3", "5", "b7", "9"}, "Dominant 9", "dominant", 4, "9"};

        types["maj11"] = {"maj11", {"1", "3", "5", "7", "9", "11"}, "Major 11", "major", 5, "maj11"};
        types["min11"] = {"min11", {"1", "b3", "5", "b7", "9", "11"}, "Minor 11", "minor", 5, "m11"};
        types["dom11"] = {"dom11", {"1", "3", "5", "b7", "9", "11"}, "Dominant 11", "dominant", 5, "11"};

        types["maj13"] = {"maj13", {"1", "3", "5", "7", "9", "11", "13"}, "Major 13", "major", 6, "maj13"};
        types["min13"] = {"min13", {"1", "b3", "5", "b7", "9", "11", "13"}, "Minor 13", "minor", 6, "m13"};
        types["dom13"] = {"dom13", {"1", "3", "5", "b7", "9", "11", "13"}, "Dominant 13", "dominant", 6, "13"};

        types["dim9"] = {"dim9", {"1", "b3", "b5", "bb7", "9"}, "Diminished 9", "diminished", 4, "dim9"};
        types["dim11"] = {"dim11", {"1", "b3", "b5", "bb7", "9", "11"}, "Diminished 11", "diminished", 5, "dim11"};
        
        // Intervals
        types["interval_m2"] = {"interval_m2", {"1", "b2"}, "Minor 2nd", "interval", 1, "m2"};
        types["interval_M2"] = {"interval_M2", {"1", "2"}, "Major 2nd", "interval", 1, "M2"};
        types["interval_m3"] = {"interval_m3", {"1", "b3"}, "Minor 3rd", "interval", 1, "m3"};
        types["interval_M3"] = {"interval_M3", {"1", "3"}, "Major 3rd", "interval", 1, "M3"};
        types["interval_P4"] = {"interval_P4", {"1", "4"}, "Perfect 4th", "interval", 1, "P4"};
        types["interval_A4"] = {"interval_A4", {"1", "#4"}, "Augmented 4th", "interval", 1, "A4"};
        types["interval_d5"] = {"interval_d5", {"1", "b5"}, "Diminished 5th", "interval", 1, "d5"};
        types["interval_P5"] = {"interval_P5", {"1", "5"}, "Perfect 5th", "interval", 1, "P5"};
        types["interval_A5"] = {"interval_A5", {"1", "#5"}, "Augmented 5th", "interval", 1, "A5"};
        types["interval_m6"] = {"interval_m6", {"1", "b6"}, "Minor 6th", "interval", 1, "m6"};
        types["interval_M6"] = {"interval_M6", {"1", "6"}, "Major 6th", "interval", 1, "M6"};
        types["interval_m7"] = {"interval_m7", {"1", "b7"}, "Minor 7th", "interval", 1, "m7"};
        types["interval_M7"] = {"interval_M7", {"1", "7"}, "Major 7th", "interval", 1, "M7"};
        types["interval_P8"] = {"interval_P8", {"1", "8"}, "Perfect 8th", "interval", 1, "P8"};
        
        return types;
    }
    
    // Get chord type by key
    inline ChordType getChordType(const juce::String& key)
    {
        auto types = getStandardizedChordTypes();
        auto it = types.find(key.toStdString());
        if (it != types.end())
            return it->second;
        
        // Return empty chord type if not found
        return ChordType{"", {}, "", "", 0, ""};
    }
    
    // Get interval note (placeholder implementation)
    inline juce::String getIntervalNote(const juce::String& rootNote, const juce::String& interval)
    {
        // This is a simplified implementation
        // In a full implementation, you'd calculate the actual note based on the interval
        juce::ignoreUnused(rootNote, interval);
        return rootNote; // Placeholder
    }
    
    // Get inversion from bass note (placeholder implementation)
    inline int getInversionFromBassNote(const juce::String& rootNote, const juce::String& bassNote, const ChordType& chordType)
    {
        // This is a simplified implementation
        // In a full implementation, you'd determine which inversion has the bass note in the bass
        juce::ignoreUnused(rootNote, bassNote, chordType);
        return 0; // Placeholder
    }
    
    // Get chord aliases map
    inline std::unordered_map<juce::String, ChordAlias> getChordAliases()
    {
        std::unordered_map<juce::String, ChordAlias> aliases;
        
        // Basic triads
        aliases["major"] = {"maj", {}, {}};
        aliases["minor"] = {"min", {}, {}};
        aliases["diminished"] = {"dim", {}, {}};
        aliases["augmented"] = {"aug", {}, {}};
        
        // 7th chord aliases
        aliases["dominant"] = {"dom7", {}, {}};
        aliases["dominant7"] = {"dom7", {}, {}};
        aliases["major7"] = {"maj7", {}, {}};
        aliases["minor7"] = {"min7", {}, {}};
        aliases["diminished7"] = {"dim7", {}, {}};
        aliases["halfdim"] = {"halfDim7", {}, {}};
        aliases["halfdiminished"] = {"halfDim7", {}, {}};
        aliases["augmented7"] = {"aug7", {}, {}};
        aliases["minormajor7"] = {"minMaj7", {}, {}};
        
        // Sus chord aliases
        aliases["suspended4"] = {"sus4", {}, {}};
        aliases["suspended2"] = {"sus2", {}, {}};
        aliases["suspension4"] = {"sus4", {}, {}};
        aliases["suspension2"] = {"sus2", {}, {}};
        
        // 6th chord aliases
        aliases["sixth"] = {"maj6", {}, {}};
        aliases["major6"] = {"maj6", {}, {}};
        aliases["minor6"] = {"min6", {}, {}};
        
        // Extended chord aliases
        aliases["ninth"] = {"dom9", {}, {}};
        aliases["eleventh"] = {"dom11", {}, {}};
        aliases["thirteenth"] = {"dom13", {}, {}};
        
        return aliases;
    }
    
    // Quality map for filename generation
    inline std::unordered_map<juce::String, juce::String> getQualityDisplayMap()
    {
        return {
            {"maj", ""},
            {"min", "m"},
            {"aug", "aug"},
            {"dim", "dim"},
            {"sus4", "sus4"},
            {"sus2", "sus2"},
            {"flat5", "b5"},
            {"maj7", "maj7"},
            {"min7", "m7"},
            {"dom7", "7"},
            {"dim7", "dim7"},
            {"halfDim7", "m7b5"},
            {"aug7", "aug7"},
            {"minMaj7", "m(maj7)"},
            {"augMaj7", "maj7#5"},
            {"maj6", "6"},
            {"min6", "m6"},
            {"maj9", "maj9"},
            {"min9", "m9"},
            {"dom9", "9"},
            {"maj11", "maj11"},
            {"min11", "m11"},
            {"dom11", "11"},
            {"maj13", "maj13"},
            {"min13", "m13"},
            {"dom13", "13"},
            {"dim9", "dim9"},
            {"dim11", "dim11"},
            {"6b5", "6b5"},
            {"aug6", "aug6"},
            
            // Interval display mappings
            {"interval_P1", " P1"},
            {"interval_m2", " m2"},
            {"interval_M2", " M2"},
            {"interval_m3", " m3"},
            {"interval_M3", " M3"},
            {"interval_P4", " P4"},
            {"interval_A4", " A4"},
            {"interval_d5", " d5"},
            {"interval_P5", " P5"},
            {"interval_A5", " A5"},
            {"interval_m6", " m6"},
            {"interval_M6", " M6"},
            {"interval_m7", " m7"},
            {"interval_M7", " M7"},
            {"interval_P8", " P8"}
        };
    }
    
    // Helper function to sanitize chord folder names
    inline juce::String sanitizeChordFolderName(const juce::String& standardizedKey)
    {
        if (standardizedKey.isEmpty())
            return "unknown_chord";
        
        juce::String result;
        for (int i = 0; i < standardizedKey.length(); ++i)
        {
            juce_wchar c = standardizedKey[i];
            if (juce::CharacterFunctions::isLetterOrDigit(c))
                result += juce::CharacterFunctions::toLowerCase(c);
            else if (c == '#')
                result += "sharp";
            else if (c == 'b')
                result += "flat";
        }
        
        return result.isEmpty() ? "unknown_chord" : result;
    }
}
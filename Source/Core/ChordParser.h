#pragma once

#include <JuceHeader.h>
#include "ChordTypes.h"
#include <regex>
#include <unordered_map>
#include <vector>

class ChordParser
{
public:
    ChordParser();
    ~ChordParser() = default;
    
    // Parsed data structure
    struct ParsedData
    {
        // Original input
        juce::String originalFilename;
        juce::String originalExtension;
        
        // Parsed components
        juce::String cleanedBasename;
        juce::String qualityDescriptorString;
        juce::String specificChordNotationFull;
        juce::String inversionText;
        
        // Chord components
        juce::String rootNote;
        juce::String standardizedQuality;
        juce::StringArray extensions;
        juce::StringArray alterations;
        juce::StringArray addedNotes;
        juce::StringArray suspensions;
        
        // Bass and inversion
        juce::String bassNoteSlash;
        juce::String determinedBassNote;
        juce::String inversionTextParsed;
        
        // Issues/warnings
        juce::StringArray issues;
        
        // Helper methods
        juce::String getFullChordName() const;
        juce::String getInversionSuffix() const;
    };
    
    // Main parsing method
    ParsedData parseFilename(const juce::String& filename);
    
private:
    // Regex patterns
    std::regex rootNotePattern;
    std::regex extensionPattern;
    std::regex alterationPattern;
    std::regex addPattern;
    std::regex bassNotePattern;
    std::regex inversionTextPattern;
    
    // Quality symbols map
    std::unordered_map<juce::String, std::pair<juce::String, juce::StringArray>> qualitySymbolsMap;
    
    // Initialization
    void initializeQualitySymbols();
    
    // Core parsing methods
    juce::String extractRootNote(const juce::String& str);
    void extractExtensionsAndAlterations(const juce::String& str, ParsedData& data);
    void parseInversionText(const juce::String& text, ParsedData& data);
    
    // Specialized parsing methods
    ParsedData parseInterval(const juce::String& str);
    bool parseFromDescriptor(const juce::String& descriptor, ParsedData& data);
    
    // Helper methods
    bool isChordProgression(const juce::String& str) const;
    bool isInterval(const juce::String& str) const;
    bool isInversionIndicator(const juce::String& str) const;
    void validateAndCleanup(ParsedData& data);
    juce::String normalizeForParsing(const juce::String& str) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordParser)
};
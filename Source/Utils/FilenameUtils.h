#pragma once

#include <JuceHeader.h>
#include "../Core/ChordParser.h"

namespace FilenameUtils
{
    // Validation functions
    bool isValidParsedData(const ChordParser::ParsedData& parsedData);
    
    // Filename generation
    juce::String generateNewSampleFilename(const ChordParser::ParsedData& parsedData, const juce::String& originalExtension);
    juce::String sanitizeFilename(const juce::String& filename);
    juce::String getQualityDisplayString(const ChordParser::ParsedData& parsedData);
    
    // Utility functions
    bool isAudioFile(const juce::File& file);
    juce::String normalizeRootNote(const juce::String& rootNote);
    juce::String getChordFolderName(const juce::String& standardizedQuality);
    bool validateFilename(const juce::String& filename);
    
    // File management
    juce::Array<juce::File> getAllAudioFiles(const juce::File& directory, bool recursive = false);
    juce::String createUniqueFilename(const juce::File& directory, const juce::String& desiredName);
}
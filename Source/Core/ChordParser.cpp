#include "ChordParser.h"

ChordParser::ChordParser()
{
   // Initialize regex patterns
   rootNotePattern = std::regex(R"(([A-G](?:##|#|bb|b)?))");
   extensionPattern = std::regex(R"((?:#13|b13|13|#11|b11|11|#9|b9|9|b7|7))");
   alterationPattern = std::regex(R"((?:#5|\+5|b5|-5|#4|\+4))");
   addPattern = std::regex(R"(add\s*\(?\s*(?:#13|b13|13|#11|b11|11|#9|b9|9|6|4|2|m2|m3|#5|b5)\s*\)?)");
   bassNotePattern = std::regex(R"(/([A-G](?:##|#|bb|b)?))");
   inversionTextPattern = std::regex(R"((root(?:\s+pos(?:ition)?)?|1st\s+inv(?:ersion)?|2nd\s+inv(?:ersion)?|3rd\s+inv(?:ersion)?|bass))", 
                                    std::regex_constants::icase);
   
   initializeQualitySymbols();
}

void ChordParser::initializeQualitySymbols()
{
    // ROBUST PARSING: Back to discrete chord types for reliable parsing
    // We'll map messy real-world input to clean standardized output
    qualitySymbolsMap = {
        // === COMPOUND EXTENDED CHORDS (longest first) ===
        // These need to be treated as complete entities for robust parsing
        
        // 13th chords with modifiers
        {"maj13#11", {"maj13", {"#11"}}},
        {"maj13b9", {"maj13", {"b9"}}},
        {"13b5sus4", {"dom13", {"b5", "sus4"}}},
        {"13sus4", {"dom13", {"sus4"}}},
        {"13sus2", {"dom13", {"sus2"}}},
        {"13b9", {"dom13", {"b9"}}},
        {"13#11", {"dom13", {"#11"}}},
        {"13b5", {"dom13", {"b5"}}},
        {"m13", {"min13", {}}},
        {"min13", {"min13", {}}},
        {"-13", {"min13", {}}},
        {"maj13", {"maj13", {}}},
        {"13", {"dom13", {}}},
        
        // 11th chords with modifiers  
        {"maj11#5", {"maj11", {"#5"}}},
        {"11sus4", {"dom11", {"sus4"}}},
        {"11sus2", {"dom11", {"sus2"}}},
        {"11b5", {"dom11", {"b5"}}},
        {"m11", {"min11", {}}},
        {"min11", {"min11", {}}},
        {"-11", {"min11", {}}},
        {"maj11", {"maj11", {}}},
        {"11", {"dom11", {}}},
        {"dim11", {"dim11", {}}},
        
        // 9th chords with modifiers
        {"maj9#11", {"maj9", {"#11"}}},
        {"maj9b5", {"maj9", {"b5"}}},
        {"9sus4", {"dom9", {"sus4"}}},
        {"9sus2", {"dom9", {"sus2"}}},
        {"9b5sus4", {"dom9", {"b5", "sus4"}}},
        {"9b5sus2", {"dom9", {"b5", "sus2"}}},
        {"9b5", {"dom9", {"b5"}}},
        {"9#5", {"dom9", {"#5"}}},
        {"9b9", {"dom9", {"b9"}}},
        {"9#9", {"dom9", {"#9"}}},
        {"m9", {"min9", {}}},
        {"min9", {"min9", {}}},
        {"-9", {"min9", {}}},
        {"maj9", {"maj9", {}}},
        {"9", {"dom9", {}}},
        {"dim9", {"dim9", {}}},
        
        // Complex 7th chords
        {"7b5#9sus", {"dom7", {"#9", "sus4", "b5"}}},
        {"7b5(b9)sus", {"dom7", {"b9", "sus4", "b5"}}},
        {"7b5b9sus", {"dom7", {"b9", "sus4", "b5"}}},
        {"7(b9)", {"dom7", {"b9"}}},
        {"7b9b5", {"dom7", {"b9", "b5"}}},
        {"7#9", {"dom7", {"#9"}}},
        {"7b9", {"dom7", {"b9"}}},
        {"7#11", {"dom7", {"#11"}}},
        {"7#5", {"aug7", {}}},
        {"7b5", {"dom7", {"b5"}}},
        {"7sus4", {"dom7", {"sus4"}}},
        {"7sus2", {"dom7", {"sus2"}}},
        {"7sus", {"dom7", {"sus4"}}}, // Default sus to sus4
        
        // Major 7th variations
        {"maj7#11", {"maj7", {"#11"}}},
        {"maj7#5", {"augMaj7", {}}},
        {"maj7b5", {"maj7", {"b5"}}},
        {"maj7sus4", {"maj7", {"sus4"}}},
        {"maj7sus2", {"maj7", {"sus2"}}},
        {"maj7sus", {"maj7", {"sus4"}}},
        {"major7", {"maj7", {}}},
        {"ma7", {"maj7", {}}},
        {"∆7", {"maj7", {}}},
        {"∆", {"maj7", {}}},
        {"maj7", {"maj7", {}}},
        
        // Minor 7th variations
        {"m7b5", {"halfDim7", {}}},
        {"m7#5", {"min7", {"#5"}}},
        {"m7sus4", {"min7", {"sus4"}}},
        {"m7sus2", {"min7", {"sus2"}}},
        {"min7", {"min7", {}}},
        {"m7", {"min7", {}}},
        {"-7", {"min7", {}}},
        
        // Other 7th chords
        {"minmaj7", {"minMaj7", {}}},
        {"m(maj7)", {"minMaj7", {}}},
        {"m∆7", {"minMaj7", {}}},
        {"mmaj7", {"minMaj7", {}}},
        {"mm7", {"minMaj7", {}}},
        {"dim7", {"dim7", {}}},
        {"°7", {"dim7", {}}},
        {"o7", {"dim7", {}}},
        {"halfdim7", {"halfDim7", {}}},
        {"ø7", {"halfDim7", {}}},
        {"ø", {"halfDim7", {}}},
        {"aug7", {"aug7", {}}},
        {"+7", {"aug7", {}}},
        {"augmaj7", {"augMaj7", {}}},
        {"dom7", {"dom7", {}}},
        {"7", {"dom7", {}}},
        
        // 6th chords with extensions
        {"6/9", {"maj6", {"9"}}},
        {"6-9", {"maj6", {"9"}}},
        {"69", {"maj6", {"9"}}},
        {"6b9", {"maj6", {"b9"}}},
        {"6add9", {"maj6", {"add9"}}},
        {"6b5", {"maj6", {"b5"}}},
        {"m6/9", {"min6", {"9"}}},
        {"m6-9", {"min6", {"9"}}},
        {"m69", {"min6", {"9"}}},
        {"m6add9", {"min6", {"add9"}}},
        {"m6#5", {"min6", {"#5"}}},
        {"6", {"maj6", {}}},
        {"m6", {"min6", {}}},
        {"aug6", {"aug6", {}}},
        
        // Add chords
        {"add(m2)", {"maj", {"add2"}}},
        {"add(2)", {"maj", {"add2"}}},
        {"add(4)", {"maj", {"add4"}}},
        {"add(#5)", {"maj", {"add#5"}}},
        {"add(b5)", {"maj", {"addb5"}}},
        {"add(6)", {"maj", {"add6"}}},
        {"add(9)", {"maj", {"add9"}}},
        {"add(11)", {"maj", {"add11"}}},
        {"add(13)", {"maj", {"add13"}}},
        {"add#5", {"maj", {"add#5"}}},
        {"addb5", {"maj", {"addb5"}}},
        {"add13", {"maj", {"add13"}}},
        {"add11", {"maj", {"add11"}}},
        {"add9", {"maj", {"add9"}}},
        {"add6", {"maj", {"add6"}}},
        {"add4", {"maj", {"add4"}}},
        {"add2", {"maj", {"add2"}}},
        
        // Minor add chords
        {"madd9", {"min", {"add9"}}},
        {"madd11", {"min", {"add11"}}},
        {"madd4", {"min", {"add4"}}},
        {"madd2", {"min", {"add2"}}},
        {"m add9", {"min", {"add9"}}},
        {"m add(9)", {"min", {"add9"}}},
        {"m add(4)", {"min", {"add4"}}},
        {"m add(2)", {"min", {"add2"}}},
        {"m add(b5)", {"min", {"addb5"}}},
        {"min add9", {"min", {"add9"}}},
        {"minor add9", {"min", {"add9"}}},
        
        // Sus chords
        {"sus4b5", {"sus4", {"b5"}}},
        {"sus2sus4", {"sus2", {"sus4"}}}, // Both suspensions
        {"sus2", {"sus2", {}}},
        {"sus4", {"sus4", {}}},
        {"sus", {"sus4", {}}}, // Default to sus4
        
        // IMPROVED: Augmented chord handling (put these BEFORE triads)
        {"augmented", {"aug", {}}},
        {"aug", {"aug", {}}},
        {"+", {"aug", {}}},
        {"#5", {"aug", {}}}, // This should be augmented, not interval
        
        // Triads and basic qualities
        {"major", {"maj", {}}},
        {"maj", {"maj", {}}},
        {"ma", {"maj", {}}},
        {"minor", {"min", {}}},
        {"min", {"min", {}}},
        {"m", {"min", {}}},
        {"-", {"min", {}}},
        {"diminished", {"dim", {}}},
        {"dim", {"dim", {}}},
        {"°", {"dim", {}}},
        {"o", {"dim", {}}},
        {"flat5", {"flat5", {}}},
        {"b5", {"flat5", {}}},
        
        // Power chords and intervals - ONLY when very specific
        // Removed the bare "5" entry to prevent confusion
        
        // Intervals (only when explicitly marked)
        {"interval_P1", {"interval_P1", {}}},
        {"interval_m2", {"interval_m2", {}}},
        {"interval_M2", {"interval_M2", {}}},
        {"interval_m3", {"interval_m3", {}}},
        {"interval_M3", {"interval_M3", {}}},
        {"interval_P4", {"interval_P4", {}}},
        {"interval_A4", {"interval_A4", {}}},
        {"interval_d5", {"interval_d5", {}}},
        {"interval_P5", {"interval_P5", {}}},
        {"interval_A5", {"interval_A5", {}}},
        {"interval_m6", {"interval_m6", {}}},
        {"interval_M6", {"interval_M6", {}}},
        {"interval_m7", {"interval_m7", {}}},
        {"interval_M7", {"interval_M7", {}}},
        {"interval_P8", {"interval_P8", {}}},
        
        // Common alternative symbols - REMOVED problematic ones
        // These were causing the wrong interpretations
        {"P1", {"maj", {}}}, // Fallback to major
        {"m2", {"maj", {"add2"}}}, // Treat as add2
        {"M2", {"maj", {"add2"}}},
        {"m3", {"min", {}}}, // Minor triad
        {"M3", {"maj", {}}}, // Major triad  
        {"P4", {"sus4", {}}}, // Sus4
        {"A4", {"maj", {"#4"}}}, // Added #4
        {"d5", {"flat5", {}}}, // Flat5 triad
        {"P5", {"maj", {}}}, // Major triad (assume power chord context)
        {"A5", {"aug", {}}}, // Augmented
        {"m6", {"min6", {}}}, // Minor 6th
        {"M6", {"maj6", {}}}, // Major 6th
        {"m7", {"min7", {}}}, // Minor 7th
        {"M7", {"maj7", {}}}, // Major 7th
        {"P8", {"maj", {}}} // Octave -> major
    };
}

ChordParser::ParsedData ChordParser::parseFilename(const juce::String& filename)
{
    ParsedData data;
    
    if (filename.isEmpty())
        return data;
    
    // Extract basename and extension
    juce::File tempFile(filename);
    data.originalFilename = filename;
    data.cleanedBasename = tempFile.getFileNameWithoutExtension();
    data.originalExtension = tempFile.getFileExtension();
    
    juce::String workName = data.cleanedBasename;
    
    // ROBUST PARSING: Handle various filename patterns
    
    // 1. Check for chord progressions first
    if (isChordProgression(workName))
    {
        data.issues.add("Chord progression - not a single chord");
        return data;
    }
    
    // 2. Handle intervals explicitly (only when clearly marked as intervals)
    if (isInterval(workName))
    {
        return parseInterval(workName);
    }
    
    // 3. IMPROVED: Handle power chords more carefully
    // Only treat as simple power chord if it's truly just a 5th with no other chord information
    if (workName.startsWith("5_") && !workName.contains("add") && !workName.contains("#") && !workName.contains("b"))
    {
        juce::String powerChordPart = workName.fromFirstOccurrenceOf("_", false, false).trim();
        if (powerChordPart.isEmpty())
            powerChordPart = workName.fromFirstOccurrenceOf(" ", false, false).trim();
            
        // Only treat as interval if it's really simple like "5_ AE" or "5_ D5"
        if (powerChordPart.length() >= 1 && powerChordPart.length() <= 3)
        {
            data.rootNote = powerChordPart.substring(0, 1);
            if (powerChordPart.length() > 1 && (powerChordPart[1] == '#' || powerChordPart[1] == 'b'))
                data.rootNote += powerChordPart[1];
            
            // Check if this is really just a power chord (no other chord info)
            if (!powerChordPart.contains("add") && !powerChordPart.contains("maj") && 
                !powerChordPart.contains("min") && !powerChordPart.contains("7"))
            {
                data.standardizedQuality = "interval_P5";
                return data;
            }
        }
    }
    
    // 4. Main parsing: Split filename into components
    juce::String descriptorPart;
    juce::String specificChordPart;
    juce::String inversionPart;
    
    // Handle inversion suffix (anything after " - ")
    if (workName.contains(" - "))
    {
        int dashIndex = workName.lastIndexOf(" - ");
        juce::String afterDash = workName.substring(dashIndex + 3).trim();
        
        // Check if it's an inversion indicator
        if (isInversionIndicator(afterDash))
        {
            inversionPart = afterDash;
            workName = workName.substring(0, dashIndex).trim();
        }
    }
    
    // Split descriptor and chord parts
    if (workName.contains("_"))
    {
        // Primary pattern: "Descriptor_ ChordNotation"
        int underscoreIndex = workName.indexOf("_");
        juce::String potentialDesc = workName.substring(0, underscoreIndex).trim();
        juce::String potentialChord = workName.substring(underscoreIndex + 1).trim();
        
        // Validate the split makes sense
        if (potentialChord.isNotEmpty() && extractRootNote(potentialChord).isNotEmpty())
        {
            descriptorPart = potentialDesc;
            specificChordPart = potentialChord;
        }
        else if (potentialDesc.isNotEmpty() && extractRootNote(potentialDesc).isNotEmpty())
        {
            // Sometimes it's backwards
            descriptorPart = potentialChord;
            specificChordPart = potentialDesc;
        }
        else
        {
            // Fall back to treating the whole thing as chord notation
            specificChordPart = workName;
        }
    }
    else
    {
        // No underscore - try to find where descriptor ends and chord begins
        std::regex rootSearchPattern(R"(\b([A-G](?:[#b])*)[^a-zA-Z])");
        std::smatch rootMatch;
        std::string workNameStr = workName.toStdString();
        
        if (std::regex_search(workNameStr, rootMatch, rootSearchPattern))
        {
            size_t rootPos = static_cast<size_t>(rootMatch.position());
            if (rootPos > 0)
            {
                descriptorPart = workName.substring(0, static_cast<int>(rootPos)).trim();
                specificChordPart = workName.substring(static_cast<int>(rootPos)).trim();
            }
            else
            {
                specificChordPart = workName;
            }
        }
        else
        {
            specificChordPart = workName;
        }
    }
    
    if (specificChordPart.isEmpty())
    {
        data.issues.add("Could not identify chord notation");
        return data;
    }
    
    // Store parsed components
    data.qualityDescriptorString = descriptorPart;
    data.specificChordNotationFull = specificChordPart;
    data.inversionText = inversionPart;
    
    // 5. Extract root note
    data.rootNote = extractRootNote(specificChordPart);
    if (data.rootNote.isEmpty())
    {
        data.issues.add("No root note found");
        return data;
    }
    
    // 6. Parse quality - IMPROVED LOGIC
    juce::String qualityString = specificChordPart.substring(data.rootNote.length()).trim();
    
    // Extract slash bass note first
    std::smatch bassMatch;
    std::string qualityStd = qualityString.toStdString();
    if (std::regex_search(qualityStd, bassMatch, bassNotePattern))
    {
        data.bassNoteSlash = juce::String(bassMatch[1].str());
        qualityString = qualityString.substring(0, static_cast<int>(bassMatch.position())).trim();
    }
    
    // SPECIAL HANDLING: Check for augmented chords first
    if (descriptorPart.toLowerCase().contains("#5") || 
        descriptorPart.toLowerCase().contains("aug") ||
        qualityString.contains("#5") ||
        qualityString.contains("aug"))
    {
        data.standardizedQuality = "aug";
        
        // Extract any additional modifiers
        if (qualityString.contains("7") || descriptorPart.contains("7"))
        {
            data.standardizedQuality = "aug7";
        }
        else if (qualityString.contains("maj7") || descriptorPart.contains("maj7"))
        {
            data.standardizedQuality = "augMaj7";
        }
    }
    // SPECIAL HANDLING: Power chords with extensions (like "5 add6")
    else if ((descriptorPart.startsWith("5 ") || descriptorPart.startsWith("5") || specificChordPart.contains("5")) && 
             (descriptorPart.contains("add") || specificChordPart.contains("add") || qualityString.contains("add")))
    {
        data.standardizedQuality = "maj"; // Base triad, but we'll mark it as no 3rd later
        
        // Extract the add notes
        juce::String fullText = descriptorPart + " " + specificChordPart + " " + qualityString;
        if (fullText.contains("add6"))
        {
            data.addedNotes.add("add6");
        }
        if (fullText.contains("add9"))
        {
            data.addedNotes.add("add9");
        }
        if (fullText.contains("add4"))
        {
            data.addedNotes.add("add4");
        }
        if (fullText.contains("add2"))
        {
            data.addedNotes.add("add2");
        }
        
        // Mark that this is a power chord (no 3rd) by using a special note
        data.suspensions.add("no3rd"); // We'll handle this in display logic
    }
    else
    {
        // Try to match against quality symbols - MOST SPECIFIC FIRST
        if (!qualityString.isEmpty())
        {
            // Sort by length (longest first) for proper matching
            std::vector<std::pair<juce::String, std::pair<juce::String, juce::StringArray>>> sortedSymbols;
            for (const auto& [key, value] : qualitySymbolsMap)
            {
                sortedSymbols.push_back({key, value});
            }
            
            std::sort(sortedSymbols.begin(), sortedSymbols.end(),
                      [](const auto& a, const auto& b) { return a.first.length() > b.first.length(); });
            
            bool foundMatch = false;
            juce::String normalizedQuality = qualityString.toLowerCase().replace(" ", "");
            
            for (const auto& [symbol, qualityInfo] : sortedSymbols)
            {
                juce::String normalizedSymbol = symbol.toLowerCase().replace(" ", "");
                
                if (normalizedQuality == normalizedSymbol ||
                    normalizedQuality.startsWith(normalizedSymbol))
                {
                    data.standardizedQuality = qualityInfo.first;
                    
                    // Add any implied extensions/alterations
                    for (const auto& ext : qualityInfo.second)
                    {
                        if (ext.startsWith("add"))
                            data.addedNotes.add(ext);
                        else if (ext.contains("sus"))
                            data.suspensions.add(ext);
                        else if (ext.contains("#") || ext.contains("b"))
                            data.alterations.add(ext);
                        else
                            data.extensions.add(ext);
                    }
                    
                    foundMatch = true;
                    break;
                }
            }
            
            if (!foundMatch)
            {
                // Try descriptor-based matching
                if (!descriptorPart.isEmpty())
                {
                    foundMatch = parseFromDescriptor(descriptorPart, data);
                }
            }
            
            if (!foundMatch)
            {
                // Extract any additional extensions/alterations from remaining quality string
                extractExtensionsAndAlterations(qualityString, data);
            }
        }
    }
    
    // 7. Use descriptor to help determine quality if not found
    if (data.standardizedQuality.isEmpty() && !descriptorPart.isEmpty())
    {
        parseFromDescriptor(descriptorPart, data);
    }
    
    // 8. Default to major if we have a root but no quality
    if (data.standardizedQuality.isEmpty() && !data.rootNote.isEmpty())
    {
        data.standardizedQuality = "maj";
    }
    
    // 9. Parse inversion information
    if (!inversionPart.isEmpty())
    {
        parseInversionText(inversionPart, data);
    }
    
    // 10. Set final bass note
    if (!data.bassNoteSlash.isEmpty() && data.determinedBassNote.isEmpty())
    {
        data.determinedBassNote = data.bassNoteSlash;
    }
    
    // 11. Validate and clean up
    validateAndCleanup(data);
    
    return data;
}

// Helper method to parse interval notation
ChordParser::ParsedData ChordParser::parseInterval(const juce::String& str)
{
    ParsedData data;
    
    // Extract root note
    data.rootNote = extractRootNote(str);
    if (data.rootNote.isEmpty())
    {
        data.issues.add("No root note found in interval");
        return data;
    }
    
    // Find interval type
    juce::String strLower = str.toLowerCase();
    
    if (strLower.contains("minor 2") || strLower.contains("m2"))
        data.standardizedQuality = "interval_m2";
    else if (strLower.contains("major 2") || strLower.contains("M2"))
        data.standardizedQuality = "interval_M2";
    else if (strLower.contains("minor 3") || strLower.contains("m3"))
        data.standardizedQuality = "interval_m3";
    else if (strLower.contains("major 3") || strLower.contains("M3"))
        data.standardizedQuality = "interval_M3";
    else if (strLower.contains("perfect 4") || strLower.contains("P4"))
        data.standardizedQuality = "interval_P4";
    else if (strLower.contains("tritone") || strLower.contains("aug 4") || strLower.contains("A4"))
        data.standardizedQuality = "interval_A4";
    else if (strLower.contains("dim 5") || strLower.contains("d5"))
        data.standardizedQuality = "interval_d5";
    else if (strLower.contains("perfect 5") || strLower.contains("P5"))
        data.standardizedQuality = "interval_P5";
    else if (strLower.contains("aug 5") || strLower.contains("A5"))
        data.standardizedQuality = "interval_A5";
    else if (strLower.contains("minor 6") || strLower.contains("m6"))
        data.standardizedQuality = "interval_m6";
    else if (strLower.contains("major 6") || strLower.contains("M6"))
        data.standardizedQuality = "interval_M6";
    else if (strLower.contains("minor 7") || strLower.contains("m7"))
        data.standardizedQuality = "interval_m7";
    else if (strLower.contains("major 7") || strLower.contains("M7"))
        data.standardizedQuality = "interval_M7";
    else if (strLower.contains("octave") || strLower.contains("P8"))
        data.standardizedQuality = "interval_P8";
    else
    {
        data.issues.add("Unknown interval type");
        data.standardizedQuality = "interval_P5"; // Default
    }
    
    return data;
}

// Helper to parse from descriptor
bool ChordParser::parseFromDescriptor(const juce::String& descriptor, ParsedData& data)
{
    juce::String descLower = descriptor.toLowerCase().replace(" ", "");
    
    // IMPROVED: Handle augmented chord descriptions first
    if (descLower.contains("#5") || descLower.contains("aug"))
    {
        if (data.standardizedQuality.isEmpty())
        {
            if (descLower.contains("maj7") || descLower.contains("major7"))
                data.standardizedQuality = "augMaj7";
            else if (descLower.contains("7"))
                data.standardizedQuality = "aug7";
            else
                data.standardizedQuality = "aug";
        }
        return true;
    }
    
    // Handle power chord descriptions
    if (descLower.startsWith("5") && (descLower.contains("add") || descriptor.contains("add")))
    {
        if (data.standardizedQuality.isEmpty())
            data.standardizedQuality = "maj";
        
        // Mark as power chord
        if (!data.suspensions.contains("no3rd"))
            data.suspensions.add("no3rd");
        
        return true;
    }
    
    // Map common descriptors to chord types
    static const std::unordered_map<juce::String, juce::String> descriptorMap = {
        {"major", "maj"}, {"maj", "maj"},
        {"minor", "min"}, {"min", "min"},
        {"diminished", "dim"}, {"dim", "dim"},
        {"augmented", "aug"}, {"aug", "aug"},
        {"major7", "maj7"}, {"maj7", "maj7"}, {"major7th", "maj7"},
        {"minor7", "min7"}, {"min7", "min7"}, {"minor7th", "min7"},
        {"dominant7", "dom7"}, {"dom7", "dom7"}, {"7", "dom7"}, {"7th", "dom7"},
        {"major6", "maj6"}, {"maj6", "maj6"}, {"6", "maj6"}, {"6th", "maj6"},
        {"minor6", "min6"}, {"min6", "min6"},
        {"major9", "maj9"}, {"maj9", "maj9"}, {"9th", "dom9"}, {"9", "dom9"},
        {"minor9", "min9"}, {"min9", "min9"},
        {"major11", "maj11"}, {"maj11", "maj11"}, {"11th", "dom11"}, {"11", "dom11"},
        {"minor11", "min11"}, {"min11", "min11"},
        {"major13", "maj13"}, {"maj13", "maj13"}, {"13th", "dom13"}, {"13", "dom13"},
        {"minor13", "min13"}, {"min13", "min13"},
        {"sus4", "sus4"}, {"sus2", "sus2"}, {"suspended4", "sus4"}, {"suspended2", "sus2"}
    };
    
    auto it = descriptorMap.find(descLower);
    if (it != descriptorMap.end())
    {
        if (data.standardizedQuality.isEmpty())
            data.standardizedQuality = it->second;
        return true;
    }
    
    return false;
}

// Helper to check if string indicates inversion
bool ChordParser::isInversionIndicator(const juce::String& str) const
{
    juce::String lower = str.toLowerCase();
    return lower.contains("inversion") || lower.contains("inv") ||
           lower.contains("bass") || lower.contains("root") ||
           lower.contains("position") || lower.contains("pos");
}

// Helper to validate and cleanup parsed data
void ChordParser::validateAndCleanup(ParsedData& data)
{
    // Validate chord type exists
    auto chordTypes = ChordTypes::getStandardizedChordTypes();
    if (!data.standardizedQuality.isEmpty() && 
        chordTypes.find(data.standardizedQuality.toStdString()) == chordTypes.end())
    {
        data.issues.add("Unknown chord type: " + data.standardizedQuality);
        data.standardizedQuality = "maj"; // Fallback
    }
    
    // Remove duplicates from arrays
    auto removeDuplicates = [](juce::StringArray& arr) {
        juce::StringArray unique;
        for (const auto& item : arr)
        {
            if (!unique.contains(item))
                unique.add(item);
        }
        arr = unique;
    };
    
    removeDuplicates(data.extensions);
    removeDuplicates(data.alterations);
    removeDuplicates(data.addedNotes);
    removeDuplicates(data.suspensions);
    
    // Basic validation
    if (data.rootNote.isEmpty())
        data.issues.add("No root note found");
    if (data.standardizedQuality.isEmpty())
        data.issues.add("No chord quality determined");
}

juce::String ChordParser::extractRootNote(const juce::String& str)
{
   std::smatch match;
   std::string strStd = str.toStdString();
   
   if (std::regex_search(strStd, match, rootNotePattern))
   {
       return juce::String(match[0].str());
   }
   
   return juce::String();
}

void ChordParser::extractExtensionsAndAlterations(const juce::String& str, ParsedData& data)
{
   if (str.isEmpty())
       return;
   
   // Normalize string
   juce::String normalized = normalizeForParsing(str);
   std::string normalizedStd = normalized.toStdString();
   
   // Extract add patterns first
   std::smatch match;
   std::string temp = normalizedStd;
   while (std::regex_search(temp, match, addPattern))
   {
       juce::String addStr = juce::String(match[0].str());
       if (!data.addedNotes.contains(addStr))
           data.addedNotes.add(addStr);
       temp = match.suffix();
   }
   
   // Extract extensions
   temp = normalizedStd;
   while (std::regex_search(temp, match, extensionPattern))
   {
       juce::String ext = juce::String(match[0].str());
       if (!data.extensions.contains(ext))
           data.extensions.add(ext);
       temp = match.suffix();
   }
   
   // Extract alterations
   temp = normalizedStd;
   while (std::regex_search(temp, match, alterationPattern))
   {
       juce::String alt = juce::String(match[0].str());
       if (!data.alterations.contains(alt))
           data.alterations.add(alt);
       temp = match.suffix();
   }
   
   // Extract sus
   if (normalized.contains("sus4") && !data.suspensions.contains("sus4"))
       data.suspensions.add("sus4");
   else if (normalized.contains("sus2") && !data.suspensions.contains("sus2"))
       data.suspensions.add("sus2");
   else if (normalized.contains("sus") && !data.suspensions.contains("sus4"))
       data.suspensions.add("sus4");
}

void ChordParser::parseInversionText(const juce::String& text, ParsedData& data)
{
   std::smatch match;
   std::string textStd = text.toLowerCase().toStdString();
   
   if (std::regex_search(textStd, match, inversionTextPattern))
   {
       data.inversionTextParsed = juce::String(match[0].str());
       
       if (data.inversionTextParsed.contains("bass"))
       {
           juce::String bassNote = extractRootNote(text);
           if (bassNote.isNotEmpty())
           {
               data.determinedBassNote = bassNote;
           }
       }
   }
}

bool ChordParser::isChordProgression(const juce::String& str) const
{
   juce::String strLower = str.toLowerCase();
   
   // Don't mistake inversions for progressions
   if (strLower.contains(" - ") && isInversionIndicator(strLower))
       return false;
   
   // Check for clear progression indicators
   if (strLower.contains("ii-v") || strLower.contains("i-ii-v") || strLower.contains("v-i"))
       return true;
   
   // Roman numeral patterns
   std::regex romanProgression(R"([ivx]+-[ivx]+)", std::regex_constants::icase);
   return std::regex_search(str.toStdString(), romanProgression);
}

bool ChordParser::isInterval(const juce::String& str) const
{
   return str.startsWithIgnoreCase("interval");
}

juce::String ChordParser::normalizeForParsing(const juce::String& str) const
{
   juce::String normalized = str;
   normalized = normalized.replace(":", " ")
                         .replace("(", " ")
                         .replace(")", " ")
                         .replace(",", " ")
                         .replace(";", " ");
   
   while (normalized.contains("  "))
       normalized = normalized.replace("  ", " ");
   
   return normalized.trim();
}

// ParsedData helper methods
juce::String ChordParser::ParsedData::getFullChordName() const
{
    juce::String name = rootNote;
    
    // Special handling for power chords (no 3rd)
    bool isPowerChord = suspensions.contains("no3rd");
    
    if (isPowerChord)
    {
        // Power chord notation: root + "5" + extensions
        name += "5";
        
        // Add extensions in parentheses for power chords
        juce::StringArray powerChordExtensions;
        
        // Add suspensions (except no3rd)
        for (const auto& sus : suspensions)
        {
            if (sus != "no3rd")
                powerChordExtensions.add(sus);
        }
        
        // Add added notes
        for (const auto& add : addedNotes)
        {
            powerChordExtensions.add(add);
        }
        
        // Add alterations
        for (const auto& alt : alterations)
        {
            powerChordExtensions.add(alt);
        }
        
        // Add extensions
        for (const auto& ext : extensions)
        {
            powerChordExtensions.add(ext);
        }
        
        if (!powerChordExtensions.isEmpty())
        {
            name += "(" + powerChordExtensions.joinIntoString(",") + ")";
        }
    }
    else
    {
        // Regular chord notation
        
        // Build from components using the quality map
        auto qualityMap = ChordTypes::getQualityDisplayMap();
        auto it = qualityMap.find(standardizedQuality.toStdString());
        if (it != qualityMap.end() && it->second.isNotEmpty())
        {
            name += it->second;
        }
        else if (standardizedQuality.isNotEmpty() && standardizedQuality != "maj")
        {
            name += standardizedQuality;
        }
        
        // Add suspensions
        for (const auto& sus : suspensions)
        {
            if (sus != "no3rd") // Skip the special no3rd marker
                name += sus;
        }
        
        // Add extensions
        for (const auto& ext : extensions)
        {
            name += ext;
        }
        
        // Add alterations
        for (const auto& alt : alterations)
        {
            name += alt;
        }
        
        // Add added notes
        for (const auto& add : addedNotes)
        {
            if (add.contains("add"))
                name += add;
            else
                name += "add" + add;
        }
    }
    
    // Add bass note if different from root
    if (determinedBassNote.isNotEmpty() && determinedBassNote != rootNote)
        name += "/" + determinedBassNote;
    
    return name;
}

juce::String ChordParser::ParsedData::getInversionSuffix() const
{
   if (inversionTextParsed.isEmpty() || inversionTextParsed.contains("root"))
       return "";
   
   if (inversionTextParsed.contains("1st"))
       return "_inv1";
   if (inversionTextParsed.contains("2nd"))
       return "_inv2";
   if (inversionTextParsed.contains("3rd"))
       return "_inv3";
   if (inversionTextParsed.contains("bass"))
   {
       if (determinedBassNote.isNotEmpty())
           return "_bass" + determinedBassNote;
       return "_bass";
   }
   
   return "";
}
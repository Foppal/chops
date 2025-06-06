#include "FilenameUtils.h"
#include "../Core/ChordTypes.h"

namespace FilenameUtils
{

bool isValidParsedData(const ChordParser::ParsedData& parsedData)
{
   // Check for critical parsing failures
   if (parsedData.rootNote.isEmpty())
       return false;
   
   if (parsedData.standardizedQuality.isEmpty())
       return false;
   
   // Check for issues that would prevent proper organization
   if (!parsedData.issues.isEmpty())
   {
       for (const auto& issue : parsedData.issues)
       {
           // Fatal issues that should move file to mismatch
           if (issue.contains("Chord progression") ||
               issue.contains("Chord transition") ||
               issue.contains("No root note") ||
               issue.contains("No chord quality"))
           {
               return false;
           }
       }
   }
   
   // Validate that the chord type exists in our taxonomy
   auto chordTypes = ChordTypes::getStandardizedChordTypes();
   if (chordTypes.find(parsedData.standardizedQuality.toStdString()) == chordTypes.end())
   {
       return false;
   }
   
   return true;
}

juce::String generateNewSampleFilename(const ChordParser::ParsedData& parsedData, const juce::String& originalExtension)
{
    if (!isValidParsedData(parsedData))
        return "parse_failed_" + parsedData.originalFilename;
    
    juce::String filename;
    
    // Root note
    filename += parsedData.rootNote;
    
    // Base quality
    auto qualityMap = ChordTypes::getQualityDisplayMap();
    auto it = qualityMap.find(parsedData.standardizedQuality.toStdString());
    if (it != qualityMap.end() && it->second.isNotEmpty())
    {
        filename += it->second;
    }
    else if (parsedData.standardizedQuality.isNotEmpty() && 
             parsedData.standardizedQuality != "maj" &&
             !parsedData.standardizedQuality.startsWith("interval"))
    {
        filename += parsedData.standardizedQuality;
    }
    
    // Track what's already in the filename to avoid duplicates
    juce::String filenameCheck = filename.toLowerCase();
    
    // Add suspensions (check for duplicates)
    for (const auto& sus : parsedData.suspensions)
    {
        if (!filenameCheck.contains(sus.toLowerCase()))
        {
            filename += sus;
            filenameCheck += sus.toLowerCase();
        }
    }
    
    // Add extensions (avoid duplicates with chord type)
    // Don't add 9, 11, or 13 if the chord type already includes them
    for (const auto& ext : parsedData.extensions)
    {
        bool shouldAdd = true;
        
        // Check if this extension is already part of the chord type
        if ((parsedData.standardizedQuality.contains("9") && ext == "9") ||
            (parsedData.standardizedQuality.contains("11") && (ext == "9" || ext == "11")) ||
            (parsedData.standardizedQuality.contains("13") && (ext == "9" || ext == "11" || ext == "13")))
        {
            shouldAdd = false;
        }
        
        if (shouldAdd && !filenameCheck.contains(ext.toLowerCase()))
        {
            filename += ext;
            filenameCheck += ext.toLowerCase();
        }
    }
    
    // Add alterations
    for (const auto& alt : parsedData.alterations)
    {
        if (!filenameCheck.contains(alt.toLowerCase()))
        {
            filename += alt;
            filenameCheck += alt.toLowerCase();
        }
    }
    
    // Add added notes intelligently
    for (const auto& add : parsedData.addedNotes)
    {
        juce::String cleanAdd = add.replace(" ", ""); // Remove spaces
        juce::String noteToAdd;
        
        if (cleanAdd.startsWith("add"))
        {
            noteToAdd = cleanAdd.substring(3); // Get the note part after "add"
        }
        else
        {
            noteToAdd = cleanAdd;
        }
        
        // Check if this note is already in the filename
        // For example, don't add "add9" if we already have "9" in the chord
        if (!filenameCheck.contains(noteToAdd.toLowerCase()))
        {
            filename += "add" + noteToAdd;
            filenameCheck += "add" + noteToAdd.toLowerCase();
        }
    }
    
    // Add bass note if present
    if (parsedData.determinedBassNote.isNotEmpty() && 
        parsedData.determinedBassNote != parsedData.rootNote &&
        parsedData.inversionTextParsed.isEmpty())
    {
        filename += "_" + parsedData.determinedBassNote;
    }
    
    // Add inversion suffix
    filename += parsedData.getInversionSuffix();
    
    // Add extension
    filename += originalExtension;
    
    // Clean up
    filename = sanitizeFilename(filename);
    
    return filename;
}

juce::String sanitizeFilename(const juce::String& filename)
{
   juce::String sanitized = filename;
   
   // Remove or replace truly problematic characters for cross-platform compatibility
   // These are the only characters that cause real filesystem issues
   sanitized = sanitized.replace("<", "_")
                       .replace(">", "_")
                       .replace(":", "_")
                       .replace("\"", "_")
                       .replace("|", "_")
                       .replace("?", "_")
                       .replace("*", "_")
                       .replace("/", "_")   // Path separator
                       .replace("\\", "_"); // Windows path separator
   
   // IMPORTANT: Keep # and b as they're valid and essential for music notation
   // Don't replace # with s or b with f - modern filesystems handle these fine
   
   // Clean up multiple consecutive separators
   while (sanitized.contains("__"))
       sanitized = sanitized.replace("__", "_");
   
   while (sanitized.contains("--"))
       sanitized = sanitized.replace("--", "-");
   
   // Remove leading/trailing underscores and hyphens
   while (sanitized.startsWith("_") || sanitized.startsWith("-"))
       sanitized = sanitized.substring(1);
   
   while (sanitized.endsWith("_") && !sanitized.endsWith("."))
       sanitized = sanitized.dropLastCharacters(1);
   
   // Ensure the filename isn't empty (excluding extension)
   juce::String withoutExt = sanitized.upToLastOccurrenceOf(".", false, false);
   if (withoutExt.isEmpty())
       sanitized = "unnamed" + sanitized;
   
   return sanitized;
}

juce::String getQualityDisplayString(const ChordParser::ParsedData& parsedData)
{
   juce::String result;
   
   // Get base quality
   auto qualityMap = ChordTypes::getQualityDisplayMap();
   auto it = qualityMap.find(parsedData.standardizedQuality.toStdString());
   if (it != qualityMap.end() && it->second.isNotEmpty())
   {
       result += it->second;
   }
   else if (parsedData.standardizedQuality.isNotEmpty() && parsedData.standardizedQuality != "maj")
   {
       result += parsedData.standardizedQuality;
   }
   
   // Add suspensions
   for (const auto& sus : parsedData.suspensions)
   {
       result += sus;
   }
   
   // Add extensions
   for (const auto& ext : parsedData.extensions)
   {
       result += ext;
   }
   
   // Add alterations
   for (const auto& alt : parsedData.alterations)
   {
       result += alt;
   }
   
   // Add added notes
   for (const auto& add : parsedData.addedNotes)
   {
       if (add.contains("add"))
           result += add;
       else
           result += "add" + add;
   }
   
   return result;
}

bool isAudioFile(const juce::File& file)
{
   static const juce::StringArray audioExtensions = {
       ".wav", ".mp3", ".aif", ".aiff", ".flac", ".m4a", ".ogg", ".wma", ".caf"
   };
   
   juce::String extension = file.getFileExtension().toLowerCase();
   return audioExtensions.contains(extension);
}

juce::String normalizeRootNote(const juce::String& rootNote)
{
   juce::String normalized = rootNote.trim().toUpperCase();
   
   // Handle common alternative notations
   if (normalized == "DB") normalized = "C#";
   if (normalized == "EB") normalized = "D#";
   if (normalized == "GB") normalized = "F#";
   if (normalized == "AB") normalized = "G#";
   if (normalized == "BB") normalized = "A#";
   
   // Handle double sharps and flats (rare but possible)
   if (normalized.endsWith("##"))
   {
       // Convert double sharp to equivalent note
       juce::String baseNote = normalized.substring(0, 1);
       if (baseNote == "C") return "D";
       if (baseNote == "D") return "E";
       if (baseNote == "E") return "F#";
       if (baseNote == "F") return "G";
       if (baseNote == "G") return "A";
       if (baseNote == "A") return "B";
       if (baseNote == "B") return "C#";
   }
   
   if (normalized.endsWith("BB"))
   {
       // Convert double flat to equivalent note
       juce::String baseNote = normalized.substring(0, 1);
       if (baseNote == "C") return "Bb";
       if (baseNote == "D") return "C";
       if (baseNote == "E") return "D";
       if (baseNote == "F") return "Eb";
       if (baseNote == "G") return "F";
       if (baseNote == "A") return "G";
       if (baseNote == "B") return "A";
   }
   
   return normalized;
}

juce::String getChordFolderName(const juce::String& standardizedQuality)
{
   // Use the chord taxonomy sanitization function
   return ChordTypes::sanitizeChordFolderName(standardizedQuality);
}

bool validateFilename(const juce::String& filename)
{
   // Check for empty filename
   if (filename.isEmpty())
       return false;
   
   // Check for reserved Windows filenames
   juce::StringArray reservedNames = {
       "CON", "PRN", "AUX", "NUL",
       "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
       "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
   };
   
   juce::String nameWithoutExt = filename.upToLastOccurrenceOf(".", false, false).toUpperCase();
   if (reservedNames.contains(nameWithoutExt))
       return false;
   
   // Check for problematic characters
   juce::String problematic = "<>:\"|?*/\\";
   for (int i = 0; i < problematic.length(); ++i)
   {
       if (filename.containsChar(problematic[i]))
           return false;
   }
   
   // Check length (most filesystems support 255 characters)
   if (filename.length() > 255)
       return false;
   
   // Check for leading/trailing spaces or dots
   if (filename.startsWithChar(' ') || filename.endsWithChar(' ') ||
       filename.startsWithChar('.') || (filename.endsWithChar('.') && !filename.contains(".")))
       return false;
   
   return true;
}

juce::Array<juce::File> getAllAudioFiles(const juce::File& directory, bool recursive)
{
   juce::Array<juce::File> audioFiles;
   
   if (!directory.isDirectory())
       return audioFiles;
   
   juce::Array<juce::File> allFiles;
   directory.findChildFiles(allFiles, 
                           recursive ? juce::File::findFilesAndDirectories : juce::File::findFiles, 
                           recursive);
   
   for (const auto& file : allFiles)
   {
       if (file.existsAsFile() && isAudioFile(file))
       {
           audioFiles.add(file);
       }
   }
   
   return audioFiles;
}

juce::String createUniqueFilename(const juce::File& directory, const juce::String& desiredName)
{
   juce::File proposedFile = directory.getChildFile(desiredName);
   
   if (!proposedFile.exists())
       return desiredName;
   
   juce::String baseName = proposedFile.getFileNameWithoutExtension();
   juce::String extension = proposedFile.getFileExtension();
   
   int counter = 2;
   do {
       juce::String newName = baseName + "_" + juce::String(counter) + extension;
       proposedFile = directory.getChildFile(newName);
       
       if (!proposedFile.exists())
           return newName;
       
       counter++;
   } while (counter < 1000); // Reasonable upper limit
   
   // If we somehow get here, add timestamp
   juce::String timestamp = juce::String(juce::Time::getCurrentTime().toMilliseconds());
   return baseName + "_" + timestamp + extension;
}

} // namespace FilenameUtils
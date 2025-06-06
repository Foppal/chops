#include "MetadataService.h"
#include "ChordParser.h"
#include "../Utils/FilenameUtils.h"
#include <fstream>
#include <algorithm>
#include <cstring>  // for memcpy

//==============================================================================
MetadataService::MetadataService()
{
    juce::Logger::writeToLog("MetadataService initialized");
}

//==============================================================================
// ChordMetadata Implementation
//==============================================================================

bool MetadataService::ChordMetadata::isValid() const
{
    return rootNote.isNotEmpty() && chordType.isNotEmpty();
}

juce::String MetadataService::ChordMetadata::toString() const
{
    juce::String result;
    result << "ChordMetadata{\n";
    result << "  rootNote: '" << rootNote << "'\n";
    result << "  chordType: '" << chordType << "'\n";
    result << "  chordTypeDisplay: '" << chordTypeDisplay << "'\n";
    result << "  tags: [" << tags.joinIntoString(", ") << "]\n";
    result << "  rating: " << rating << "\n";
    result << "  isFavorite: " << (isFavorite ? "true" : "false") << "\n";
    result << "}";
    return result;
}

ChopsDatabase::SampleInfo MetadataService::ChordMetadata::toDatabaseSampleInfo(const juce::String& filePath, int64 fileSize) const
{
    ChopsDatabase::SampleInfo sampleInfo;
    
    // File information
    sampleInfo.filePath = filePath;
    sampleInfo.fileSize = fileSize;
    sampleInfo.originalFilename = originalFilename.isNotEmpty() ? originalFilename : juce::File(filePath).getFileName();
    sampleInfo.currentFilename = juce::File(filePath).getFileName();
    
    // Chord information
    sampleInfo.rootNote = rootNote;
    sampleInfo.chordType = chordType;
    sampleInfo.chordTypeDisplay = chordTypeDisplay;
    sampleInfo.extensions = extensions;
    sampleInfo.alterations = alterations;
    sampleInfo.addedNotes = addedNotes;
    sampleInfo.suspensions = suspensions;
    sampleInfo.bassNote = bassNote;
    sampleInfo.inversion = inversion;
    
    // User metadata
    sampleInfo.tags = tags;
    sampleInfo.rating = rating;
    sampleInfo.isFavorite = isFavorite;
    sampleInfo.userNotes = userNotes;
    sampleInfo.color = color;
    sampleInfo.playCount = playCount;
    sampleInfo.lastPlayed = lastPlayed;
    
    // Timestamps
    sampleInfo.dateAdded = dateAdded;
    sampleInfo.dateModified = dateModified;
    
    return sampleInfo;
}

MetadataService::ChordMetadata MetadataService::ChordMetadata::fromDatabaseSampleInfo(const ChopsDatabase::SampleInfo& sampleInfo)
{
    ChordMetadata metadata;
    
    // Chord information
    metadata.rootNote = sampleInfo.rootNote;
    metadata.chordType = sampleInfo.chordType;
    metadata.chordTypeDisplay = sampleInfo.chordTypeDisplay;
    metadata.extensions = sampleInfo.extensions;
    metadata.alterations = sampleInfo.alterations;
    metadata.addedNotes = sampleInfo.addedNotes;
    metadata.suspensions = sampleInfo.suspensions;
    metadata.bassNote = sampleInfo.bassNote;
    metadata.inversion = sampleInfo.inversion;
    
    // User metadata
    metadata.tags = sampleInfo.tags;
    metadata.rating = sampleInfo.rating;
    metadata.isFavorite = sampleInfo.isFavorite;
    metadata.userNotes = sampleInfo.userNotes;
    metadata.color = sampleInfo.color;
    metadata.playCount = sampleInfo.playCount;
    metadata.lastPlayed = sampleInfo.lastPlayed;
    
    // System metadata
    metadata.originalFilename = sampleInfo.originalFilename;
    metadata.dateAdded = sampleInfo.dateAdded;
    metadata.dateModified = sampleInfo.dateModified;
    
    return metadata;
}

//==============================================================================
// Core Metadata Operations
//==============================================================================

bool MetadataService::readMetadataFromFile(const juce::File& audioFile, ChordMetadata& metadata)
{
    if (!audioFile.existsAsFile() || !isAudioFile(audioFile))
    {
        juce::Logger::writeToLog("MetadataService: Invalid audio file: " + audioFile.getFullPathName());
        return false;
    }
    
    juce::String iXMLContent;
    if (!readIXMLChunk(audioFile, iXMLContent))
    {
        // No iXML chunk found - not an error, just means no metadata
        return false;
    }
    
    return iXMLToMetadata(iXMLContent, metadata);
}

bool MetadataService::writeMetadataToFile(const juce::File& audioFile, const ChordMetadata& metadata)
{
    if (!audioFile.existsAsFile() || !isAudioFile(audioFile))
    {
        juce::Logger::writeToLog("MetadataService: Cannot write to invalid audio file: " + audioFile.getFullPathName());
        return false;
    }
    
    if (!metadata.isValid())
    {
        juce::Logger::writeToLog("MetadataService: Cannot write invalid metadata");
        return false;
    }
    
    juce::String iXMLContent = metadataToIXML(metadata);
    return writeIXMLChunk(audioFile, iXMLContent);
}

bool MetadataService::hasMetadata(const juce::File& audioFile)
{
    ChordMetadata metadata;
    return readMetadataFromFile(audioFile, metadata);
}

//==============================================================================
// Database Sync Operations
//==============================================================================

bool MetadataService::syncFileWithDatabase(const juce::File& audioFile, ChopsDatabase* database)
{
    if (!database || !audioFile.existsAsFile())
        return false;
    
    juce::String filePath = audioFile.getFullPathName();
    
    // Check if file exists in database
    auto existingSample = database->getSampleByPath(filePath);
    
    ChordMetadata fileMetadata;
    bool hasFileMetadata = readMetadataFromFile(audioFile, fileMetadata);
    
    if (existingSample)
    {
        // File exists in database
        if (hasFileMetadata)
        {
            // Both file and database have metadata - check which is newer
            juce::Time fileModTime = audioFile.getLastModificationTime();
            juce::Time dbModTime = existingSample->dateModified;
            
            if (fileModTime > dbModTime)
            {
                // File is newer - update database
                auto updatedSample = fileMetadata.toDatabaseSampleInfo(filePath, audioFile.getSize());
                updatedSample.id = existingSample->id; // Preserve database ID
                return database->updateSample(updatedSample);
            }
            else if (dbModTime > fileModTime)
            {
                // Database is newer - update file
                ChordMetadata dbMetadata = ChordMetadata::fromDatabaseSampleInfo(*existingSample);
                return writeMetadataToFile(audioFile, dbMetadata);
            }
            // If times are equal, no sync needed
            return true;
        }
        else
        {
            // Database has metadata, file doesn't - write to file
            ChordMetadata dbMetadata = ChordMetadata::fromDatabaseSampleInfo(*existingSample);
            return writeMetadataToFile(audioFile, dbMetadata);
        }
    }
    else
    {
        // File not in database
        if (hasFileMetadata)
        {
            // File has metadata - add to database
            auto sampleInfo = fileMetadata.toDatabaseSampleInfo(filePath, audioFile.getSize());
            int newId = database->insertSample(sampleInfo);
            return newId > 0;
        }
        else
        {
            // No metadata anywhere - try to parse from filename
            ChordParser parser;
            auto parsedData = parser.parseFilename(audioFile.getFileName());
            
            if (FilenameUtils::isValidParsedData(parsedData))
            {
                // Create metadata from parsed filename
                ChordMetadata newMetadata;
                newMetadata.rootNote = parsedData.rootNote;
                newMetadata.chordType = parsedData.standardizedQuality;
                newMetadata.chordTypeDisplay = parsedData.getFullChordName();
                newMetadata.extensions = parsedData.extensions;
                newMetadata.alterations = parsedData.alterations;
                newMetadata.addedNotes = parsedData.addedNotes;
                newMetadata.suspensions = parsedData.suspensions;
                newMetadata.bassNote = parsedData.determinedBassNote;
                newMetadata.inversion = parsedData.inversionTextParsed;
                newMetadata.originalFilename = audioFile.getFileName();
                newMetadata.dateAdded = juce::Time::getCurrentTime();
                newMetadata.dateModified = audioFile.getLastModificationTime();
                
                // Write to file and database
                bool fileWritten = writeMetadataToFile(audioFile, newMetadata);
                auto sampleInfo = newMetadata.toDatabaseSampleInfo(filePath, audioFile.getSize());
                int newId = database->insertSample(sampleInfo);
                
                return fileWritten && (newId > 0);
            }
        }
    }
    
    return false;
}

bool MetadataService::updateFileMetadata(const juce::File& audioFile, const ChordMetadata& metadata, ChopsDatabase* database)
{
    if (!database || !audioFile.existsAsFile())
        return false;
    
    // Write to file first
    ChordMetadata updatedMetadata = metadata;
    updatedMetadata.dateModified = juce::Time::getCurrentTime();
    
    bool fileSuccess = writeMetadataToFile(audioFile, updatedMetadata);
    if (!fileSuccess)
        return false;
    
    // Update database
    juce::String filePath = audioFile.getFullPathName();
    auto existingSample = database->getSampleByPath(filePath);
    
    if (existingSample)
    {
        // Update existing sample
        auto sampleInfo = updatedMetadata.toDatabaseSampleInfo(filePath, audioFile.getSize());
        sampleInfo.id = existingSample->id; // Preserve database ID
        return database->updateSample(sampleInfo);
    }
    else
    {
        // Insert new sample
        auto sampleInfo = updatedMetadata.toDatabaseSampleInfo(filePath, audioFile.getSize());
        int newId = database->insertSample(sampleInfo);
        return newId > 0;
    }
}

//==============================================================================
// iXML Implementation (WAV chunk handling)
//==============================================================================

bool MetadataService::readIXMLChunk(const juce::File& audioFile, juce::String& iXMLContent)
{
    if (!audioFile.existsAsFile())
    {
        juce::Logger::writeToLog("MetadataService: File does not exist: " + audioFile.getFullPathName());
        return false;
    }
    
    try
    {
        juce::FileInputStream inputStream(audioFile);
        if (!inputStream.openedOk())
        {
            juce::Logger::writeToLog("MetadataService: Could not open file for reading: " + audioFile.getFullPathName());
            return false;
        }
        
        // Read entire file into memory for easier parsing
        juce::MemoryBlock fileData;
        inputStream.readIntoMemoryBlock(fileData);
        // FileInputStream closes automatically when it goes out of scope
        
        if (fileData.getSize() < 12) // Minimum for RIFF header
        {
            juce::Logger::writeToLog("MetadataService: File too small to be a WAV file");
            return false;
        }
        
        const char* data = static_cast<const char*>(fileData.getData());
        size_t fileSize = fileData.getSize();
        
        // Verify RIFF/WAVE header
        if (strncmp(data, "RIFF", 4) != 0 || strncmp(data + 8, "WAVE", 4) != 0)
        {
            juce::Logger::writeToLog("MetadataService: Not a valid WAV file: " + audioFile.getFileName());
            return false;
        }
        
        // Search for iXML chunk
        size_t pos = 12; // Start after RIFF header
        
        while (pos + 8 <= fileSize) // Need at least 8 bytes for chunk header
        {
            const char* chunkId = data + pos;
            uint32_t chunkSize;
            memcpy(&chunkSize, data + pos + 4, 4);
            
            if (strncmp(chunkId, "iXML", 4) == 0)
            {
                // Found iXML chunk
                if (pos + 8 + chunkSize <= fileSize)
                {
                    const char* chunkData = data + pos + 8;
                    iXMLContent = juce::String::fromUTF8(chunkData, static_cast<int>(chunkSize));
                    
                    juce::Logger::writeToLog("MetadataService: Found iXML chunk (size: " + juce::String(chunkSize) + " bytes)");
                    return true;
                }
                else
                {
                    juce::Logger::writeToLog("MetadataService: iXML chunk extends beyond file boundary");
                    return false;
                }
            }
            
            // Move to next chunk
            pos += 8 + chunkSize;
            
            // Align to word boundary
            if (chunkSize % 2 != 0)
                pos++;
        }
        
        // No iXML chunk found
        return false;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("MetadataService: Exception reading iXML: " + juce::String(e.what()));
        return false;
    }
    catch (...)
    {
        juce::Logger::writeToLog("MetadataService: Unknown exception reading iXML");
        return false;
    }
}

bool MetadataService::writeIXMLChunk(const juce::File& audioFile, const juce::String& iXMLContent)
{
    if (iXMLContent.isEmpty())
    {
        juce::Logger::writeToLog("MetadataService: Cannot write empty iXML content");
        return false;
    }
    
    // Create backup
    juce::File backupFile = audioFile.withFileExtension(audioFile.getFileExtension() + ".backup");
    if (!audioFile.copyFileTo(backupFile))
    {
        juce::Logger::writeToLog("MetadataService: Could not create backup for: " + audioFile.getFileName());
        return false;
    }
    
    try
    {
        // Read original file into memory
        juce::FileInputStream inputStream(audioFile);
        if (!inputStream.openedOk())
        {
            juce::Logger::writeToLog("MetadataService: Could not open file for reading: " + audioFile.getFullPathName());
            backupFile.deleteFile();
            return false;
        }
        
        juce::MemoryBlock originalData;
        inputStream.readIntoMemoryBlock(originalData);
        // FileInputStream closes automatically when it goes out of scope
        
        if (originalData.getSize() < 44) // Minimum WAV header size
        {
            juce::Logger::writeToLog("MetadataService: File too small to be a valid WAV file");
            backupFile.deleteFile();
            return false;
        }
        
        // Verify it's a WAV file
        const char* data = static_cast<const char*>(originalData.getData());
        if (strncmp(data, "RIFF", 4) != 0 || strncmp(data + 8, "WAVE", 4) != 0)
        {
            juce::Logger::writeToLog("MetadataService: Not a valid WAV file");
            backupFile.deleteFile();
            return false;
        }
        
        // Prepare iXML chunk data
        juce::String iXMLUtf8 = iXMLContent.toUTF8();
        uint32_t iXMLSize = static_cast<uint32_t>(iXMLUtf8.getNumBytesAsUTF8());
        
        // Calculate total new file size
        size_t originalSize = originalData.getSize();
        size_t newFileSize = originalSize + 8 + iXMLSize + (iXMLSize % 2); // +8 for chunk header, +padding
        
        // Create new file content
        juce::MemoryBlock newData(newFileSize);
        
        // Copy original data
        newData.copyFrom(originalData.getData(), 0, originalSize);
        
        // Update RIFF chunk size in header
        uint32_t newRiffSize = static_cast<uint32_t>(newFileSize - 8);
        newData.copyFrom(&newRiffSize, 4, sizeof(newRiffSize));
        
        // Append iXML chunk
        char* writePtr = static_cast<char*>(newData.getData()) + originalSize;
        
        // iXML chunk ID
        memcpy(writePtr, "iXML", 4);
        writePtr += 4;
        
        // iXML chunk size
        memcpy(writePtr, &iXMLSize, 4);
        writePtr += 4;
        
        // iXML data
        memcpy(writePtr, iXMLUtf8.toUTF8().getAddress(), iXMLSize);
        writePtr += iXMLSize;
        
        // Padding byte if needed
        if (iXMLSize % 2 != 0)
        {
            *writePtr = 0;
        }
        
        // Write new file
        juce::FileOutputStream outputStream(audioFile);
        if (!outputStream.openedOk())
        {
            juce::Logger::writeToLog("MetadataService: Could not open file for writing");
            // Restore from backup
            backupFile.moveFileTo(audioFile);
            return false;
        }
        
        bool writeSuccess = outputStream.write(newData.getData(), newData.getSize());
        outputStream.flush();
        
        if (!writeSuccess)
        {
            juce::Logger::writeToLog("MetadataService: Failed to write new file content");
            // FileOutputStream closes automatically when it goes out of scope
            // Restore from backup
            audioFile.deleteFile();
            backupFile.moveFileTo(audioFile);
            return false;
        }
        
        // Delete backup on success
        backupFile.deleteFile();
        
        juce::Logger::writeToLog("MetadataService: Successfully wrote iXML chunk to: " + audioFile.getFileName());
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("MetadataService: Exception writing iXML: " + juce::String(e.what()));
        // Restore from backup on error
        if (backupFile.existsAsFile())
        {
            audioFile.deleteFile();
            backupFile.moveFileTo(audioFile);
        }
        return false;
    }
    catch (...)
    {
        juce::Logger::writeToLog("MetadataService: Unknown exception writing iXML");
        // Restore from backup on error
        if (backupFile.existsAsFile())
        {
            audioFile.deleteFile();
            backupFile.moveFileTo(audioFile);
        }
        return false;
    }
}

//==============================================================================
// Metadata Serialization
//==============================================================================

juce::String MetadataService::metadataToIXML(const ChordMetadata& metadata)
{
    juce::String xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<BWFXML>\n";
    xml << "  <CHOPS_METADATA version=\"1.0\">\n";
    
    // Chord information
    xml << "    <CHORD>\n";
    xml << "      <ROOT_NOTE>" << escapeXMLAttribute(metadata.rootNote) << "</ROOT_NOTE>\n";
    xml << "      <CHORD_TYPE>" << escapeXMLAttribute(metadata.chordType) << "</CHORD_TYPE>\n";
    xml << "      <CHORD_TYPE_DISPLAY>" << escapeXMLAttribute(metadata.chordTypeDisplay) << "</CHORD_TYPE_DISPLAY>\n";
    
    if (!metadata.extensions.isEmpty())
        xml << "      <EXTENSIONS>" << escapeXMLAttribute(metadata.extensions.joinIntoString(",")) << "</EXTENSIONS>\n";
    if (!metadata.alterations.isEmpty())
        xml << "      <ALTERATIONS>" << escapeXMLAttribute(metadata.alterations.joinIntoString(",")) << "</ALTERATIONS>\n";
    if (!metadata.addedNotes.isEmpty())
        xml << "      <ADDED_NOTES>" << escapeXMLAttribute(metadata.addedNotes.joinIntoString(",")) << "</ADDED_NOTES>\n";
    if (!metadata.suspensions.isEmpty())
        xml << "      <SUSPENSIONS>" << escapeXMLAttribute(metadata.suspensions.joinIntoString(",")) << "</SUSPENSIONS>\n";
    
    if (metadata.bassNote.isNotEmpty())
        xml << "      <BASS_NOTE>" << escapeXMLAttribute(metadata.bassNote) << "</BASS_NOTE>\n";
    if (metadata.inversion.isNotEmpty())
        xml << "      <INVERSION>" << escapeXMLAttribute(metadata.inversion) << "</INVERSION>\n";
    xml << "    </CHORD>\n";
    
    // User metadata
    xml << "    <USER_DATA>\n";
    if (!metadata.tags.isEmpty())
        xml << "      <TAGS>" << escapeXMLAttribute(metadata.tags.joinIntoString(",")) << "</TAGS>\n";
    xml << "      <RATING>" << metadata.rating << "</RATING>\n";
    xml << "      <IS_FAVORITE>" << (metadata.isFavorite ? "true" : "false") << "</IS_FAVORITE>\n";
    if (metadata.userNotes.isNotEmpty())
        xml << "      <USER_NOTES>" << escapeXMLAttribute(metadata.userNotes) << "</USER_NOTES>\n";
    xml << "      <COLOR>" << escapeXMLAttribute(metadata.color.toDisplayString(true)) << "</COLOR>\n";
    xml << "      <PLAY_COUNT>" << metadata.playCount << "</PLAY_COUNT>\n";
    if (metadata.lastPlayed.toMilliseconds() > 0)
        xml << "      <LAST_PLAYED>" << escapeXMLAttribute(metadata.lastPlayed.toISO8601(true)) << "</LAST_PLAYED>\n";
    xml << "    </USER_DATA>\n";
    
    // System metadata
    xml << "    <SYSTEM_DATA>\n";
    if (metadata.originalFilename.isNotEmpty())
        xml << "      <ORIGINAL_FILENAME>" << escapeXMLAttribute(metadata.originalFilename) << "</ORIGINAL_FILENAME>\n";
    if (metadata.dateAdded.toMilliseconds() > 0)
        xml << "      <DATE_ADDED>" << escapeXMLAttribute(metadata.dateAdded.toISO8601(true)) << "</DATE_ADDED>\n";
    if (metadata.dateModified.toMilliseconds() > 0)
        xml << "      <DATE_MODIFIED>" << escapeXMLAttribute(metadata.dateModified.toISO8601(true)) << "</DATE_MODIFIED>\n";
    xml << "    </SYSTEM_DATA>\n";
    
    xml << "  </CHOPS_METADATA>\n";
    xml << "</BWFXML>\n";
    
    return xml;
}

bool MetadataService::iXMLToMetadata(const juce::String& iXMLContent, ChordMetadata& metadata)
{
    std::unique_ptr<juce::XmlElement> xmlDoc(juce::XmlDocument::parse(iXMLContent));
    if (!xmlDoc)
        return false;
    
    auto chopsMetadata = xmlDoc->getChildByName("CHOPS_METADATA");
    if (!chopsMetadata)
        return false;
    
    // Read chord information
    auto chordElement = chopsMetadata->getChildByName("CHORD");
    if (chordElement)
    {
        metadata.rootNote = chordElement->getChildElementAllSubText("ROOT_NOTE", "");
        metadata.chordType = chordElement->getChildElementAllSubText("CHORD_TYPE", "");
        metadata.chordTypeDisplay = chordElement->getChildElementAllSubText("CHORD_TYPE_DISPLAY", "");
        metadata.bassNote = chordElement->getChildElementAllSubText("BASS_NOTE", "");
        metadata.inversion = chordElement->getChildElementAllSubText("INVERSION", "");
        
        // Parse arrays
        juce::String extensionsStr = chordElement->getChildElementAllSubText("EXTENSIONS", "");
        if (extensionsStr.isNotEmpty())
            metadata.extensions = juce::StringArray::fromTokens(extensionsStr, ",", "");
        
        juce::String alterationsStr = chordElement->getChildElementAllSubText("ALTERATIONS", "");
        if (alterationsStr.isNotEmpty())
            metadata.alterations = juce::StringArray::fromTokens(alterationsStr, ",", "");
        
        juce::String addedNotesStr = chordElement->getChildElementAllSubText("ADDED_NOTES", "");
        if (addedNotesStr.isNotEmpty())
            metadata.addedNotes = juce::StringArray::fromTokens(addedNotesStr, ",", "");
        
        juce::String suspensionsStr = chordElement->getChildElementAllSubText("SUSPENSIONS", "");
        if (suspensionsStr.isNotEmpty())
            metadata.suspensions = juce::StringArray::fromTokens(suspensionsStr, ",", "");
    }
    
    // Read user metadata
    auto userElement = chopsMetadata->getChildByName("USER_DATA");
    if (userElement)
    {
        juce::String tagsStr = userElement->getChildElementAllSubText("TAGS", "");
        if (tagsStr.isNotEmpty())
            metadata.tags = juce::StringArray::fromTokens(tagsStr, ",", "");
        
        metadata.rating = userElement->getChildElementAllSubText("RATING", "0").getIntValue();
        metadata.isFavorite = userElement->getChildElementAllSubText("IS_FAVORITE", "false") == "true";
        metadata.userNotes = userElement->getChildElementAllSubText("USER_NOTES", "");
        metadata.playCount = userElement->getChildElementAllSubText("PLAY_COUNT", "0").getIntValue();
        
        juce::String colorStr = userElement->getChildElementAllSubText("COLOR", "");
        if (colorStr.isNotEmpty())
            metadata.color = juce::Colour::fromString(colorStr);
        
        juce::String lastPlayedStr = userElement->getChildElementAllSubText("LAST_PLAYED", "");
        if (lastPlayedStr.isNotEmpty())
            metadata.lastPlayed = juce::Time::fromISO8601(lastPlayedStr);
    }
    
    // Read system metadata
    auto systemElement = chopsMetadata->getChildByName("SYSTEM_DATA");
    if (systemElement)
    {
        metadata.originalFilename = systemElement->getChildElementAllSubText("ORIGINAL_FILENAME", "");
        
        juce::String dateAddedStr = systemElement->getChildElementAllSubText("DATE_ADDED", "");
        if (dateAddedStr.isNotEmpty())
            metadata.dateAdded = juce::Time::fromISO8601(dateAddedStr);
        
        juce::String dateModifiedStr = systemElement->getChildElementAllSubText("DATE_MODIFIED", "");
        if (dateModifiedStr.isNotEmpty())
            metadata.dateModified = juce::Time::fromISO8601(dateModifiedStr);
    }
    
    return metadata.isValid();
}

//==============================================================================
// Batch Operations
//==============================================================================

MetadataService::ScanResult MetadataService::scanAndSyncDirectory(const juce::File& directory, ChopsDatabase* database, 
                                                                  bool recursive, bool writeMetadataToFiles)
{
    ScanResult result;
    
    if (!directory.isDirectory() || !database)
    {
        result.errorMessages.add("Invalid directory or database");
        result.errors++;
        return result;
    }
    
    juce::Array<juce::File> audioFiles;
    // FIXED: Use correct JUCE API for recursive file search
    directory.findChildFiles(audioFiles, juce::File::findFiles, recursive, "*");
    
    for (const auto& file : audioFiles)
    {
        if (!isAudioFile(file))
            continue;
        
        result.filesProcessed++;
        
        try
        {
            ChordMetadata metadata;
            bool hasMetadata = readMetadataFromFile(file, metadata);
            
            if (hasMetadata)
            {
                result.filesWithMetadata++;
            }
            else
            {
                result.filesWithoutMetadata++;
                
                if (writeMetadataToFiles)
                {
                    // Try to generate metadata from filename
                    ChordParser parser;
                    auto parsedData = parser.parseFilename(file.getFileName());
                    
                    if (FilenameUtils::isValidParsedData(parsedData))
                    {
                        ChordMetadata newMetadata;
                        newMetadata.rootNote = parsedData.rootNote;
                        newMetadata.chordType = parsedData.standardizedQuality;
                        newMetadata.chordTypeDisplay = parsedData.getFullChordName();
                        newMetadata.extensions = parsedData.extensions;
                        newMetadata.alterations = parsedData.alterations;
                        newMetadata.addedNotes = parsedData.addedNotes;
                        newMetadata.suspensions = parsedData.suspensions;
                        newMetadata.bassNote = parsedData.determinedBassNote;
                        newMetadata.inversion = parsedData.inversionTextParsed;
                        newMetadata.originalFilename = file.getFileName();
                        newMetadata.dateAdded = juce::Time::getCurrentTime();
                        newMetadata.dateModified = file.getLastModificationTime();
                        
                        if (writeMetadataToFile(file, newMetadata))
                        {
                            result.metadataWritten++;
                            hasMetadata = true;
                            metadata = newMetadata;
                        }
                    }
                }
            }
            
            // Sync with database
            if (hasMetadata && syncFileWithDatabase(file, database))
            {
                result.databaseUpdated++;
            }
        }
        catch (const std::exception& e)
        {
            result.errors++;
            result.errorMessages.add("Error processing " + file.getFileName() + ": " + e.what());
        }
    }
    
    return result;
}

bool MetadataService::migrateFromFilenameToMetadata(const juce::File& audioFile, ChopsDatabase* database)
{
    if (!audioFile.existsAsFile() || !database)
        return false;
    
    // Check if file already has metadata
    if (hasMetadata(audioFile))
        return true; // Already migrated
    
    // Parse filename
    ChordParser parser;
    auto parsedData = parser.parseFilename(audioFile.getFileName());
    
    if (!FilenameUtils::isValidParsedData(parsedData))
        return false;
    
    // Create metadata from parsed data
    ChordMetadata metadata;
    metadata.rootNote = parsedData.rootNote;
    metadata.chordType = parsedData.standardizedQuality;
    metadata.chordTypeDisplay = parsedData.getFullChordName();
    metadata.extensions = parsedData.extensions;
    metadata.alterations = parsedData.alterations;
    metadata.addedNotes = parsedData.addedNotes;
    metadata.suspensions = parsedData.suspensions;
    metadata.bassNote = parsedData.determinedBassNote;
    metadata.inversion = parsedData.inversionTextParsed;
    metadata.originalFilename = audioFile.getFileName();
    metadata.dateAdded = juce::Time::getCurrentTime();
    metadata.dateModified = audioFile.getLastModificationTime();
    
    // Write to file and sync with database
    return updateFileMetadata(audioFile, metadata, database);
}

MetadataService::ScanResult MetadataService::migrateEntireLibrary(const juce::File& libraryRoot, ChopsDatabase* database)
{
    ScanResult result;
    
    if (!libraryRoot.isDirectory() || !database)
    {
        result.errorMessages.add("Invalid library root or database");
        result.errors++;
        return result;
    }
    
    juce::Array<juce::File> audioFiles;
    // FIXED: Use correct JUCE API for recursive file search
    libraryRoot.findChildFiles(audioFiles, juce::File::findFiles, true, "*");
    
    for (const auto& file : audioFiles)
    {
        if (!isAudioFile(file))
            continue;
        
        result.filesProcessed++;
        
        try
        {
            if (!hasMetadata(file))
            {
                result.filesWithoutMetadata++;
                
                if (migrateFromFilenameToMetadata(file, database))
                {
                    result.metadataWritten++;
                    result.databaseUpdated++;
                    result.filesWithMetadata++;
                }
                else
                {
                    result.errors++;
                    result.errorMessages.add("Failed to migrate: " + file.getFileName());
                }
            }
            else
            {
                result.filesWithMetadata++;
            }
        }
        catch (const std::exception& e)
        {
            result.errors++;
            result.errorMessages.add("Error migrating " + file.getFileName() + ": " + e.what());
        }
    }
    
    return result;
}

//==============================================================================
// Validation and Repair
//==============================================================================

bool MetadataService::validateFileMetadata(const juce::File& audioFile, juce::StringArray& issues)
{
    issues.clear();
    
    if (!audioFile.existsAsFile())
    {
        issues.add("File does not exist");
        return false;
    }
    
    if (!isAudioFile(audioFile))
    {
        issues.add("Not an audio file");
        return false;
    }
    
    ChordMetadata metadata;
    if (!readMetadataFromFile(audioFile, metadata))
    {
        issues.add("No metadata found");
        return false;
    }
    
    if (!metadata.isValid())
    {
        issues.add("Invalid metadata structure");
        return false;
    }
    
    // Additional validation checks
    if (metadata.rootNote.isEmpty())
        issues.add("Missing root note");
    
    if (metadata.chordType.isEmpty())
        issues.add("Missing chord type");
    
    if (metadata.rating < 0 || metadata.rating > 5)
        issues.add("Invalid rating value");
    
    return issues.isEmpty();
}

bool MetadataService::repairMetadata(const juce::File& audioFile, ChopsDatabase* database)
{
    if (!audioFile.existsAsFile() || !database)
        return false;
    
    juce::StringArray issues;
    if (validateFileMetadata(audioFile, issues))
        return true; // No repair needed
    
    // Try to repair by re-parsing filename
    ChordParser parser;
    auto parsedData = parser.parseFilename(audioFile.getFileName());
    
    if (!FilenameUtils::isValidParsedData(parsedData))
        return false;
    
    // Create new metadata
    ChordMetadata repairedMetadata;
    repairedMetadata.rootNote = parsedData.rootNote;
    repairedMetadata.chordType = parsedData.standardizedQuality;
    repairedMetadata.chordTypeDisplay = parsedData.getFullChordName();
    repairedMetadata.extensions = parsedData.extensions;
    repairedMetadata.alterations = parsedData.alterations;
    repairedMetadata.addedNotes = parsedData.addedNotes;
    repairedMetadata.suspensions = parsedData.suspensions;
    repairedMetadata.bassNote = parsedData.determinedBassNote;
    repairedMetadata.inversion = parsedData.inversionTextParsed;
    repairedMetadata.originalFilename = audioFile.getFileName();
    repairedMetadata.dateAdded = juce::Time::getCurrentTime();
    repairedMetadata.dateModified = audioFile.getLastModificationTime();
    
    // Preserve existing user metadata if possible
    ChordMetadata existingMetadata;
    if (readMetadataFromFile(audioFile, existingMetadata))
    {
        repairedMetadata.tags = existingMetadata.tags;
        repairedMetadata.rating = existingMetadata.rating;
        repairedMetadata.isFavorite = existingMetadata.isFavorite;
        repairedMetadata.userNotes = existingMetadata.userNotes;
        repairedMetadata.color = existingMetadata.color;
        repairedMetadata.playCount = existingMetadata.playCount;
        repairedMetadata.lastPlayed = existingMetadata.lastPlayed;
        repairedMetadata.dateAdded = existingMetadata.dateAdded;
    }
    
    return updateFileMetadata(audioFile, repairedMetadata, database);
}

//==============================================================================
// Helper Methods
//==============================================================================

bool MetadataService::isAudioFile(const juce::File& file)
{
    return FilenameUtils::isAudioFile(file);
}

juce::String MetadataService::sanitizeXMLString(const juce::String& input)
{
    return input.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;")
                .replace("\"", "&quot;")
                .replace("'", "&apos;");
}

juce::String MetadataService::escapeXMLAttribute(const juce::String& input)
{
    return sanitizeXMLString(input);
}
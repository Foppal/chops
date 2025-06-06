#include "MetadataServiceTest.h"
#include <fstream>
#include <iostream>
#include <cmath>  // For M_PI and sin

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//==============================================================================
MetadataServiceTest::MetadataServiceTest()
{
}

//==============================================================================
bool MetadataServiceTest::runAllTests(const juce::File& testDirectory)
{
    juce::Logger::writeToLog("=== METADATA SERVICE TEST SUITE ===");
    juce::Logger::writeToLog("Test directory: " + testDirectory.getFullPathName());
    
    if (!testDirectory.isDirectory())
    {
        if (!testDirectory.createDirectory())
        {
            juce::Logger::writeToLog("❌ FATAL: Could not create test directory");
            return false;
        }
    }
    
    // Try to find a real WAV file first
    juce::File realWavFile = findExistingWavFile();
    if (realWavFile.existsAsFile())
    {
        juce::Logger::writeToLog("✅ Found existing WAV file for testing: " + realWavFile.getFullPathName());
        juce::Logger::writeToLog("   File size: " + juce::String(realWavFile.getSize()) + " bytes");
    }
    else
    {
        juce::Logger::writeToLog("⚠️ No existing WAV file found, will create test file");
    }
    
    bool allTestsPassed = true;
    int totalTests = 0;
    int passedTests = 0;
    
    // Run all tests
    std::vector<TestResult> results;
    
    juce::Logger::writeToLog("\n=== RUNNING BASIC METADATA TEST ===");
    results.push_back(testBasicMetadataWriteReadDetailed(testDirectory, realWavFile));
    
    juce::Logger::writeToLog("\n=== RUNNING COMPLEX METADATA TEST ===");
    results.push_back(testComplexMetadataWriteReadDetailed(testDirectory, realWavFile));
    
    juce::Logger::writeToLog("\n=== RUNNING FILE WITHOUT METADATA TEST ===");
    results.push_back(testFileWithoutMetadataDetailed(testDirectory));
    
    juce::Logger::writeToLog("\n=== RUNNING INVALID FILE TEST ===");
    results.push_back(testInvalidFileDetailed(testDirectory));
    
    juce::Logger::writeToLog("\n=== RUNNING METADATA UPDATE TEST ===");
    results.push_back(testMetadataUpdateDetailed(testDirectory, realWavFile));
    
    // Test WAV file validation
    juce::Logger::writeToLog("\n=== RUNNING WAV FILE VALIDATION TEST ===");
    results.push_back(testWavFileValidation(testDirectory));
    
    // Report results
    juce::Logger::writeToLog("\n=== TEST RESULTS SUMMARY ===");
    for (const auto& result : results)
    {
        totalTests++;
        if (result.success)
        {
            passedTests++;
        }
        else
        {
            allTestsPassed = false;
        }
        
        juce::Logger::writeToLog(result.toString());
    }
    
    juce::Logger::writeToLog("\n=== FINAL SUMMARY ===");
    juce::Logger::writeToLog(juce::String::formatted("Tests: %d/%d passed", passedTests, totalTests));
    juce::Logger::writeToLog("Overall result: " + juce::String(allTestsPassed ? "✅ ALL TESTS PASSED" : "❌ SOME TESTS FAILED"));
    
    return allTestsPassed;
}

//==============================================================================
MetadataServiceTest::TestResult MetadataServiceTest::testBasicMetadataWriteReadDetailed(const juce::File& testDirectory, const juce::File& existingWavFile)
{
    TestResult result;
    result.message = "Basic metadata write/read test (detailed)";
    
    juce::Logger::writeToLog("📝 Starting basic metadata test...");
    
    try
    {
        // Determine which file to use
        juce::File testWav;
        bool usingExistingFile = false;
        
        if (existingWavFile.existsAsFile())
        {
            // Copy existing file to test directory
            testWav = testDirectory.getChildFile("basic_test_copy.wav");
            juce::Logger::writeToLog("📂 Copying existing WAV file to test directory...");
            juce::Logger::writeToLog("   Source: " + existingWavFile.getFullPathName());
            juce::Logger::writeToLog("   Target: " + testWav.getFullPathName());
            
            if (!existingWavFile.copyFileTo(testWav))
            {
                result.details = "Failed to copy existing WAV file to test directory";
                juce::Logger::writeToLog("❌ Copy failed!");
                return result;
            }
            
            usingExistingFile = true;
            juce::Logger::writeToLog("✅ Successfully copied file (size: " + juce::String(testWav.getSize()) + " bytes)");
        }
        else
        {
            // Create a test WAV file
            juce::Logger::writeToLog("🏗️ Creating new test WAV file...");
            testWav = createTestWavFileDetailed(testDirectory, "basic_test.wav");
            if (!testWav.existsAsFile())
            {
                result.details = "Failed to create test WAV file";
                juce::Logger::writeToLog("❌ WAV file creation failed!");
                return result;
            }
            juce::Logger::writeToLog("✅ Created test WAV file (size: " + juce::String(testWav.getSize()) + " bytes)");
        }
        
        // Validate the WAV file first
        juce::Logger::writeToLog("🔍 Validating WAV file structure...");
        if (!validateWavFileStructure(testWav))
        {
            result.details = "WAV file validation failed";
            return result;
        }
        juce::Logger::writeToLog("✅ WAV file structure is valid");
        
        // Check if file already has metadata
        juce::Logger::writeToLog("🔍 Checking for existing metadata...");
        MetadataService::ChordMetadata existingMetadata;
        bool hasExistingMetadata = metadataService.readMetadataFromFile(testWav, existingMetadata);
        
        if (hasExistingMetadata)
        {
            juce::Logger::writeToLog("⚠️ File already has metadata:");
            juce::Logger::writeToLog("   " + existingMetadata.toString());
        }
        else
        {
            juce::Logger::writeToLog("✅ File has no existing metadata (as expected)");
        }
        
        // Create test metadata
        juce::Logger::writeToLog("📋 Creating test metadata...");
        auto originalMetadata = createTestMetadata();
        juce::Logger::writeToLog("   Created metadata: " + originalMetadata.toString());
        
        // Write metadata
        juce::Logger::writeToLog("📝 Writing metadata to file...");
        juce::Logger::writeToLog("   File before write: " + juce::String(testWav.getSize()) + " bytes");
        
        bool writeSuccess = metadataService.writeMetadataToFile(testWav, originalMetadata);
        if (!writeSuccess)
        {
            result.details = "Failed to write metadata to file: " + testWav.getFullPathName();
            juce::Logger::writeToLog("❌ Write failed!");
            return result;
        }
        
        juce::Logger::writeToLog("✅ Metadata written successfully");
        juce::Logger::writeToLog("   File after write: " + juce::String(testWav.getSize()) + " bytes");
        
        // Verify file still exists and is readable
        if (!testWav.existsAsFile())
        {
            result.details = "File disappeared after writing metadata!";
            juce::Logger::writeToLog("❌ File disappeared after write!");
            return result;
        }
        
        // Read metadata back
        juce::Logger::writeToLog("📖 Reading metadata from file...");
        MetadataService::ChordMetadata readMetadata;
        bool readSuccess = metadataService.readMetadataFromFile(testWav, readMetadata);
        if (!readSuccess)
        {
            result.details = "Failed to read metadata from file after writing";
            juce::Logger::writeToLog("❌ Read failed!");
            
            // Additional debugging
            juce::Logger::writeToLog("🔍 Debugging read failure...");
            juce::Logger::writeToLog("   File exists: " + juce::String(testWav.existsAsFile() ? "YES" : "NO"));
            juce::Logger::writeToLog("   File size: " + juce::String(testWav.getSize()) + " bytes");
            juce::Logger::writeToLog("   File readable: " + juce::String(testWav.hasReadAccess() ? "YES" : "NO"));
            
            return result;
        }
        
        juce::Logger::writeToLog("✅ Successfully read metadata back");
        juce::Logger::writeToLog("   Read metadata: " + readMetadata.toString());
        
        // Compare metadata
        juce::Logger::writeToLog("🔍 Comparing original and read metadata...");
        juce::String differences;
        if (!compareMetadataDetailed(originalMetadata, readMetadata, differences))
        {
            result.details = "Metadata doesn't match: " + differences;
            juce::Logger::writeToLog("❌ Metadata comparison failed!");
            juce::Logger::writeToLog("   Differences: " + differences);
            return result;
        }
        
        juce::Logger::writeToLog("✅ Metadata matches perfectly!");
        
        result.success = true;
        result.details = "Successfully wrote and read back metadata using " + 
                        juce::String(usingExistingFile ? "existing" : "generated") + " WAV file";
        
        juce::Logger::writeToLog("🎉 Basic metadata test PASSED!");
    }
    catch (const std::exception& e)
    {
        result.details = "Exception: " + juce::String(e.what());
        juce::Logger::writeToLog("❌ Exception in basic test: " + juce::String(e.what()));
    }
    catch (...)
    {
        result.details = "Unknown exception occurred";
        juce::Logger::writeToLog("❌ Unknown exception in basic test");
    }
    
    return result;
}

MetadataServiceTest::TestResult MetadataServiceTest::testComplexMetadataWriteReadDetailed(const juce::File& testDirectory, const juce::File& existingWavFile)
{
    TestResult result;
    result.message = "Complex metadata write/read test (detailed)";
    
    juce::Logger::writeToLog("📝 Starting complex metadata test...");
    
    try
    {
        // Use existing file if available, otherwise create one
        juce::File testWav;
        if (existingWavFile.existsAsFile())
        {
            testWav = testDirectory.getChildFile("complex_test_copy.wav");
            juce::Logger::writeToLog("📂 Copying existing WAV file for complex test...");
            if (!existingWavFile.copyFileTo(testWav))
            {
                result.details = "Failed to copy existing WAV file";
                return result;
            }
            juce::Logger::writeToLog("✅ File copied successfully");
        }
        else
        {
            testWav = createTestWavFileDetailed(testDirectory, "complex_test.wav");
            if (!testWav.existsAsFile())
            {
                result.details = "Failed to create test WAV file";
                return result;
            }
        }
        
        // Create complex test metadata
        juce::Logger::writeToLog("📋 Creating complex test metadata...");
        auto originalMetadata = createComplexTestMetadata();
        juce::Logger::writeToLog("   Complex metadata created with " + juce::String(originalMetadata.tags.size()) + " tags");
        
        // Write metadata
        juce::Logger::writeToLog("📝 Writing complex metadata...");
        bool writeSuccess = metadataService.writeMetadataToFile(testWav, originalMetadata);
        if (!writeSuccess)
        {
            result.details = "Failed to write complex metadata to file";
            juce::Logger::writeToLog("❌ Complex metadata write failed!");
            return result;
        }
        juce::Logger::writeToLog("✅ Complex metadata written");
        
        // Read metadata back
        juce::Logger::writeToLog("📖 Reading complex metadata back...");
        MetadataService::ChordMetadata readMetadata;
        bool readSuccess = metadataService.readMetadataFromFile(testWav, readMetadata);
        if (!readSuccess)
        {
            result.details = "Failed to read complex metadata from file";
            juce::Logger::writeToLog("❌ Complex metadata read failed!");
            return result;
        }
        juce::Logger::writeToLog("✅ Complex metadata read back with " + juce::String(readMetadata.tags.size()) + " tags");
        
        // Compare metadata
        juce::String differences;
        if (!compareMetadataDetailed(originalMetadata, readMetadata, differences))
        {
            result.details = "Complex metadata doesn't match: " + differences;
            juce::Logger::writeToLog("❌ Complex metadata comparison failed: " + differences);
            return result;
        }
        
        result.success = true;
        result.details = "Successfully wrote and read back complex metadata";
        juce::Logger::writeToLog("🎉 Complex metadata test PASSED!");
    }
    catch (const std::exception& e)
    {
        result.details = "Exception: " + juce::String(e.what());
        juce::Logger::writeToLog("❌ Exception in complex test: " + juce::String(e.what()));
    }
    catch (...)
    {
        result.details = "Unknown exception occurred";
        juce::Logger::writeToLog("❌ Unknown exception in complex test");
    }
    
    return result;
}

MetadataServiceTest::TestResult MetadataServiceTest::testFileWithoutMetadataDetailed(const juce::File& testDirectory)
{
    TestResult result;
    result.message = "File without metadata test (detailed)";
    
    juce::Logger::writeToLog("📝 Testing file without metadata...");
    
    try
    {
        // Create a fresh test WAV file
        juce::File testWav = createTestWavFileDetailed(testDirectory, "no_metadata_test.wav");
        if (!testWav.existsAsFile())
        {
            result.details = "Failed to create test WAV file";
            return result;
        }
        juce::Logger::writeToLog("✅ Created fresh WAV file for no-metadata test");
        
        // Try to read metadata (should fail gracefully)
        juce::Logger::writeToLog("🔍 Attempting to read metadata from fresh file...");
        MetadataService::ChordMetadata metadata;
        bool hasMetadata = metadataService.readMetadataFromFile(testWav, metadata);
        
        if (hasMetadata)
        {
            result.details = "Expected no metadata, but found some: " + metadata.toString();
            juce::Logger::writeToLog("❌ Unexpectedly found metadata: " + metadata.toString());
            return result;
        }
        juce::Logger::writeToLog("✅ Correctly found no metadata in fresh file");
        
        // Check hasMetadata method
        juce::Logger::writeToLog("🔍 Testing hasMetadata() method...");
        bool hasMetadataCheck = metadataService.hasMetadata(testWav);
        if (hasMetadataCheck)
        {
            result.details = "hasMetadata() returned true for file without metadata";
            juce::Logger::writeToLog("❌ hasMetadata() incorrectly returned true");
            return result;
        }
        juce::Logger::writeToLog("✅ hasMetadata() correctly returned false");
        
        result.success = true;
        result.details = "Correctly detected file without metadata";
        juce::Logger::writeToLog("🎉 No-metadata test PASSED!");
    }
    catch (const std::exception& e)
    {
        result.details = "Exception: " + juce::String(e.what());
        juce::Logger::writeToLog("❌ Exception in no-metadata test: " + juce::String(e.what()));
    }
    catch (...)
    {
        result.details = "Unknown exception occurred";
        juce::Logger::writeToLog("❌ Unknown exception in no-metadata test");
    }
    
    return result;
}

MetadataServiceTest::TestResult MetadataServiceTest::testInvalidFileDetailed(const juce::File& testDirectory)
{
    TestResult result;
    result.message = "Invalid file test (detailed)";
    
    juce::Logger::writeToLog("📝 Testing invalid file handling...");
    
    try
    {
        // Create a non-WAV file
        juce::File textFile = testDirectory.getChildFile("not_a_wav.txt");
        juce::Logger::writeToLog("📄 Creating text file: " + textFile.getFullPathName());
        textFile.replaceWithText("This is not a WAV file - it's just text content for testing");
        
        // Try to read metadata (should fail gracefully)
        juce::Logger::writeToLog("🔍 Attempting to read metadata from text file...");
        MetadataService::ChordMetadata metadata;
        bool hasMetadata = metadataService.readMetadataFromFile(textFile, metadata);
        
        if (hasMetadata)
        {
            result.details = "Expected no metadata from text file, but found some";
            juce::Logger::writeToLog("❌ Unexpectedly found metadata in text file");
            return result;
        }
        juce::Logger::writeToLog("✅ Correctly rejected text file");
        
        // Try to write metadata (should fail gracefully)
        juce::Logger::writeToLog("📝 Attempting to write metadata to text file...");
        auto testMetadata = createTestMetadata();
        bool writeSuccess = metadataService.writeMetadataToFile(textFile, testMetadata);
        
        if (writeSuccess)
        {
            result.details = "Expected write to fail for text file, but it succeeded";
            juce::Logger::writeToLog("❌ Write to text file unexpectedly succeeded");
            return result;
        }
        juce::Logger::writeToLog("✅ Correctly rejected write to text file");
        
        // Test with non-existent file
        juce::File nonExistent = testDirectory.getChildFile("does_not_exist.wav");
        juce::Logger::writeToLog("🔍 Testing non-existent file: " + nonExistent.getFullPathName());
        bool readNonExistent = metadataService.readMetadataFromFile(nonExistent, metadata);
        
        if (readNonExistent)
        {
            result.details = "Expected read to fail for non-existent file, but it succeeded";
            juce::Logger::writeToLog("❌ Read from non-existent file unexpectedly succeeded");
            return result;
        }
        juce::Logger::writeToLog("✅ Correctly rejected non-existent file");
        
        result.success = true;
        result.details = "Correctly handled invalid files";
        juce::Logger::writeToLog("🎉 Invalid file test PASSED!");
    }
    catch (const std::exception& e)
    {
        result.details = "Exception: " + juce::String(e.what());
        juce::Logger::writeToLog("❌ Exception in invalid file test: " + juce::String(e.what()));
    }
    catch (...)
    {
        result.details = "Unknown exception occurred";
        juce::Logger::writeToLog("❌ Unknown exception in invalid file test");
    }
    
    return result;
}

MetadataServiceTest::TestResult MetadataServiceTest::testMetadataUpdateDetailed(const juce::File& testDirectory, const juce::File& existingWavFile)
{
    TestResult result;
    result.message = "Metadata update test (detailed)";
    
    juce::Logger::writeToLog("📝 Testing metadata updates...");
    
    try
    {
        // Use existing file if available
        juce::File testWav;
        if (existingWavFile.existsAsFile())
        {
            testWav = testDirectory.getChildFile("update_test_copy.wav");
            if (!existingWavFile.copyFileTo(testWav))
            {
                result.details = "Failed to copy existing WAV file";
                return result;
            }
        }
        else
        {
            testWav = createTestWavFileDetailed(testDirectory, "update_test.wav");
            if (!testWav.existsAsFile())
            {
                result.details = "Failed to create test WAV file";
                return result;
            }
        }
        
        // Write initial metadata
        juce::Logger::writeToLog("📝 Writing initial metadata...");
        auto originalMetadata = createTestMetadata();
        juce::Logger::writeToLog("   Initial rating: " + juce::String(originalMetadata.rating));
        juce::Logger::writeToLog("   Initial favorite: " + juce::String(originalMetadata.isFavorite ? "true" : "false"));
        
        bool writeSuccess = metadataService.writeMetadataToFile(testWav, originalMetadata);
        if (!writeSuccess)
        {
            result.details = "Failed to write initial metadata";
            juce::Logger::writeToLog("❌ Initial metadata write failed");
            return result;
        }
        juce::Logger::writeToLog("✅ Initial metadata written");
        
        // Update metadata
        juce::Logger::writeToLog("🔄 Updating metadata...");
        auto updatedMetadata = originalMetadata;
        updatedMetadata.rating = 5;
        updatedMetadata.isFavorite = true;
        updatedMetadata.tags.add("updated");
        updatedMetadata.userNotes = "This metadata was updated in test";
        updatedMetadata.dateModified = juce::Time::getCurrentTime();
        
        juce::Logger::writeToLog("   Updated rating: " + juce::String(updatedMetadata.rating));
        juce::Logger::writeToLog("   Updated favorite: " + juce::String(updatedMetadata.isFavorite ? "true" : "false"));
        juce::Logger::writeToLog("   Added tag: 'updated'");
        
        // Write updated metadata
        bool updateSuccess = metadataService.writeMetadataToFile(testWav, updatedMetadata);
        if (!updateSuccess)
        {
            result.details = "Failed to write updated metadata";
            juce::Logger::writeToLog("❌ Updated metadata write failed");
            return result;
        }
        juce::Logger::writeToLog("✅ Updated metadata written");
        
        // Read back updated metadata
        juce::Logger::writeToLog("📖 Reading back updated metadata...");
        MetadataService::ChordMetadata readMetadata;
        bool readSuccess = metadataService.readMetadataFromFile(testWav, readMetadata);
        if (!readSuccess)
        {
            result.details = "Failed to read updated metadata";
            juce::Logger::writeToLog("❌ Updated metadata read failed");
            return result;
        }
        
        // Verify specific updates
        juce::Logger::writeToLog("🔍 Verifying updates...");
        
        if (readMetadata.rating != 5)
        {
            result.details = "Rating was not updated correctly (expected 5, got " + juce::String(readMetadata.rating) + ")";
            juce::Logger::writeToLog("❌ Rating update failed: expected 5, got " + juce::String(readMetadata.rating));
            return result;
        }
        juce::Logger::writeToLog("✅ Rating correctly updated to 5");
        
        if (!readMetadata.isFavorite)
        {
            result.details = "Favorite flag was not updated correctly";
            juce::Logger::writeToLog("❌ Favorite flag update failed");
            return result;
        }
        juce::Logger::writeToLog("✅ Favorite flag correctly updated to true");
        
        if (!readMetadata.tags.contains("updated"))
        {
            result.details = "Tags were not updated correctly (missing 'updated' tag)";
            juce::Logger::writeToLog("❌ Tag update failed - 'updated' tag not found");
            juce::Logger::writeToLog("   Available tags: " + readMetadata.tags.joinIntoString(", "));
            return result;
        }
        juce::Logger::writeToLog("✅ Tag 'updated' correctly added");
        
        if (readMetadata.userNotes != "This metadata was updated in test")
        {
            result.details = "User notes were not updated correctly";
            juce::Logger::writeToLog("❌ User notes update failed");
            juce::Logger::writeToLog("   Expected: 'This metadata was updated in test'");
            juce::Logger::writeToLog("   Got: '" + readMetadata.userNotes + "'");
            return result;
        }
        juce::Logger::writeToLog("✅ User notes correctly updated");
        
        result.success = true;
        result.details = "Successfully updated metadata";
        juce::Logger::writeToLog("🎉 Metadata update test PASSED!");
    }
    catch (const std::exception& e)
    {
        result.details = "Exception: " + juce::String(e.what());
        juce::Logger::writeToLog("❌ Exception in update test: " + juce::String(e.what()));
    }
    catch (...)
    {
        result.details = "Unknown exception occurred";
        juce::Logger::writeToLog("❌ Unknown exception in update test");
    }
    
    return result;
}

MetadataServiceTest::TestResult MetadataServiceTest::testWavFileValidation(const juce::File& testDirectory)
{
    TestResult result;
    result.message = "WAV file validation test";
    
    juce::Logger::writeToLog("📝 Testing WAV file validation...");
    
    try
    {
        // Test with a generated WAV file
        juce::File testWav = createTestWavFileDetailed(testDirectory, "validation_test.wav");
        if (!testWav.existsAsFile())
        {
            result.details = "Failed to create test WAV file for validation";
            return result;
        }
        
        // Validate the structure
        bool isValid = validateWavFileStructure(testWav);
        if (!isValid)
        {
            result.details = "Generated WAV file failed validation";
            return result;
        }
        
        result.success = true;
        result.details = "WAV file validation passed";
        juce::Logger::writeToLog("🎉 WAV validation test PASSED!");
    }
    catch (const std::exception& e)
    {
        result.details = "Exception: " + juce::String(e.what());
        juce::Logger::writeToLog("❌ Exception in validation test: " + juce::String(e.what()));
    }
    catch (...)
    {
        result.details = "Unknown exception occurred";
        juce::Logger::writeToLog("❌ Unknown exception in validation test");
    }
    
    return result;
}

//==============================================================================
// Helper Methods (Enhanced)
//==============================================================================

juce::File MetadataServiceTest::findExistingWavFile()
{
    juce::Logger::writeToLog("🔍 Searching for existing WAV files...");
    
    // Search in common locations
    juce::StringArray searchPaths = {
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getFullPathName(),
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName(),
        juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Documents").getFullPathName(),
        "/Users/grulf/Documents/Chops Library",  // Your specific library path
        juce::File::getCurrentWorkingDirectory().getFullPathName()
    };
    
    for (const auto& searchPath : searchPaths)
    {
        juce::File searchDir(searchPath);
        if (!searchDir.isDirectory())
            continue;
            
        juce::Logger::writeToLog("   Searching in: " + searchPath);
        
        juce::Array<juce::File> files;
        searchDir.findChildFiles(files, juce::File::findFiles, true, "*.wav");
        
        for (const auto& file : files)
        {
            if (file.existsAsFile() && file.getSize() > 1000) // At least 1KB
            {
                juce::Logger::writeToLog("   Found candidate: " + file.getFullPathName() + " (" + juce::String(file.getSize()) + " bytes)");
                
                // Do a quick validation
                if (validateWavFileStructure(file))
                {
                    juce::Logger::writeToLog("   ✅ Valid WAV file found!");
                    return file;
                }
                else
                {
                    juce::Logger::writeToLog("   ⚠️ File failed validation, continuing search...");
                }
            }
        }
    }
    
    juce::Logger::writeToLog("   No suitable WAV files found in search paths");
    return juce::File();
}

bool MetadataServiceTest::validateWavFileStructure(const juce::File& wavFile)
{
    if (!wavFile.existsAsFile())
    {
        juce::Logger::writeToLog("❌ WAV validation: File does not exist");
        return false;
    }
    
    if (wavFile.getSize() < 44)
    {
        juce::Logger::writeToLog("❌ WAV validation: File too small (" + juce::String(wavFile.getSize()) + " bytes, minimum 44)");
        return false;
    }
    
    try
    {
        juce::FileInputStream inputStream(wavFile);
        if (!inputStream.openedOk())
        {
            juce::Logger::writeToLog("❌ WAV validation: Cannot open file for reading");
            return false;
        }
        
        // Read first 12 bytes for RIFF header
        char header[12];
        int bytesRead = inputStream.read(header, 12);
        if (bytesRead != 12)
        {
            juce::Logger::writeToLog("❌ WAV validation: Cannot read header (got " + juce::String(bytesRead) + " bytes)");
            return false;
        }
        
        // Check RIFF signature
        if (strncmp(header, "RIFF", 4) != 0)
        {
            juce::Logger::writeToLog("❌ WAV validation: Missing RIFF signature");
            return false;
        }
        
        // Check WAVE signature
        if (strncmp(header + 8, "WAVE", 4) != 0)
        {
            juce::Logger::writeToLog("❌ WAV validation: Missing WAVE signature");
            return false;
        }
        
        // Get file size from header
        uint32_t headerFileSize;
        memcpy(&headerFileSize, header + 4, 4);
        int64_t actualFileSize = wavFile.getSize();
        
        juce::Logger::writeToLog("✅ WAV validation: Valid RIFF/WAVE header");
        juce::Logger::writeToLog("   Header file size: " + juce::String(headerFileSize + 8) + " bytes");
        juce::Logger::writeToLog("   Actual file size: " + juce::String(actualFileSize) + " bytes");
        
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("❌ WAV validation exception: " + juce::String(e.what()));
        return false;
    }
    catch (...)
    {
        juce::Logger::writeToLog("❌ WAV validation: Unknown exception");
        return false;
    }
}

juce::File MetadataServiceTest::createTestWavFileDetailed(const juce::File& directory, const juce::String& filename)
{
    juce::File wavFile = directory.getChildFile(filename);
    juce::Logger::writeToLog("🏗️ Creating WAV file: " + wavFile.getFullPathName());
    
    try
    {
        std::ofstream file(wavFile.getFullPathName().toStdString(), std::ios::binary);
        if (!file.is_open())
        {
            juce::Logger::writeToLog("❌ Cannot open file for writing");
            return juce::File();
        }
        
        // Create a more substantial WAV file with actual audio data
        const uint32_t sampleRate = 44100;
        const uint16_t numChannels = 2;
        const uint16_t bitsPerSample = 16;
        const uint32_t numSamples = sampleRate / 2; // 0.5 seconds of audio
        const uint32_t dataSize = numSamples * numChannels * (bitsPerSample / 8);
        const uint32_t fileSize = 36 + dataSize; // Header size + data size
        
        juce::Logger::writeToLog("   Sample rate: " + juce::String(sampleRate) + " Hz");
        juce::Logger::writeToLog("   Channels: " + juce::String(numChannels));
        juce::Logger::writeToLog("   Bit depth: " + juce::String(bitsPerSample) + " bits");
        juce::Logger::writeToLog("   Duration: " + juce::String(numSamples / sampleRate, 2) + " seconds");
        juce::Logger::writeToLog("   Data size: " + juce::String(dataSize) + " bytes");
        juce::Logger::writeToLog("   Total file size: " + juce::String(fileSize + 8) + " bytes");
        
        // RIFF header
        file.write("RIFF", 4);
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);
        
        // Format chunk
        file.write("fmt ", 4);
        uint32_t fmtSize = 16;
        file.write(reinterpret_cast<const char*>(&fmtSize), 4);
        
        uint16_t audioFormat = 1; // PCM
        file.write(reinterpret_cast<const char*>(&audioFormat), 2);
        file.write(reinterpret_cast<const char*>(&numChannels), 2);
        file.write(reinterpret_cast<const char*>(&sampleRate), 4);
        
        uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        
        uint16_t blockAlign = numChannels * (bitsPerSample / 8);
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        
        // Data chunk
        file.write("data", 4);
        file.write(reinterpret_cast<const char*>(&dataSize), 4);
        
        // Write audio data (440Hz sine wave)
        juce::Logger::writeToLog("   Writing audio data...");
        for (uint32_t i = 0; i < numSamples; i++)
        {
            double time = static_cast<double>(i) / sampleRate;
            double frequency = 440.0; // A4 note
            double amplitude = 0.3;
            
            int16_t sample = static_cast<int16_t>(amplitude * 32767.0 * sin(2.0 * M_PI * frequency * time));
            
            // Write stereo samples
            file.write(reinterpret_cast<const char*>(&sample), 2);
            file.write(reinterpret_cast<const char*>(&sample), 2);
        }
        
        file.close();
        
        if (wavFile.existsAsFile() && wavFile.getSize() > 0)
        {
            juce::Logger::writeToLog("✅ WAV file created successfully");
            juce::Logger::writeToLog("   Final size: " + juce::String(wavFile.getSize()) + " bytes");
            
            // Validate the created file
            if (validateWavFileStructure(wavFile))
            {
                juce::Logger::writeToLog("✅ Generated WAV file passed validation");
                return wavFile;
            }
            else
            {
                juce::Logger::writeToLog("❌ Generated WAV file failed validation");
                wavFile.deleteFile();
                return juce::File();
            }
        }
        else
        {
            juce::Logger::writeToLog("❌ WAV file creation failed - file is empty or missing");
            return juce::File();
        }
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("❌ Exception creating WAV file: " + juce::String(e.what()));
        return juce::File();
    }
    catch (...)
    {
        juce::Logger::writeToLog("❌ Unknown exception creating WAV file");
        return juce::File();
    }
}

bool MetadataServiceTest::compareMetadataDetailed(const MetadataService::ChordMetadata& expected, 
                                                const MetadataService::ChordMetadata& actual, 
                                                juce::String& differences)
{
    juce::StringArray diffs;
    
    juce::Logger::writeToLog("🔍 Detailed metadata comparison:");
    
    // Compare each field with detailed logging
    if (expected.rootNote != actual.rootNote)
    {
        juce::String diff = "rootNote: expected '" + expected.rootNote + "', got '" + actual.rootNote + "'";
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ rootNote matches: '" + expected.rootNote + "'");
    }
    
    if (expected.chordType != actual.chordType)
    {
        juce::String diff = "chordType: expected '" + expected.chordType + "', got '" + actual.chordType + "'";
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ chordType matches: '" + expected.chordType + "'");
    }
    
    if (expected.chordTypeDisplay != actual.chordTypeDisplay)
    {
        juce::String diff = "chordTypeDisplay: expected '" + expected.chordTypeDisplay + "', got '" + actual.chordTypeDisplay + "'";
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ chordTypeDisplay matches: '" + expected.chordTypeDisplay + "'");
    }
    
    if (expected.rating != actual.rating)
    {
        juce::String diff = "rating: expected " + juce::String(expected.rating) + ", got " + juce::String(actual.rating);
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ rating matches: " + juce::String(expected.rating));
    }
    
    if (expected.isFavorite != actual.isFavorite)
    {
        juce::String diff = "isFavorite: expected " + juce::String(expected.isFavorite ? "true" : "false") + 
                         ", got " + juce::String(actual.isFavorite ? "true" : "false");
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ isFavorite matches: " + juce::String(expected.isFavorite ? "true" : "false"));
    }
    
    if (expected.userNotes != actual.userNotes)
    {
        juce::String diff = "userNotes: expected '" + expected.userNotes + "', got '" + actual.userNotes + "'";
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ userNotes matches: '" + expected.userNotes + "'");
    }
    
    // Check arrays with detailed comparison
    if (expected.tags.size() != actual.tags.size() || expected.tags.joinIntoString(",") != actual.tags.joinIntoString(","))
    {
        juce::String diff = "tags: expected [" + expected.tags.joinIntoString(",") + "], got [" + actual.tags.joinIntoString(",") + "]";
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ tags match (" + juce::String(expected.tags.size()) + " items): [" + expected.tags.joinIntoString(",") + "]");
    }
    
    if (expected.extensions.size() != actual.extensions.size() || expected.extensions.joinIntoString(",") != actual.extensions.joinIntoString(","))
    {
        juce::String diff = "extensions: expected [" + expected.extensions.joinIntoString(",") + "], got [" + actual.extensions.joinIntoString(",") + "]";
        diffs.add(diff);
        juce::Logger::writeToLog("   ❌ " + diff);
    }
    else
    {
        juce::Logger::writeToLog("   ✅ extensions match (" + juce::String(expected.extensions.size()) + " items)");
    }
    
    differences = diffs.joinIntoString("; ");
    bool matches = diffs.isEmpty();
    
    if (matches)
    {
        juce::Logger::writeToLog("🎉 All metadata fields match perfectly!");
    }
    else
    {
        juce::Logger::writeToLog("❌ Found " + juce::String(diffs.size()) + " metadata differences");
    }
    
    return matches;
}

// Keep the existing helper methods but with logging
MetadataService::ChordMetadata MetadataServiceTest::createTestMetadata()
{
    MetadataService::ChordMetadata metadata;
    
    // Basic chord info
    metadata.rootNote = "C";
    metadata.chordType = "maj7";
    metadata.chordTypeDisplay = "Cmaj7";
    metadata.extensions.add("9");
    metadata.alterations.add("#11");
    metadata.addedNotes.add("add13");
    metadata.suspensions.add("sus4");
    metadata.bassNote = "E";
    metadata.inversion = "1st inversion";
    
    // User metadata
    metadata.tags.add("test");
    metadata.tags.add("jazz");
    metadata.rating = 4;
    metadata.isFavorite = false;
    metadata.userNotes = "Test chord metadata";
    metadata.color = juce::Colours::blue;
    metadata.playCount = 5;
    metadata.lastPlayed = juce::Time::getCurrentTime();
    
    // System metadata
    metadata.originalFilename = "test_sample.wav";
    metadata.dateAdded = juce::Time::getCurrentTime();
    metadata.dateModified = juce::Time::getCurrentTime();
    
    return metadata;
}

MetadataService::ChordMetadata MetadataServiceTest::createComplexTestMetadata()
{
    MetadataService::ChordMetadata metadata;
    
    // Complex chord info
    metadata.rootNote = "F#";
    metadata.chordType = "halfDim7";
    metadata.chordTypeDisplay = "F#ø7";
    metadata.extensions.add("9");
    metadata.extensions.add("11");
    metadata.extensions.add("13");
    metadata.alterations.add("b5");
    metadata.alterations.add("#9");
    metadata.addedNotes.add("add6");
    metadata.addedNotes.add("add4");
    metadata.suspensions.add("sus2");
    metadata.suspensions.add("sus4");
    metadata.bassNote = "C";
    metadata.inversion = "3rd inversion";
    
    // Complex user metadata
    metadata.tags.add("complex");
    metadata.tags.add("test");
    metadata.tags.add("jazz");
    metadata.tags.add("fusion");
    metadata.tags.add("advanced");
    metadata.rating = 5;
    metadata.isFavorite = true;
    metadata.userNotes = "Complex test chord with multiple extensions, alterations, and added notes. This tests the full range of metadata capabilities.";
    metadata.color = juce::Colour::fromRGB(255, 128, 64);
    metadata.playCount = 42;
    metadata.lastPlayed = juce::Time::getCurrentTime() - juce::RelativeTime::days(3);
    
    // System metadata
    metadata.originalFilename = "complex_test_sample_with_long_name.wav";
    metadata.dateAdded = juce::Time::getCurrentTime() - juce::RelativeTime::days(30);
    metadata.dateModified = juce::Time::getCurrentTime() - juce::RelativeTime::hours(2);
    
    return metadata;
}

// Keep existing helper methods unchanged
bool MetadataServiceTest::compareMetadata(const MetadataService::ChordMetadata& expected, 
                                        const MetadataService::ChordMetadata& actual, 
                                        juce::String& differences)
{
    return compareMetadataDetailed(expected, actual, differences);
}

juce::File MetadataServiceTest::createTestWavFile(const juce::File& directory, const juce::String& filename)
{
    return createTestWavFileDetailed(directory, filename);
}
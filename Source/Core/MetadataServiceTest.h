#pragma once

#include <JuceHeader.h>
#include "MetadataService.h"

/**
 * Simple test class for MetadataService
 * 
 * This provides basic tests to verify that metadata can be written to
 * and read from WAV files correctly.
 */
class MetadataServiceTest
{
public:
    MetadataServiceTest();
    
    struct TestResult
    {
        bool success = false;
        juce::String message;
        juce::String details;
        
        juce::String toString() const
        {
            juce::String result = success ? "✅ PASS: " : "❌ FAIL: ";
            result += message;
            if (details.isNotEmpty())
                result += "\n   Details: " + details;
            return result;
        }
    };
    
    // Run all tests
    bool runAllTests(const juce::File& testDirectory);
    
    // Individual tests
    TestResult testBasicMetadataWriteRead();
    TestResult testComplexMetadataWriteRead();
    TestResult testFileWithoutMetadata();
    TestResult testInvalidFile();
    TestResult testMetadataUpdate();
    
    // Test with actual WAV file
    TestResult testWithRealWavFile(const juce::File& wavFile);
    
    // Create a test WAV file
    static juce::File createTestWavFile(const juce::File& directory, const juce::String& filename);

private:
    MetadataService metadataService;
    
    // Helper methods
    MetadataService::ChordMetadata createTestMetadata();
    MetadataService::ChordMetadata createComplexTestMetadata();
    bool compareMetadata(const MetadataService::ChordMetadata& expected, 
                        const MetadataService::ChordMetadata& actual, 
                        juce::String& differences);
};
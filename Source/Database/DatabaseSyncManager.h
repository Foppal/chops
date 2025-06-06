#pragma once

#include <JuceHeader.h>
#include "ChopsDatabase.h" // Make sure this path is correct from this file's location

class DatabaseSyncManager : public juce::Timer
{
public:
    DatabaseSyncManager();
    ~DatabaseSyncManager() override;
    
    bool initialize(const juce::File& databasePath);
    
    ChopsDatabase* getReadDatabase() const { return const_cast<ChopsDatabase*>(&readDatabase); }

    int insertProcessedSample(const ChopsDatabase::SampleInfo& sampleInfo); 
    bool addTag(int sampleId, const juce::String& tag);
    bool removeTag(int sampleId, const juce::String& tag);
    bool setRating(int sampleId, int rating);
    bool setColor(int sampleId, const juce::Colour& color);
    bool toggleFavorite(int sampleId);
    bool incrementPlayCount(int sampleId);
    bool setNotes(int sampleId, const juce::String& notes);
    
    bool addTagsToMultiple(const juce::Array<int>& sampleIds, const juce::String& tag);
    bool setRatingForMultiple(const juce::Array<int>& sampleIds, int rating);
    
    int createCollection(const juce::String& name, const juce::String& description = "");
    bool addToCollection(int collectionId, int sampleId);
    bool removeFromCollection(int collectionId, int sampleId);
    juce::Array<int> getCollections();
    
    bool canUndo() const { return !undoStack.isEmpty(); }
    bool canRedo() const { return !redoStack.isEmpty(); }
    bool undo();
    bool redo();
    
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void databaseUpdated() = 0; 
        virtual void sampleMetadataChanged(int sampleId) = 0; 
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }
    void notifyListenersDatabaseUpdated();

private:
    ChopsDatabase readDatabase;
    ChopsDatabase writeDatabase;
    juce::CriticalSection writeLock;
    juce::File databaseFile;
    juce::Time lastModificationTime;
    
    struct Action {
        juce::String type; int sampleId; juce::var oldValue; juce::var newValue; juce::Time timestamp;
    };
    juce::Array<Action> undoStack;
    juce::Array<Action> redoStack;
    static constexpr int maxUndoLevels = 50;
    void logAction(const juce::String& actionType, int id, const juce::var& oldValue, const juce::var& newValue);
    
    struct WriteOperation { std::function<bool()> operation; std::function<void(bool)> callback; };
    juce::Array<WriteOperation> writeQueue;
    void processWriteQueue();
    
    void timerCallback() override;
    juce::ListenerList<Listener> listeners;
    void reloadReadDatabase(); 
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DatabaseSyncManager)
};
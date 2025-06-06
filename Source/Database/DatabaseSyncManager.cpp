#include "DatabaseSyncManager.h"

DatabaseSyncManager::DatabaseSyncManager() { startTimer(1000); }
DatabaseSyncManager::~DatabaseSyncManager() { stopTimer(); }

bool DatabaseSyncManager::initialize(const juce::File& dbPath) {
    juce::ScopedLock lock(writeLock);
    databaseFile = dbPath;
    juce::Logger::writeToLog("DSM: Init with DB: " + databaseFile.getFullPathName());
    if (!databaseFile.existsAsFile()) { juce::Logger::writeToLog("DSM Err: DB file missing: " + databaseFile.getFullPathName()); return false; }
    if (!readDatabase.open(databaseFile.getFullPathName())) { juce::Logger::writeToLog("DSM Err: Fail open read-DB: " + databaseFile.getFullPathName()); return false; }
    if (!writeDatabase.open(databaseFile.getFullPathName())) { juce::Logger::writeToLog("DSM Err: Fail open write-DB: " + databaseFile.getFullPathName()); readDatabase.close(); return false; }
    lastModificationTime = databaseFile.existsAsFile() ? databaseFile.getLastModificationTime() : juce::Time(0);
    juce::Logger::writeToLog("DSM: Initialized. Last mod: " + lastModificationTime.toString (true, true));
    return true;
}

void DatabaseSyncManager::reloadReadDatabase() {
    juce::Logger::writeToLog("DSM: Reloading read DB...");
    juce::String dbPath = databaseFile.getFullPathName();
    readDatabase.close(); 
    if (!readDatabase.open(dbPath)) juce::Logger::writeToLog("DSM Err: Failed reload read DB from " + dbPath);
    else juce::Logger::writeToLog("DSM: Read DB reloaded.");
}

void DatabaseSyncManager::notifyListenersDatabaseUpdated() { listeners.call(&Listener::databaseUpdated); }

int DatabaseSyncManager::insertProcessedSample(const ChopsDatabase::SampleInfo& sampleInfo) {
    juce::ScopedLock lock(writeLock);
    if (!writeDatabase.isOpen()) { juce::Logger::writeToLog("DSM Err: Write DB not open for insert."); return -1; }
    int newId = writeDatabase.insertSample(sampleInfo);
    if (newId > 0) {
        logAction("sample_inserted", newId, juce::var(), juce::var(sampleInfo.originalFilename)); 
        reloadReadDatabase();
        listeners.call(&Listener::databaseUpdated); 
    } else {
        juce::Logger::writeToLog("DSM Err: Failed to insert sample: " + sampleInfo.originalFilename);
    }
    return newId;
}

bool DatabaseSyncManager::addTag(int id, const juce::String& tag) {
    juce::ScopedLock lock(writeLock); if (!writeDatabase.isOpen()||tag.isEmpty()) return false;
    auto oldT = readDatabase.getTags(id); bool ok = writeDatabase.addTag(id,tag);
    if(ok){logAction("tag_added",id,juce::var(oldT.joinIntoString(";;")),juce::var(tag)); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,id);} return ok;
}
bool DatabaseSyncManager::removeTag(int id, const juce::String& tag) {
    juce::ScopedLock lock(writeLock); if (!writeDatabase.isOpen()||tag.isEmpty()) return false;
    bool ok = writeDatabase.removeTag(id,tag);
    if(ok){logAction("tag_removed",id,juce::var(tag),juce::var()); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,id);} return ok;
}
bool DatabaseSyncManager::setRating(int id, int r) {
    juce::ScopedLock lock(writeLock); if (!writeDatabase.isOpen()) return false;
    auto si=readDatabase.getSampleById(id); int oldR=si?si->rating:0;
    bool ok = writeDatabase.setRating(id,r);
    if(ok){logAction("rating_changed",id,juce::var(oldR),juce::var(r)); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,id);} return ok;
}
bool DatabaseSyncManager::setColor(int id, const juce::Colour& c) {
    juce::ScopedLock lock(writeLock); if (!writeDatabase.isOpen()) return false;
    auto si=readDatabase.getSampleById(id); juce::String oCStr=si?si->color.toDisplayString(true):juce::Colours::transparentBlack.toDisplayString(true);
    bool ok = writeDatabase.setColor(id,c);
    if(ok){logAction("color_changed",id,juce::var(oCStr),juce::var(c.toDisplayString(true))); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,id);} return ok;
}
bool DatabaseSyncManager::toggleFavorite(int id) {
    juce::ScopedLock lock(writeLock); if (!writeDatabase.isOpen()) return false;
    auto si=readDatabase.getSampleById(id); if(!si)return false; bool wasF=si->isFavorite;
    bool ok=wasF?writeDatabase.removeFromFavorites(id):writeDatabase.addToFavorites(id);
    if(ok){logAction("favorite_toggled",id,juce::var(wasF),juce::var(!wasF)); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,id);} return ok;
}
bool DatabaseSyncManager::incrementPlayCount(int id) {
    juce::ScopedLock lock(writeLock); if (!writeDatabase.isOpen()) return false;
    auto si=readDatabase.getSampleById(id); int oC=si?si->playCount:0;
    bool ok=writeDatabase.incrementPlayCount(id);
    if(ok){logAction("play_count_incremented",id,juce::var(oC),juce::var(oC+1)); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,id);} return ok;
}
bool DatabaseSyncManager::setNotes(int id, const juce::String& n) {
    juce::ScopedLock lock(writeLock); if (!writeDatabase.isOpen()) return false;
    auto si=readDatabase.getSampleById(id); juce::String oN=si?si->userNotes:"";
    bool ok=writeDatabase.setNotes(id,n);
    if(ok){logAction("notes_changed",id,juce::var(oN),juce::var(n)); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,id);} return ok;
}

bool DatabaseSyncManager::addTagsToMultiple(const juce::Array<int>& ids, const juce::String& tag) {
    juce::ScopedLock l(writeLock); if(!writeDatabase.isOpen()||ids.isEmpty()||tag.isEmpty())return false;
    if(!writeDatabase.beginTransaction())return false; bool ok=true;
    for(int id:ids)if(!writeDatabase.addTag(id,tag)){ok=false;break;}
    if(ok){writeDatabase.commitTransaction(); reloadReadDatabase(); for(int id:ids)listeners.call(&Listener::sampleMetadataChanged,id); listeners.call(&Listener::databaseUpdated);}
    else writeDatabase.rollbackTransaction(); return ok;
}
bool DatabaseSyncManager::setRatingForMultiple(const juce::Array<int>& ids, int r) {
    juce::ScopedLock l(writeLock); if(!writeDatabase.isOpen()||ids.isEmpty())return false;
    if(!writeDatabase.beginTransaction())return false; bool ok=true;
    for(int id:ids)if(!writeDatabase.setRating(id,r)){ok=false;break;}
    if(ok){writeDatabase.commitTransaction(); reloadReadDatabase(); for(int id:ids)listeners.call(&Listener::sampleMetadataChanged,id); listeners.call(&Listener::databaseUpdated);}
    else writeDatabase.rollbackTransaction(); return ok;
}

int DatabaseSyncManager::createCollection(const juce::String& n, const juce::String& d){juce::ignoreUnused(n,d);return -1;}
bool DatabaseSyncManager::addToCollection(int cId,int sId){juce::ignoreUnused(cId,sId);return false;}
bool DatabaseSyncManager::removeFromCollection(int cId,int sId){juce::ignoreUnused(cId,sId);return false;}
juce::Array<int> DatabaseSyncManager::getCollections(){return{};}

void DatabaseSyncManager::logAction(const juce::String& type, int id, const juce::var& ov, const juce::var& nv) {
    undoStack.add({type,id,ov,nv,juce::Time::getCurrentTime()});
    if(undoStack.size()>maxUndoLevels)undoStack.remove(0);
    redoStack.clear();
}

bool DatabaseSyncManager::undo() {
    juce::ScopedLock lock(writeLock); if (!canUndo()||!writeDatabase.isOpen()) return false;
    Action action = undoStack.getLast(); // Get copy
    bool success = false;
    if (action.type=="tag_added") success=writeDatabase.removeTag(action.sampleId, action.newValue.toString());
    else if (action.type=="tag_removed") success=writeDatabase.addTag(action.sampleId, action.oldValue.toString());
    else if (action.type=="rating_changed") success=writeDatabase.setRating(action.sampleId, (int)action.oldValue);
    else if (action.type=="color_changed") success=writeDatabase.setColor(action.sampleId, juce::Colour::fromString(action.oldValue.toString()));
    else if (action.type=="favorite_toggled"){bool origFav=(bool)action.newValue; if(origFav)success=writeDatabase.removeFromFavorites(action.sampleId); else success=writeDatabase.addToFavorites(action.sampleId);}
    else if (action.type=="notes_changed") success=writeDatabase.setNotes(action.sampleId, action.oldValue.toString());
    if(success){undoStack.removeLast(); redoStack.add(action); if(redoStack.size()>maxUndoLevels)redoStack.remove(0); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,action.sampleId); listeners.call(&Listener::databaseUpdated);}
    return success;
}
bool DatabaseSyncManager::redo() {
    juce::ScopedLock lock(writeLock); if (!canRedo()||!writeDatabase.isOpen()) return false;
    Action action = redoStack.getLast(); // Get copy
    bool success = false;
    if (action.type=="tag_added") success=writeDatabase.addTag(action.sampleId, action.newValue.toString());
    else if (action.type=="tag_removed") success=writeDatabase.removeTag(action.sampleId, action.oldValue.toString());
    else if (action.type=="rating_changed") success=writeDatabase.setRating(action.sampleId, (int)action.newValue);
    else if (action.type=="color_changed") success=writeDatabase.setColor(action.sampleId, juce::Colour::fromString(action.newValue.toString()));
    else if (action.type=="favorite_toggled"){bool targetFav=(bool)action.newValue; if(targetFav)success=writeDatabase.addToFavorites(action.sampleId); else success=writeDatabase.removeFromFavorites(action.sampleId);}
    else if (action.type=="notes_changed") success=writeDatabase.setNotes(action.sampleId, action.newValue.toString());
    if(success){redoStack.removeLast(); undoStack.add(action); if(undoStack.size()>maxUndoLevels)undoStack.remove(0); reloadReadDatabase(); listeners.call(&Listener::sampleMetadataChanged,action.sampleId); listeners.call(&Listener::databaseUpdated);}
    return success;
}

void DatabaseSyncManager::processWriteQueue() {
    if (writeQueue.isEmpty()) return;
    juce::Logger::writeToLog("DSM: Processing write queue ("+juce::String(writeQueue.size())+")");
    bool changed=false;
    for(const auto&op:writeQueue) if(op.operation()) changed=true; // op.callback could be used
    writeQueue.clear();
    if(changed){reloadReadDatabase(); listeners.call(&Listener::databaseUpdated);}
}
void DatabaseSyncManager::timerCallback() {
    if(writeQueue.size()>0){juce::ScopedLock lock(writeLock); processWriteQueue();}
    if(databaseFile.existsAsFile()){
        juce::Time currentModTime=databaseFile.getLastModificationTime();
        if(currentModTime > lastModificationTime){
            juce::Logger::writeToLog("DSM: External DB mod detected.");
            juce::ScopedLock lock(writeLock); 
            lastModificationTime = currentModTime;
            reloadReadDatabase();
            listeners.call(&Listener::databaseUpdated);
        }
    }
}
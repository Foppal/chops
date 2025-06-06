#include "ChopsDatabase.h" // Must be first for JuceHeader.h if PCH are used
#include "../Core/ChordTypes.h"
#include <sqlite3.h>
#include <algorithm> // For std::remove_if

// Helper to convert juce::String to std::string for SQLite
static std::string toStdString(const juce::String& str)
{
    return str.toStdString();
}

// Helper to convert SQLite text to juce::String
static juce::String fromSqliteText(const unsigned char* text)
{
    return text ? juce::String(reinterpret_cast<const char*>(text)) : juce::String();
}

//==============================================================================
ChopsDatabase::ChopsDatabase()
    : db(nullptr), searchStmt(nullptr), sampleByPathStmt(nullptr), sampleByIdStmt(nullptr)
{
}

ChopsDatabase::~ChopsDatabase()
{
    close();
}

bool ChopsDatabase::open(const juce::String& databasePath)
{
    close(); 
    
    juce::Logger::writeToLog("Opening database: " + databasePath);
    
    int result = sqlite3_open(toStdString(databasePath).c_str(), 
                             reinterpret_cast<sqlite3**>(&db));
    
    if (result != SQLITE_OK)
    {
        juce::String errorMsg = "Failed to open database: " + databasePath;
        if (db)
        {
            errorMsg += " - " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db)));
        }
        juce::Logger::writeToLog(errorMsg);
        close();
        return false;
    }
    
    sqlite3_exec(static_cast<sqlite3*>(db), "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
    sqlite3_exec(static_cast<sqlite3*>(db), "PRAGMA journal_mode = WAL", nullptr, nullptr, nullptr);
    sqlite3_exec(static_cast<sqlite3*>(db), "PRAGMA synchronous = NORMAL", nullptr, nullptr, nullptr);
    sqlite3_exec(static_cast<sqlite3*>(db), "PRAGMA cache_size = 10000", nullptr, nullptr, nullptr); // Consider making cache size configurable or based on system
    
    prepareStatements();
    
    juce::Logger::writeToLog("Database opened successfully");
    return true;
}

void ChopsDatabase::close()
{
    finalizeStatements();
    
    if (db != nullptr)
    {
        int result = sqlite3_close_v2(static_cast<sqlite3*>(db)); // Use sqlite3_close_v2 for graceful shutdown
        if (result != SQLITE_OK)
        {
            juce::Logger::writeToLog("Warning: Error closing database - " + 
                                   juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
        }
        db = nullptr;
    }
}

//==============================================================================
void ChopsDatabase::prepareStatements()
{
    if (db == nullptr) return;
    
    juce::Logger::writeToLog("Preparing database statements");
    
    int result = sqlite3_prepare_v2(static_cast<sqlite3*>(db), 
        R"(
            SELECT s.*, GROUP_CONCAT(t.name, ',') as tag_list
            FROM samples s
            LEFT JOIN sample_tags st ON s.id = st.sample_id
            LEFT JOIN tags t ON st.tag_id = t.id
            WHERE 1=1
            AND (?1 = '' OR s.search_text LIKE '%' || ?2 || '%')
            AND (?3 = '' OR s.root_note = ?4)
            AND (?5 = '' OR s.chord_type = ?6)
            GROUP BY s.id
            ORDER BY s.root_note, s.chord_type, s.date_added DESC
            LIMIT ?7 OFFSET ?8
        )", 
        -1, reinterpret_cast<sqlite3_stmt**>(&searchStmt), nullptr);
    
    if (result != SQLITE_OK) {
        juce::Logger::writeToLog("Failed to prepare search statement: " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
    }
    
    // ... rest of the method remains the same
}

void ChopsDatabase::finalizeStatements()
{
    if (searchStmt) { sqlite3_finalize(static_cast<sqlite3_stmt*>(searchStmt)); searchStmt = nullptr; }
    if (sampleByPathStmt) { sqlite3_finalize(static_cast<sqlite3_stmt*>(sampleByPathStmt)); sampleByPathStmt = nullptr; }
    if (sampleByIdStmt) { sqlite3_finalize(static_cast<sqlite3_stmt*>(sampleByIdStmt)); sampleByIdStmt = nullptr; }
}

//==============================================================================
ChopsDatabase::SampleInfo ChopsDatabase::parseRow(void* stmtPtr)
{
    SampleInfo info;
    auto* stmt = static_cast<sqlite3_stmt*>(stmtPtr);
    
    if (!stmt) return info;
    
    try {
        int col = 0;
        info.id = sqlite3_column_int(stmt, col++);
        info.originalFilename = fromSqliteText(sqlite3_column_text(stmt, col++));
        info.currentFilename = fromSqliteText(sqlite3_column_text(stmt, col++));
        info.filePath = fromSqliteText(sqlite3_column_text(stmt, col++));
        info.fileSize = sqlite3_column_int64(stmt, col++);
        
        info.rootNote = fromSqliteText(sqlite3_column_text(stmt, col++));
        info.chordType = fromSqliteText(sqlite3_column_text(stmt, col++));
        info.chordTypeDisplay = fromSqliteText(sqlite3_column_text(stmt, col++));
        
        info.extensions = parseJsonArray(fromSqliteText(sqlite3_column_text(stmt, col++)));
        info.alterations = parseJsonArray(fromSqliteText(sqlite3_column_text(stmt, col++)));
        info.addedNotes = parseJsonArray(fromSqliteText(sqlite3_column_text(stmt, col++)));
        info.suspensions = parseJsonArray(fromSqliteText(sqlite3_column_text(stmt, col++)));
        
        info.bassNote = fromSqliteText(sqlite3_column_text(stmt, col++));
        info.inversion = fromSqliteText(sqlite3_column_text(stmt, col++));
        
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) info.dateAdded = juce::Time::fromISO8601(fromSqliteText(sqlite3_column_text(stmt, col)));
        col++;
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) info.dateModified = juce::Time::fromISO8601(fromSqliteText(sqlite3_column_text(stmt, col)));
        col++;

        // Skip columns not in SampleInfo struct (as per schema.sql and ChopsDatabase.h)
        col++; // processing_version (col 16)
        col++; // search_text (col 17)
        col++; // duration_ms (col 18)
        col++; // sample_rate (col 19)
        col++; // bit_depth (col 20)
        col++; // channels (col 21)
        col++; // bpm (col 22)
        col++; // musical_key (col 23)

        // Resume with SampleInfo fields
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) info.rating = sqlite3_column_int(stmt, col);
        col++; // rating (col 24)
        
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) {
            auto colorHex = fromSqliteText(sqlite3_column_text(stmt, col));
            if (colorHex.isNotEmpty()) info.color = juce::Colour::fromString(colorHex);
        }
        col++; // color_hex (col 25)
        
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) info.isFavorite = sqlite3_column_int(stmt, col) != 0;
        col++; // is_favorite (col 26)
        
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) info.playCount = sqlite3_column_int(stmt, col);
        col++; // play_count (col 27)
        
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) info.userNotes = fromSqliteText(sqlite3_column_text(stmt, col));
        col++; // user_notes (col 28)
        
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) info.lastPlayed = juce::Time::fromISO8601(fromSqliteText(sqlite3_column_text(stmt, col)));
        col++; // last_played (col 29)

        // Tags (appended by GROUP_CONCAT, will be the next column after all s.* columns)
        // col should now be 30 if all s.* columns were present.
        int totalColumnCount = sqlite3_column_count(stmt);
        if (col < totalColumnCount && strcmp(sqlite3_column_name(stmt, col), "tag_list") == 0) {
            if (sqlite3_column_type(stmt, col) != SQLITE_NULL) {
                auto tagListStr = fromSqliteText(sqlite3_column_text(stmt, col));
                if (tagListStr.isNotEmpty()) {
                    info.tags = juce::StringArray::fromTokens(tagListStr, ",", "");
                }
            }
        }
    }
    catch (const std::exception& e) { // Catch specific exceptions if possible
        juce::Logger::writeToLog("Error parsing database row: " + juce::String(e.what()));
    }
    catch (...) {
        juce::Logger::writeToLog("Unknown error parsing database row");
    }
    return info;
}


juce::StringArray ChopsDatabase::parseJsonArray(const juce::String& json)
{
    juce::StringArray result;
    if (json.isEmpty() || json == "[]" || json == "null") // Handle "null" as empty too
        return result;
    
    if (json.startsWith("[") && json.endsWith("]")) {
        juce::String content = json.substring(1, json.length() - 1).trim();
        if (content.isEmpty()) return result;
        
        juce::StringArray parts;
        int start = 0;
        bool inQuotes = false;
        for (int i = 0; i < content.length(); ++i) {
            if (content[i] == '"') inQuotes = !inQuotes;
            else if (content[i] == ',' && !inQuotes) {
                parts.add(content.substring(start, i).trim());
                start = i + 1;
            }
        }
        parts.add(content.substring(start).trim());

        for (auto& part : parts) {
            part = part.trim();
            if (part.startsWith("\"") && part.endsWith("\"")) {
                part = part.substring(1, part.length() - 1);
            }
            // Unescape JSON strings if necessary, e.g., \\" -> "
            part = part.replace("\\\"", "\"").replace("\\\\", "\\"); 
            if (part.isNotEmpty()) result.add(part);
        }
    } else {
        // If it's not a valid JSON array string, perhaps log or handle differently.
        // For now, return empty if not matching format. Could also be a single non-array string.
    }
    return result;
}

juce::String ChopsDatabase::stringArrayToJson(const juce::StringArray& array)
{
    if (array.isEmpty()) return "[]";
    
    juce::String json = "[";
    for (int i = 0; i < array.size(); ++i) {
        if (i > 0) json += ",";
        // Escape quotes and backslashes for JSON validity
        json += "\"" + array[i].replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
    }
    json += "]";
    return json;
}

//==============================================================================
std::vector<ChopsDatabase::SampleInfo> ChopsDatabase::searchSamples(
    const juce::String& query, const juce::String& rootNote, const juce::String& chordType,
    BoolFilter hasExtensions, BoolFilter hasAlterations, int limit, int offset)
{
    std::vector<SampleInfo> results;
    if (db == nullptr || searchStmt == nullptr) {
        juce::Logger::writeToLog("Database or search statement not available for searchSamples");
        return results;
    }
    
    auto* stmt = static_cast<sqlite3_stmt*>(searchStmt);
    try {
        sqlite3_bind_text(stmt, 1, toStdString(query).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, toStdString(query).c_str(), -1, SQLITE_TRANSIENT); // query used twice in original SQL
        sqlite3_bind_text(stmt, 3, toStdString(rootNote).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, toStdString(rootNote).c_str(), -1, SQLITE_TRANSIENT); // rootNote used twice
        sqlite3_bind_text(stmt, 5, toStdString(chordType).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, toStdString(chordType).c_str(), -1, SQLITE_TRANSIENT); // chordType used twice
        sqlite3_bind_int(stmt, 7, limit);
        sqlite3_bind_int(stmt, 8, offset);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            results.push_back(parseRow(stmt));
        }
        sqlite3_reset(stmt); // Reset for next use
        
        if (hasExtensions != DontCare || hasAlterations != DontCare) {
            results.erase(std::remove_if(results.begin(), results.end(),
                [hasExtensions, hasAlterations](const SampleInfo& info) {
                    if (hasExtensions == Yes && info.extensions.isEmpty()) return true;
                    if (hasExtensions == No && !info.extensions.isEmpty()) return true;
                    if (hasAlterations == Yes && info.alterations.isEmpty()) return true;
                    if (hasAlterations == No && !info.alterations.isEmpty()) return true;
                    return false;
                }), results.end());
        }
    } catch (...) {
        juce::Logger::writeToLog("Error executing search query");
        sqlite3_reset(stmt); // Ensure reset even on error
    }
    return results;
}

std::unique_ptr<ChopsDatabase::SampleInfo> ChopsDatabase::getSampleByPath(const juce::String& filePath)
{
    if (db == nullptr || sampleByPathStmt == nullptr) return nullptr;
    auto* stmt = static_cast<sqlite3_stmt*>(sampleByPathStmt);
    std::unique_ptr<SampleInfo> info;
    try {
        sqlite3_bind_text(stmt, 1, toStdString(filePath).c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            info = std::make_unique<SampleInfo>(parseRow(stmt));
        }
        sqlite3_reset(stmt);
    } catch (...) {
        juce::Logger::writeToLog("Error getting sample by path");
        sqlite3_reset(stmt);
    }
    return info;
}

std::unique_ptr<ChopsDatabase::SampleInfo> ChopsDatabase::getSampleById(int sampleId)
{
    if (db == nullptr || sampleByIdStmt == nullptr) return nullptr;
    auto* stmt = static_cast<sqlite3_stmt*>(sampleByIdStmt);
    std::unique_ptr<SampleInfo> info;
    try {
        sqlite3_bind_int(stmt, 1, sampleId);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            info = std::make_unique<SampleInfo>(parseRow(stmt));
        }
        sqlite3_reset(stmt);
    } catch (...) {
        juce::Logger::writeToLog("Error getting sample by ID");
        sqlite3_reset(stmt);
    }
    return info;
}

//==============================================================================
int ChopsDatabase::insertSample(const SampleInfo& sample)
{
    if (db == nullptr) return -1;
    
    sqlite3_stmt* stmt;
    
    // Using direct string instead of variable to avoid unused variable warning
    if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), 
        R"(
            INSERT INTO samples (
                original_filename, current_filename, file_path, file_size,
                root_note, chord_type, chord_type_display,
                extensions, alterations, added_notes, suspensions,
                bass_note, inversion, 
                search_text, rating, color_hex, is_favorite, play_count, user_notes, last_played
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )", 
        -1, &stmt, nullptr) != SQLITE_OK) {
        
        juce::Logger::writeToLog("Error preparing insertSample statement: " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
        return -1;
    }

    try {
        int col = 1;
        sqlite3_bind_text(stmt, col++, toStdString(sample.originalFilename).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.currentFilename).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.filePath).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, col++, sample.fileSize);
        sqlite3_bind_text(stmt, col++, toStdString(sample.rootNote).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.chordType).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.chordTypeDisplay).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.extensions)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.alterations)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.addedNotes)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.suspensions)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.bassNote).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.inversion).c_str(), -1, SQLITE_TRANSIENT);
        
        juce::String searchText = (sample.originalFilename + " " + sample.currentFilename + " " +
                                  sample.rootNote + " " + sample.chordType + " " +
                                  sample.tags.joinIntoString(" ")).toLowerCase();
        sqlite3_bind_text(stmt, col++, toStdString(searchText).c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_bind_int(stmt, col++, sample.rating);
        sqlite3_bind_text(stmt, col++, toStdString(sample.color.toDisplayString(true)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, col++, sample.isFavorite ? 1 : 0);
        sqlite3_bind_int(stmt, col++, sample.playCount);
        sqlite3_bind_text(stmt, col++, toStdString(sample.userNotes).c_str(), -1, SQLITE_TRANSIENT);
        
        if (sample.lastPlayed.toMilliseconds() > 0)
            sqlite3_bind_text(stmt, col++, toStdString(sample.lastPlayed.toISO8601(true)).c_str(), -1, SQLITE_TRANSIENT);
        else
            sqlite3_bind_null(stmt, col++);

        int result = sqlite3_step(stmt);
        if (result == SQLITE_DONE) {
            int sampleId = static_cast<int>(sqlite3_last_insert_rowid(static_cast<sqlite3*>(db)));
            for (const auto& tag : sample.tags) {
                addTag(sampleId, tag);
            }
            sqlite3_finalize(stmt);
            return sampleId;
        } else {
            juce::Logger::writeToLog("Error inserting sample: " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
        }
    } catch (...) {
        juce::Logger::writeToLog("Exception inserting sample");
    }
    sqlite3_finalize(stmt);
    return -1;
}

bool ChopsDatabase::updateSample(const SampleInfo& sample)
{
    if (db == nullptr || sample.id <= 0) return false;
    
    // Similar to insert, update fields present in SampleInfo
    // date_modified is set to CURRENT_TIMESTAMP by the query
    const char* sql = R"(
        UPDATE samples SET
            original_filename = ?, current_filename = ?, file_path = ?, file_size = ?,
            root_note = ?, chord_type = ?, chord_type_display = ?,
            extensions = ?, alterations = ?, added_notes = ?, suspensions = ?,
            bass_note = ?, inversion = ?, search_text = ?,
            rating = ?, color_hex = ?, is_favorite = ?, play_count = ?, user_notes = ?, last_played = ?,
            date_modified = CURRENT_TIMESTAMP
        WHERE id = ? 
    )"; // 20 fields to set + id (21 bindings)

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        juce::Logger::writeToLog("Error preparing updateSample statement: " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
        return false;
    }

    try {
        int col = 1;
        sqlite3_bind_text(stmt, col++, toStdString(sample.originalFilename).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.currentFilename).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.filePath).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, col++, sample.fileSize);
        sqlite3_bind_text(stmt, col++, toStdString(sample.rootNote).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.chordType).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.chordTypeDisplay).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.extensions)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.alterations)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.addedNotes)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(stringArrayToJson(sample.suspensions)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.bassNote).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, toStdString(sample.inversion).c_str(), -1, SQLITE_TRANSIENT);
        
        juce::String searchText = (sample.originalFilename + " " + sample.currentFilename + " " +
                                  sample.rootNote + " " + sample.chordType + " " +
                                  sample.tags.joinIntoString(" ")).toLowerCase();
        sqlite3_bind_text(stmt, col++, toStdString(searchText).c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_bind_int(stmt, col++, sample.rating);
        sqlite3_bind_text(stmt, col++, toStdString(sample.color.toDisplayString(true)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, col++, sample.isFavorite ? 1 : 0);
        sqlite3_bind_int(stmt, col++, sample.playCount);
        sqlite3_bind_text(stmt, col++, toStdString(sample.userNotes).c_str(), -1, SQLITE_TRANSIENT);

        if (sample.lastPlayed.toMilliseconds() > 0)
            sqlite3_bind_text(stmt, col++, toStdString(sample.lastPlayed.toISO8601(true)).c_str(), -1, SQLITE_TRANSIENT);
        else
            sqlite3_bind_null(stmt, col++);
            
        sqlite3_bind_int(stmt, col++, sample.id);
        
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        if (!success) {
            juce::Logger::writeToLog("Error updating sample: " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
        }
        sqlite3_finalize(stmt);
        return success;
    } catch (...) {
        juce::Logger::writeToLog("Exception updating sample");
    }
    sqlite3_finalize(stmt);
    return false;
}

bool ChopsDatabase::deleteSample(int sampleId)
{
    if (db == nullptr || sampleId <= 0) return false;
    const char* sql = "DELETE FROM samples WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_int(stmt, 1, sampleId);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

// ... (The rest of the functions: addTag, removeTag, setRating, setColor, addToFavorites, etc.
//      from the previous correct reconstruction should largely be okay as they target specific columns
//      or tables and their logic is less dependent on the full 'samples' table structure like parseRow/insert/update.)
//      I will include them for completeness, ensuring their try-catch and logging are consistent.

//==============================================================================
// User metadata (continued from original structure)
bool ChopsDatabase::addTag(int sampleId, const juce::String& tag)
{
    if (db == nullptr || tag.isEmpty()) return false;
    try {
        // Insert tag if it doesn't exist
        const char* insertTagSql = "INSERT OR IGNORE INTO tags (name) VALUES (?)";
        sqlite3_stmt* insertStmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), insertTagSql, -1, &insertStmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(insertStmt, 1, toStdString(tag).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(insertStmt); // We don't care about result here, just that it ran
        sqlite3_finalize(insertStmt);

        // Get tag ID
        const char* getTagIdSql = "SELECT id FROM tags WHERE name = ?";
        sqlite3_stmt* getIdStmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), getTagIdSql, -1, &getIdStmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(getIdStmt, 1, toStdString(tag).c_str(), -1, SQLITE_TRANSIENT);
        
        int tagId = -1;
        if (sqlite3_step(getIdStmt) == SQLITE_ROW) {
            tagId = sqlite3_column_int(getIdStmt, 0);
        }
        sqlite3_finalize(getIdStmt);
        if (tagId < 0) return false;

        // Add sample-tag relationship
        const char* addRelationSql = "INSERT OR IGNORE INTO sample_tags (sample_id, tag_id) VALUES (?, ?)";
        sqlite3_stmt* addRelStmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), addRelationSql, -1, &addRelStmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(addRelStmt, 1, sampleId);
        sqlite3_bind_int(addRelStmt, 2, tagId);
        bool success = sqlite3_step(addRelStmt) == SQLITE_DONE;
        sqlite3_finalize(addRelStmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error adding tag"); }
    return false;
}


bool ChopsDatabase::removeTag(int sampleId, const juce::String& tag)
{
    if (db == nullptr) return false;
    try {
        const char* sql = "DELETE FROM sample_tags WHERE sample_id = ? AND tag_id = (SELECT id FROM tags WHERE name = ?)";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, sampleId);
        sqlite3_bind_text(stmt, 2, toStdString(tag).c_str(), -1, SQLITE_TRANSIENT);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error removing tag"); }
    return false;
}

bool ChopsDatabase::setRating(int sampleId, int rating)
{
    if (db == nullptr) return false;
    try {
        const char* sql = "UPDATE samples SET rating = ? WHERE id = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, rating);
        sqlite3_bind_int(stmt, 2, sampleId);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error setting rating"); }
    return false;
}

bool ChopsDatabase::setColor(int sampleId, const juce::Colour& color)
{
    if (db == nullptr) return false;
    try {
        const char* sql = "UPDATE samples SET color_hex = ? WHERE id = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, toStdString(color.toDisplayString(true)).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, sampleId);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error setting color"); }
    return false;
}

bool ChopsDatabase::addToFavorites(int sampleId)
{
    if (db == nullptr) return false;
    try {
        const char* sql = "UPDATE samples SET is_favorite = 1 WHERE id = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, sampleId);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error adding to favorites"); }
    return false;
}

bool ChopsDatabase::removeFromFavorites(int sampleId)
{
    if (db == nullptr) return false;
    try {
        const char* sql = "UPDATE samples SET is_favorite = 0 WHERE id = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, sampleId);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error removing from favorites"); }
    return false;
}

bool ChopsDatabase::incrementPlayCount(int sampleId)
{
    if (db == nullptr) return false;
    try {
        const char* sql = "UPDATE samples SET play_count = play_count + 1, last_played = CURRENT_TIMESTAMP WHERE id = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, sampleId);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error incrementing play count"); }
    return false;
}

bool ChopsDatabase::setNotes(int sampleId, const juce::String& notes)
{
    if (db == nullptr) return false;
    try {
        const char* sql = "UPDATE samples SET user_notes = ? WHERE id = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, toStdString(notes).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, sampleId);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    } catch (...) { juce::Logger::writeToLog("Error setting notes"); }
    return false;
}

//==============================================================================
// Tag management (continued)
juce::StringArray ChopsDatabase::getTags(int sampleId)
{
    juce::StringArray tags;
    if (db == nullptr) return tags;
    try {
        const char* sql = "SELECT t.name FROM tags t JOIN sample_tags st ON t.id = st.tag_id WHERE st.sample_id = ? ORDER BY t.name";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return tags;
        sqlite3_bind_int(stmt, 1, sampleId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            tags.add(fromSqliteText(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
    } catch (...) { juce::Logger::writeToLog("Error getting tags"); }
    return tags;
}

juce::StringArray ChopsDatabase::getAllTags()
{
    juce::StringArray tags;
    if (db == nullptr) return tags;
    try {
        const char* sql = "SELECT name FROM tags ORDER BY name";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return tags;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            tags.add(fromSqliteText(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
    } catch (...) { juce::Logger::writeToLog("Error getting all tags"); }
    return tags;
}

std::vector<ChopsDatabase::SampleInfo> ChopsDatabase::getSamplesByTag(const juce::String& tag)
{
    std::vector<SampleInfo> results;
    if (db == nullptr || tag.isEmpty()) return results;
    try {
        const char* sql = R"(
            SELECT s.*, GROUP_CONCAT(t2.name, ',') as tag_list
            FROM samples s
            JOIN sample_tags st ON s.id = st.sample_id
            JOIN tags t ON st.tag_id = t.id
            LEFT JOIN sample_tags st2 ON s.id = st2.sample_id 
            LEFT JOIN tags t2 ON st2.tag_id = t2.id      
            WHERE t.name = ?
            GROUP BY s.id
            ORDER BY s.root_note, s.chord_type
        )";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return results;
        sqlite3_bind_text(stmt, 1, toStdString(tag).c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            results.push_back(parseRow(stmt));
        }
        sqlite3_finalize(stmt);
    } catch (...) { juce::Logger::writeToLog("Error getting samples by tag"); }
    return results;
}

//==============================================================================
// Chord types
std::vector<ChopsDatabase::ChordTypeInfo> ChopsDatabase::getChordTypes(const juce::String& family)
{
    std::vector<ChordTypeInfo> types;
    if (db == nullptr) return types;
    try {
        const char* sqlBase = "SELECT type_key, display_name, intervals, family, complexity FROM chord_types";
        juce::String sql = juce::String(sqlBase) + (family.isEmpty() ? "" : " WHERE family = ?") + " ORDER BY family, complexity, type_key";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), toStdString(sql).c_str(), -1, &stmt, nullptr) != SQLITE_OK) return types;
        
        if (family.isNotEmpty()) {
            sqlite3_bind_text(stmt, 1, toStdString(family).c_str(), -1, SQLITE_TRANSIENT);
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ChordTypeInfo info;
            info.typeKey = fromSqliteText(sqlite3_column_text(stmt, 0));
            info.displayName = fromSqliteText(sqlite3_column_text(stmt, 1));
            info.intervals = parseJsonArray(fromSqliteText(sqlite3_column_text(stmt, 2)));
            info.family = fromSqliteText(sqlite3_column_text(stmt, 3));
            info.complexity = sqlite3_column_int(stmt, 4);
            types.push_back(info);
        }
        sqlite3_finalize(stmt);
    } catch (...) { juce::Logger::writeToLog("Error getting chord types"); }
    return types;
}

//==============================================================================
// Statistics and analysis (continued)
juce::StringArray ChopsDatabase::getDistinctRootNotes()
{
    juce::StringArray notes;
    // ... (implementation from previous correct version) ...
    if (db == nullptr) return notes;
    try {
        const char* sql = "SELECT DISTINCT root_note FROM samples WHERE root_note IS NOT NULL AND root_note != '' ORDER BY root_note";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return notes;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            notes.add(fromSqliteText(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
    } catch (...) { juce::Logger::writeToLog("Error getting distinct root notes"); }
    return notes;
}

juce::StringArray ChopsDatabase::getDistinctChordTypes()
{
    juce::StringArray types;
    // ... (implementation from previous correct version) ...
    if (db == nullptr) return types;
    try {
        const char* sql = "SELECT DISTINCT chord_type FROM samples WHERE chord_type IS NOT NULL AND chord_type != '' ORDER BY chord_type";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), sql, -1, &stmt, nullptr) != SQLITE_OK) return types;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            types.add(fromSqliteText(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
    } catch (...) { juce::Logger::writeToLog("Error getting distinct chord types"); }
    return types;
}

ChopsDatabase::Statistics ChopsDatabase::getStatistics()
{
    Statistics stats;
    if (db == nullptr) return stats;
    try {
        sqlite3_stmt* stmt;
        // Total samples
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "SELECT COUNT(*) FROM samples", -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) stats.totalSamples = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
        // By chord type
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "SELECT chord_type, COUNT(*) as count FROM samples WHERE chord_type IS NOT NULL AND chord_type != '' GROUP BY chord_type ORDER BY count DESC", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.byChordType.push_back({fromSqliteText(sqlite3_column_text(stmt, 0)), sqlite3_column_int(stmt, 1)});
            }
            sqlite3_finalize(stmt);
        }
        // By root note
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "SELECT root_note, COUNT(*) as count FROM samples WHERE root_note IS NOT NULL AND root_note != '' GROUP BY root_note ORDER BY root_note", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.byRootNote.push_back({fromSqliteText(sqlite3_column_text(stmt, 0)), sqlite3_column_int(stmt, 1)});
            }
            sqlite3_finalize(stmt);
        }
        // With extensions
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "SELECT COUNT(*) FROM samples WHERE extensions IS NOT NULL AND extensions != '[]'", -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) stats.withExtensions = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
        // With alterations
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "SELECT COUNT(*) FROM samples WHERE alterations IS NOT NULL AND alterations != '[]'", -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) stats.withAlterations = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
        // Added last week
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "SELECT COUNT(*) FROM samples WHERE date_added > datetime('now', '-7 days')", -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) stats.addedLastWeek = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
    } catch (...) { juce::Logger::writeToLog("Error getting statistics"); }
    return stats;
}


//==============================================================================
// Transaction support
bool ChopsDatabase::beginTransaction() { /* ... unchanged ... */ return db && sqlite3_exec(static_cast<sqlite3*>(db), "BEGIN TRANSACTION", nullptr, nullptr, nullptr) == SQLITE_OK; }
bool ChopsDatabase::commitTransaction() { /* ... unchanged ... */ return db && sqlite3_exec(static_cast<sqlite3*>(db), "COMMIT", nullptr, nullptr, nullptr) == SQLITE_OK; }
bool ChopsDatabase::rollbackTransaction() { /* ... unchanged ... */ return db && sqlite3_exec(static_cast<sqlite3*>(db), "ROLLBACK", nullptr, nullptr, nullptr) == SQLITE_OK; }

//==============================================================================
// Database maintenance
bool ChopsDatabase::vacuum() { /* ... unchanged, but add try-catch ... */ 
    if (!db) return false;
    juce::Logger::writeToLog("Running database VACUUM...");
    try {
        if (sqlite3_exec(static_cast<sqlite3*>(db), "VACUUM", nullptr, nullptr, nullptr) == SQLITE_OK) {
            juce::Logger::writeToLog("VACUUM successful.");
            return true;
        }
        juce::Logger::writeToLog("VACUUM failed: " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
    } catch (...) { juce::Logger::writeToLog("Exception during VACUUM"); }
    return false;
}
bool ChopsDatabase::analyze() { /* ... unchanged, but add try-catch ... */ 
    if (!db) return false;
    juce::Logger::writeToLog("Running database ANALYZE...");
    try {
        if (sqlite3_exec(static_cast<sqlite3*>(db), "ANALYZE", nullptr, nullptr, nullptr) == SQLITE_OK) {
            juce::Logger::writeToLog("ANALYZE successful.");
            return true;
        }
        juce::Logger::writeToLog("ANALYZE failed: " + juce::String(sqlite3_errmsg(static_cast<sqlite3*>(db))));
    } catch (...) { juce::Logger::writeToLog("Exception during ANALYZE"); }
    return false;
}

juce::String ChopsDatabase::getDatabaseInfo()
{
    // ... (implementation from previous correct version, ensure sqlite3_prepare_v2 checks results) ...
    if (db == nullptr) return "Database not open";
    juce::String info;
    try {
        info += "SQLite Version: " + juce::String(sqlite3_libversion()) + "\n";
        sqlite3_stmt* stmt;
        int64 pageCount = 0, pageSize = 0;
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "PRAGMA page_count", -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) pageCount = sqlite3_column_int64(stmt, 0);
            sqlite3_finalize(stmt);
        }
        if (sqlite3_prepare_v2(static_cast<sqlite3*>(db), "PRAGMA page_size", -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) pageSize = sqlite3_column_int64(stmt, 0);
            sqlite3_finalize(stmt);
        }
        info += "Database Size: " + juce::String::formatted("%.2f MB", (pageCount * pageSize) / (1024.0 * 1024.0)) + "\n";
        
        auto statsData = getStatistics(); // Renamed to avoid conflict with struct name
        info += "Total Samples: " + juce::String(statsData.totalSamples) + "\n";
        info += "Total Tags: " + juce::String(getAllTags().size()) + "\n"; // Can be slow if many tags
    } catch (...) { info += "Error retrieving some database info.\n"; }
    return info;
}

//==============================================================================
// Helper methods for SampleInfo (as per ChopsDatabase.h)
juce::String ChopsDatabase::SampleInfo::getFullChordName() const
{
    juce::String name = rootNote;
    
    // Get display quality - use chordTypeDisplay if available, otherwise map from chordType
    if (chordTypeDisplay.isNotEmpty() && chordTypeDisplay != rootNote)
    {
        // chordTypeDisplay already contains the full formatted name
        return chordTypeDisplay;
    }
    
    // Fallback: build from components
    auto qualityMap = ChordTypes::getQualityDisplayMap();
    auto it = qualityMap.find(chordType.toStdString());
    if (it != qualityMap.end() && it->second.isNotEmpty())
    {
        name += it->second;
    }
    else if (chordType.isNotEmpty() && chordType != "maj")
    {
        name += chordType;
    }
    
    // Add suspensions
    for (const auto& sus : suspensions)
    {
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
    
    // Add bass note if different from root
    if (bassNote.isNotEmpty() && bassNote != rootNote)
        name += "/" + bassNote;
    
    return name;
}

juce::String ChopsDatabase::SampleInfo::getShortChordName() const {
    return rootNote + chordType; // Or chordTypeDisplay if preferred
}
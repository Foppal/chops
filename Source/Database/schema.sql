-- Chops Library Database Schema

CREATE TABLE IF NOT EXISTS samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    original_filename TEXT NOT NULL,
    current_filename TEXT NOT NULL,
    file_path TEXT NOT NULL UNIQUE,
    file_size INTEGER,
    
    root_note TEXT,
    chord_type TEXT,
    chord_type_display TEXT,
    
    extensions TEXT,
    alterations TEXT,
    added_notes TEXT,
    suspensions TEXT,
    
    bass_note TEXT,
    inversion TEXT,
    
    date_added TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    date_modified TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    processing_version TEXT,
    
    search_text TEXT,
    
    duration_ms INTEGER,
    sample_rate INTEGER,
    bit_depth INTEGER,
    channels INTEGER,
    bpm REAL,
    musical_key TEXT,
    
    rating INTEGER DEFAULT 0 CHECK (rating >= 0 AND rating <= 5),
    color_hex TEXT,
    is_favorite INTEGER DEFAULT 0,
    play_count INTEGER DEFAULT 0,
    user_notes TEXT,
    last_played TIMESTAMP
);

CREATE TABLE IF NOT EXISTS tags (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS sample_tags (
    sample_id INTEGER NOT NULL,
    tag_id INTEGER NOT NULL,
    date_added TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (sample_id, tag_id),
    FOREIGN KEY (sample_id) REFERENCES samples(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_samples_root_note ON samples(root_note);
CREATE INDEX IF NOT EXISTS idx_samples_chord_type ON samples(chord_type);
CREATE INDEX IF NOT EXISTS idx_samples_search_text ON samples(search_text);
CREATE INDEX IF NOT EXISTS idx_samples_rating ON samples(rating);
CREATE INDEX IF NOT EXISTS idx_samples_is_favorite ON samples(is_favorite);

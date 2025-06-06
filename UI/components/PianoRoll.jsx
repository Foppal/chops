// PianoRoll.jsx - Interactive Piano with Realistic Design
import React, { useState, useEffect, useCallback, useRef } from "react";

const PianoRoll = ({
  highlightedNotes = [],
  onNotesChange,
  allowInput = true,
  octaves = 2,
  startOctave = 3,
  showNoteNames = false,
}) => {
  const [activeNotes, setActiveNotes] = useState(new Set());
  const pianoRef = useRef(null);

  // Piano key configuration
  const whiteKeys = ["C", "D", "E", "F", "G", "A", "B"];
  const blackKeys = {
    C: "C#",
    D: "D#",
    F: "F#",
    G: "G#",
    A: "A#",
  };

  // Generate all keys for the specified octave range
  const generateKeys = useCallback(() => {
    const keys = [];

    for (let octave = startOctave; octave < startOctave + octaves; octave++) {
      whiteKeys.forEach((note, index) => {
        const noteName = `${note}${octave}`;
        const noteOnly = note;
        const midiNote = noteToMidi(note, octave);

        keys.push({
          note,
          octave,
          noteName,
          noteOnly,
          midiNote,
          isBlack: false,
          isHighlighted:
            highlightedNotes.includes(note) ||
            highlightedNotes.includes(noteName),
          isActive: activeNotes.has(noteName),
          keyIndex: octave * 7 + index,
        });

        // Add black key if it exists
        if (blackKeys[note]) {
          const blackNote = blackKeys[note];
          const blackNoteName = `${blackNote}${octave}`;
          const blackMidiNote = noteToMidi(blackNote, octave);

          keys.push({
            note: blackNote,
            octave,
            noteName: blackNoteName,
            noteOnly: blackNote,
            midiNote: blackMidiNote,
            isBlack: true,
            isHighlighted:
              highlightedNotes.includes(blackNote) ||
              highlightedNotes.includes(blackNoteName),
            isActive: activeNotes.has(blackNoteName),
            keyIndex: octave * 7 + index + 0.5,
          });
        }
      });
    }

    return keys;
  }, [highlightedNotes, activeNotes, octaves, startOctave]);

  const keys = generateKeys();

  // Handle key toggle (click to add/remove)
  const handleKeyToggle = useCallback(
    (keyData) => {
      if (!allowInput) return;

      const newActiveNotes = new Set(activeNotes);

      // Toggle the key - if it's active, remove it; if not, add it
      if (newActiveNotes.has(keyData.noteName)) {
        newActiveNotes.delete(keyData.noteName);
      } else {
        newActiveNotes.add(keyData.noteName);
      }

      setActiveNotes(newActiveNotes);

      // Convert to note names only (without octave) for chord recognition
      const noteNames = Array.from(newActiveNotes).map((noteName) =>
        noteName.slice(0, -1)
      );
      const uniqueNotes = [...new Set(noteNames)];

      if (onNotesChange) {
        onNotesChange(uniqueNotes);
      }
    },
    [activeNotes, allowInput, onNotesChange]
  );

  // Group keys by type for rendering
  const whiteKeysData = keys.filter((k) => !k.isBlack);
  const blackKeysData = keys.filter((k) => k.isBlack);

  return (
    <div className="piano-roll" ref={pianoRef}>
      <div className="piano-container">
        {/* White Keys */}
        <div className="white-keys">
          {whiteKeysData.map((keyData) => (
            <div
              key={keyData.noteName}
              className={`piano-key white-key ${
                keyData.isHighlighted ? "highlighted" : ""
              } ${keyData.isActive ? "active" : ""}`}
              onClick={() => handleKeyToggle(keyData)}
              data-note={keyData.note}
              data-octave={keyData.octave}
            >
              {showNoteNames && (
                <div className="note-label">{keyData.note}</div>
              )}
            </div>
          ))}
        </div>

        {/* Black Keys */}
        <div className="black-keys">
          {blackKeysData.map((keyData) => {
            // Calculate position based on the white key pattern
            const whiteKeyIndex = whiteKeysData.findIndex(
              (wk) => Math.floor(wk.keyIndex) === Math.floor(keyData.keyIndex)
            );

            // Better positioning for black keys
            let leftPosition;
            const noteInOctave = keyData.note.charAt(0);
            switch (noteInOctave) {
              case "C":
                leftPosition =
                  (whiteKeyIndex + 0.7) * (100 / whiteKeysData.length);
                break;
              case "D":
                leftPosition =
                  (whiteKeyIndex + 0.7) * (100 / whiteKeysData.length);
                break;
              case "F":
                leftPosition =
                  (whiteKeyIndex + 0.7) * (100 / whiteKeysData.length);
                break;
              case "G":
                leftPosition =
                  (whiteKeyIndex + 0.7) * (100 / whiteKeysData.length);
                break;
              case "A":
                leftPosition =
                  (whiteKeyIndex + 0.7) * (100 / whiteKeysData.length);
                break;
              default:
                leftPosition =
                  (whiteKeyIndex + 0.7) * (100 / whiteKeysData.length);
            }

            return (
              <div
                key={keyData.noteName}
                className={`piano-key black-key ${
                  keyData.isHighlighted ? "highlighted" : ""
                } ${keyData.isActive ? "active" : ""}`}
                style={{
                  left: `${leftPosition}%`,
                }}
                onClick={() => handleKeyToggle(keyData)}
                data-note={keyData.note}
                data-octave={keyData.octave}
              >
                {showNoteNames && (
                  <div className="note-label">{keyData.note}</div>
                )}
              </div>
            );
          })}
        </div>
      </div>

      {/* Piano Info Bar */}
      <div className="piano-info">
        <div className="octave-info">
          <span className="octave-display">
            C{startOctave} - C{startOctave + octaves}
          </span>
        </div>

        {/* Active Notes Display */}
        {activeNotes.size > 0 && (
          <div className="active-notes">
            <span className="active-notes-label">Playing: </span>
            <span className="notes-display">
              {Array.from(activeNotes)
                .map((note) => note.slice(0, -1))
                .join(", ")}
            </span>
          </div>
        )}

        <div className="piano-hint">Click keys to build chords</div>
      </div>
    </div>
  );
};

// Helper function to convert note name and octave to MIDI number
function noteToMidi(note, octave) {
  const noteValues = {
    C: 0,
    "C#": 1,
    D: 2,
    "D#": 3,
    E: 4,
    F: 5,
    "F#": 6,
    G: 7,
    "G#": 8,
    A: 9,
    "A#": 10,
    B: 11,
    Db: 1,
    Eb: 3,
    Gb: 6,
    Ab: 8,
    Bb: 10,
  };

  return (octave + 1) * 12 + (noteValues[note] || 0);
}

export default PianoRoll;

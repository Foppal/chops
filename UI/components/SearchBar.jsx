// SearchBar.jsx - Simple Search Input Component (No Dropdown)
import React, { useState, useEffect, useRef, useCallback } from "react";

const SearchBar = ({
  value = "",
  onChange,
  placeholder = "Search chords...",
  showSuggestions = false, // Allow disabling suggestions
  className = "",
}) => {
  const [inputValue, setInputValue] = useState(value);
  const inputRef = useRef(null);

  // Update input value when prop changes
  useEffect(() => {
    setInputValue(value);
  }, [value]);

  // Handle input change
  const handleInputChange = useCallback(
    (e) => {
      const newValue = e.target.value;
      setInputValue(newValue);

      if (onChange) {
        onChange(newValue);
      }
    },
    [onChange]
  );

  // Handle key press
  const handleKeyDown = useCallback(
    (e) => {
      if (e.key === "Enter" && onChange) {
        onChange(inputValue);
      }
    },
    [inputValue, onChange]
  );

  // Clear search
  const clearSearch = useCallback(() => {
    setInputValue("");

    if (onChange) {
      onChange("");
    }

    inputRef.current?.focus();
  }, [onChange]);

  return (
    <div className={`search-container ${className}`}>
      <div className="search-input-wrapper">
        <input
          ref={inputRef}
          type="text"
          className="search-input"
          value={inputValue}
          onChange={handleInputChange}
          onKeyDown={handleKeyDown}
          placeholder={placeholder}
          autoComplete="off"
          spellCheck="false"
        />

        {inputValue && (
          <button
            className="clear-search-btn"
            onClick={clearSearch}
            type="button"
            aria-label="Clear search"
          >
            ×
          </button>
        )}

        <div className="search-icon">🔍</div>
      </div>
    </div>
  );
};

export default SearchBar;

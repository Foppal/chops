// ResultsList.jsx - Sample Display and Interaction Component
import React, { useState, useEffect, useMemo } from "react";

const ResultsList = ({
  samples = [],
  onSampleSelect,
  currentChord,
  sortBy = "relevance",
  filterBy = "all",
  showFilenames = false,
}) => {
  const [selectedSampleId, setSelectedSampleId] = useState(null);
  const [hoveredSampleId, setHoveredSampleId] = useState(null);
  const [previewPlaying, setPreviewPlaying] = useState(null);

  // Listen for preview state updates from C++
  useEffect(() => {
    if (window.ChopsBridge) {
      window.ChopsBridge.setCallback("onPreviewState", (state) => {
        if (state.isPlaying) {
          // Keep the current preview playing state
        } else {
          // Stop all previews
          setPreviewPlaying(null);
        }
      });
    }
  }, []);

  // Sort and filter samples
  const processedSamples = useMemo(() => {
    let filteredSamples = [...samples];

    // Apply filters
    switch (filterBy) {
      case "favorites":
        filteredSamples = filteredSamples.filter((s) => s.isFavorite);
        break;
      case "recent":
        filteredSamples = filteredSamples.filter((s) => {
          const weekAgo = new Date();
          weekAgo.setDate(weekAgo.getDate() - 7);
          return new Date(s.dateAdded) > weekAgo;
        });
        break;
      case "high-rated":
        filteredSamples = filteredSamples.filter((s) => s.rating >= 4);
        break;
      default:
        // Show all
        break;
    }

    // Apply sorting
    switch (sortBy) {
      case "name":
        filteredSamples.sort((a, b) =>
          a.currentFilename.localeCompare(b.currentFilename)
        );
        break;
      case "date":
        filteredSamples.sort(
          (a, b) => new Date(b.dateAdded) - new Date(a.dateAdded)
        );
        break;
      case "rating":
        filteredSamples.sort((a, b) => b.rating - a.rating);
        break;
      case "relevance":
      default:
        // Keep original order (should be relevance-sorted from backend)
        break;
    }

    return filteredSamples;
  }, [samples, sortBy, filterBy]);

  // Handle sample selection
  const handleSampleClick = (sample, event) => {
    setSelectedSampleId(sample.id);

    if (onSampleSelect) {
      onSampleSelect(sample);
    }

    // Double-click to preview
    if (event.detail === 2) {
      handlePreviewToggle(sample);
    }
  };

  // Handle preview toggle - FIXED to communicate with C++ backend
  const handlePreviewToggle = (sample) => {
    if (previewPlaying === sample.id) {
      // Stop preview
      setPreviewPlaying(null);

      // Send stop message to C++
      if (window.ChopsBridge) {
        window.ChopsBridge.sendMessage("preview", {
          action: "stop",
        });
      }
    } else {
      // Start preview
      setPreviewPlaying(sample.id);

      // Send play message to C++ with sample information
      if (window.ChopsBridge) {
        window.ChopsBridge.sendMessage("preview", {
          action: "play",
          sampleId: sample.id,
          filePath: sample.filePath,
        });
      }
    }
  };

  // Handle drag start for drag-to-DAW functionality
  const handleDragStart = (event, sample) => {
    event.dataTransfer.setData("text/plain", sample.filePath);
    event.dataTransfer.setData(
      "application/x-chops-sample",
      JSON.stringify(sample)
    );
    event.dataTransfer.effectAllowed = "copy";
  };

  // Handle rating change
  const handleRatingChange = (sample, newRating) => {
    if (window.ChopsBridge) {
      window.ChopsBridge.sendMessage("sampleMetadata", {
        action: "setRating",
        sampleId: sample.id,
        rating: newRating,
      });
    }
  };

  // Handle favorite toggle
  const handleFavoriteToggle = (sample) => {
    if (window.ChopsBridge) {
      window.ChopsBridge.sendMessage("sampleMetadata", {
        action: "toggleFavorite",
        sampleId: sample.id,
      });
    }
  };

  if (processedSamples.length === 0) {
    return (
      <div className="results-empty">
        <div className="empty-icon">🎵</div>
        <div className="empty-message">
          No samples found for "{currentChord?.symbol || "current selection"}"
        </div>
        <div className="empty-suggestion">
          Try adjusting your search criteria or browse different chord types
        </div>
      </div>
    );
  }

  return (
    <div className="results-list list-view">
      <SampleListView
        samples={processedSamples}
        selectedId={selectedSampleId}
        previewingId={previewPlaying}
        onSampleClick={handleSampleClick}
        onPreviewToggle={handlePreviewToggle}
        onDragStart={handleDragStart}
        onRatingChange={handleRatingChange}
        onFavoriteToggle={handleFavoriteToggle}
        showFilenames={showFilenames}
      />
    </div>
  );
};

// List View Component
const SampleListView = ({
  samples,
  selectedId,
  previewingId,
  onSampleClick,
  onPreviewToggle,
  onDragStart,
  onRatingChange,
  onFavoriteToggle,
  showFilenames,
}) => (
  <div className="sample-list-view">
    <div className="list-header">
      <div className="col-chord">Chord</div>
      {showFilenames && <div className="col-filename">Filename</div>}
      <div className="col-tags">Tags</div>
      <div className="col-rating">Rating</div>
      <div className="col-actions">Actions</div>
    </div>
    <div className="list-body">
      {samples.map((sample) => (
        <SampleListItem
          key={sample.id}
          sample={sample}
          isSelected={selectedId === sample.id}
          isPreviewing={previewingId === sample.id}
          onClick={(e) => onSampleClick(sample, e)}
          onPreviewToggle={() => onPreviewToggle(sample)}
          onDragStart={(e) => onDragStart(e, sample)}
          onRatingChange={(rating) => onRatingChange(sample, rating)}
          onFavoriteToggle={() => onFavoriteToggle(sample)}
          showFilenames={showFilenames}
        />
      ))}
    </div>
  </div>
);

// Sample List Item Component (for list view)
const SampleListItem = ({
  sample,
  isSelected,
  isPreviewing,
  onClick,
  onPreviewToggle,
  onDragStart,
  onRatingChange,
  onFavoriteToggle,
  showFilenames,
}) => {
  // Get chord name - use fullChordName if available, otherwise build it
  const getChordName = () => {
    if (sample.fullChordName) {
      return sample.fullChordName;
    } else if (sample.getFullChordName) {
      return sample.getFullChordName();
    } else {
      // Fallback: build from available data
      return (
        sample.chordTypeDisplay || sample.rootNote + (sample.chordType || "")
      );
    }
  };

  return (
    <div
      className={`sample-list-item ${isSelected ? "selected" : ""} ${
        showFilenames ? "with-filename" : ""
      } ${isPreviewing ? "previewing" : ""}`}
      onClick={onClick}
      draggable
      onDragStart={onDragStart}
    >
      <div className="col-chord">
        <span className="chord-name">{getChordName()}</span>
        {sample.isFavorite && (
          <button
            className="favorite-star active"
            onClick={(e) => {
              e.stopPropagation();
              onFavoriteToggle();
            }}
            title="Remove from favorites"
          >
            ⭐
          </button>
        )}
        {!sample.isFavorite && (
          <button
            className="favorite-star inactive"
            onClick={(e) => {
              e.stopPropagation();
              onFavoriteToggle();
            }}
            title="Add to favorites"
          >
            ☆
          </button>
        )}
      </div>

      {showFilenames && (
        <div className="col-filename" title={sample.currentFilename}>
          {sample.currentFilename}
        </div>
      )}

      <div className="col-tags">
        {sample.tags && sample.tags.length > 0 ? (
          <>
            {sample.tags.slice(0, 2).map((tag) => (
              <span key={tag} className="tag">
                {tag}
              </span>
            ))}
            {sample.tags.length > 2 && (
              <span className="tag-more">+{sample.tags.length - 2}</span>
            )}
          </>
        ) : (
          <span className="no-tags">—</span>
        )}
      </div>

      <div className="col-rating">
        <StarRating
          rating={sample.rating || 0}
          readonly={false}
          onChange={onRatingChange}
        />
      </div>

      <div className="col-actions">
        <button
          className={`preview-btn ${isPreviewing ? "playing" : ""}`}
          onClick={(e) => {
            e.stopPropagation();
            onPreviewToggle();
          }}
          title={isPreviewing ? "Stop preview" : "Play preview"}
        >
          {isPreviewing ? "⏸" : "▶"}
        </button>

        <button
          className="info-btn"
          onClick={(e) => {
            e.stopPropagation();
            // Could open sample details modal
            console.log("Sample info:", sample);
          }}
          title="Sample details"
        >
          ℹ
        </button>
      </div>
    </div>
  );
};

// Star Rating Component - FIXED to handle changes properly
const StarRating = ({ rating, readonly = false, onChange }) => {
  const [hoveredRating, setHoveredRating] = useState(0);

  const handleStarClick = (value) => {
    if (!readonly && onChange) {
      onChange(value);
    }
  };

  const handleStarHover = (value) => {
    if (!readonly) {
      setHoveredRating(value);
    }
  };

  const handleStarLeave = () => {
    if (!readonly) {
      setHoveredRating(0);
    }
  };

  return (
    <div className={`star-rating ${readonly ? "readonly" : "interactive"}`}>
      {[1, 2, 3, 4, 5].map((value) => (
        <span
          key={value}
          className={`star ${
            value <= (hoveredRating || rating) ? "filled" : ""
          }`}
          onClick={() => handleStarClick(value)}
          onMouseEnter={() => handleStarHover(value)}
          onMouseLeave={handleStarLeave}
          title={`${value} star${value !== 1 ? "s" : ""}`}
        >
          {value <= (hoveredRating || rating) ? "★" : "☆"}
        </span>
      ))}
    </div>
  );
};

export default ResultsList;

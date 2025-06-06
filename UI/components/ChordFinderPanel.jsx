// ChordFinderPanel.jsx - Progressive Chord Builder Component
import React, { useState, useEffect, useMemo } from 'react';
import { ChordFinderTaxonomy, ChordFinderUI } from '../utils/ChordTaxonomy';

const ChordFinderPanel = ({ 
  collapsed, 
  onToggleCollapse, 
  mode, 
  selection, 
  onChange 
}) => {
  const [currentStep, setCurrentStep] = useState(1);
  const [showAdvancedOverlay, setShowAdvancedOverlay] = useState(false);

  // Get available options based on current selections and mode
  const availableTriads = useMemo(() => {
    return ChordFinderUI.filterByMode(
      ChordFinderTaxonomy.typeCategories.chord.triadQualities,
      mode,
      'triads'
    );
  }, [mode]);

  const availableExtensions = useMemo(() => {
    if (!selection.triad) return [];
    return ChordFinderUI.getAvailableExtensions(selection.triad);
  }, [selection.triad]);

  const available7thTypes = useMemo(() => {
    if (selection.extensionCategory !== '7th') return [];
    return ChordFinderUI.getAvailable7thTypes(selection.triad);
  }, [selection.triad, selection.extensionCategory]);

  const availableExtendedTypes = useMemo(() => {
    if (selection.extensionCategory !== 'extended') return [];
    return ChordFinderUI.getAvailableExtendedTypes(selection.triad);
  }, [selection.triad, selection.extensionCategory]);

  const availableIntervals = useMemo(() => {
    return ChordFinderUI.filterByMode(
      ChordFinderTaxonomy.typeCategories.interval.options,
      mode,
      'intervals'
    );
  }, [mode]);

  // Handle step changes
  const handleStepChange = (step, value) => {
    const newSelection = { ...selection, [step]: value };
    
    // Reset dependent steps when parent steps change
    if (step === 'type') {
      newSelection.triad = '';
      newSelection.extensionCategory = '';
      newSelection.specificExtension = '';
      setCurrentStep(newSelection.type === 'interval' ? 2 : 3);
    } else if (step === 'triad') {
      newSelection.extensionCategory = '';
      newSelection.specificExtension = '';
      setCurrentStep(4);
    } else if (step === 'extensionCategory') {
      newSelection.specificExtension = '';
      setCurrentStep(5);
    }
    
    onChange(newSelection);
  };

  // Auto-advance to next step
  useEffect(() => {
    if (selection.root && !selection.type) setCurrentStep(2);
    else if (selection.type === 'chord' && !selection.triad) setCurrentStep(3);
    else if (selection.type === 'interval' && selection.root) setCurrentStep(2);
    else if (selection.triad && !selection.extensionCategory) setCurrentStep(4);
    else if (selection.extensionCategory && !selection.specificExtension) setCurrentStep(5);
  }, [selection]);

  if (collapsed) {
    return (
      <div className="chord-finder-panel collapsed">
        <div className="panel-header">
          <button className="expand-btn" onClick={onToggleCollapse}>
            ▶
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="chord-finder-panel">
      <div className="panel-header">
        <button className="expand-btn" onClick={onToggleCollapse}>
          ▼
        </button>
        <div className="panel-title">Chord Finder</div>
        {mode === 'advanced' && (
          <button 
            className="advanced-btn"
            onClick={() => setShowAdvancedOverlay(true)}
          >
            ⚙
          </button>
        )}
      </div>
      
      <div className="chord-finder-content">
        {/* Step 1: Root Note */}
        <StepContainer 
          number={1} 
          label="Root Note" 
          active={currentStep === 1}
          completed={!!selection.root}
        >
          <NoteSelector
            value={selection.root}
            onChange={(note) => handleStepChange('root', note)}
          />
        </StepContainer>

        {/* Step 2: Type (Chord vs Interval) */}
        <StepContainer 
          number={2} 
          label="Type" 
          active={currentStep === 2}
          completed={!!selection.type}
          disabled={!selection.root}
        >
          <TypeSelector
            value={selection.type}
            onChange={(type) => handleStepChange('type', type)}
          />
        </StepContainer>

        {/* Interval Selection (if type === 'interval') */}
        {selection.type === 'interval' && (
          <StepContainer 
            number={3} 
            label="Interval" 
            active={currentStep === 3}
            completed={!!selection.intervalType}
          >
            <IntervalSelector
              options={availableIntervals}
              value={selection.intervalType}
              onChange={(interval) => handleStepChange('intervalType', interval)}
            />
          </StepContainer>
        )}

        {/* Chord Building Steps (if type === 'chord') */}
        {selection.type === 'chord' && (
          <>
            {/* Step 3: Triad Quality */}
            <StepContainer 
              number={3} 
              label="Triad" 
              active={currentStep === 3}
              completed={!!selection.triad}
              disabled={!selection.type}
            >
              <TriadSelector
                options={availableTriads}
                value={selection.triad}
                onChange={(triad) => handleStepChange('triad', triad)}
              />
            </StepContainer>

            {/* Step 4: Extension Category */}
            <StepContainer 
              number={4} 
              label="Extensions" 
              active={currentStep === 4}
              completed={!!selection.extensionCategory}
              disabled={!selection.triad}
            >
              <ExtensionCategorySelector
                options={availableExtensions}
                value={selection.extensionCategory}
                onChange={(category) => handleStepChange('extensionCategory', category)}
              />
            </StepContainer>

            {/* Step 5: Specific Extension */}
            {selection.extensionCategory && selection.extensionCategory !== 'none' && (
              <StepContainer 
                number={5} 
                label="Specific" 
                active={currentStep === 5}
                completed={!!selection.specificExtension}
                disabled={!selection.extensionCategory}
              >
                {selection.extensionCategory === '7th' && (
                  <SeventhTypeSelector
                    options={available7thTypes}
                    value={selection.specificExtension}
                    onChange={(ext) => handleStepChange('specificExtension', ext)}
                  />
                )}
                
                {selection.extensionCategory === 'extended' && (
                  <ExtendedTypeSelector
                    options={availableExtendedTypes}
                    value={selection.specificExtension}
                    onChange={(ext) => handleStepChange('specificExtension', ext)}
                    triadQuality={selection.triad}
                  />
                )}
                
                {selection.extensionCategory === 'added' && (
                  <AddedNoteSelector
                    mode={mode}
                    value={selection.specificExtension}
                    onChange={(ext) => handleStepChange('specificExtension', ext)}
                  />
                )}
              </StepContainer>
            )}
          </>
        )}

        {/* Current Selection Summary */}
        {(selection.root && selection.type) && (
          <div className="selection-summary">
            <div className="summary-label">Current Selection:</div>
            <div className="summary-chord">
              {ChordFinderUI.buildChordSymbol(selection)}
            </div>
          </div>
        )}
      </div>

      {/* Advanced Overlay */}
      {showAdvancedOverlay && (
        <AdvancedChordOverlay
          selection={selection}
          onChange={onChange}
          onClose={() => setShowAdvancedOverlay(false)}
        />
      )}
    </div>
  );
};

// Sub-components
const StepContainer = ({ number, label, children, active, completed, disabled }) => (
  <div className={`step-container ${active ? 'active' : ''} ${completed ? 'completed' : ''} ${disabled ? 'disabled' : ''}`}>
    <div className="step-header">
      <div className="step-number">{number}</div>
      <div className="step-label">{label}</div>
      {completed && <div className="step-check">✓</div>}
    </div>
    <div className="step-content">
      {children}
    </div>
  </div>
);

const NoteSelector = ({ value, onChange }) => (
  <div className="note-selector">
    {ChordFinderTaxonomy.rootNotes.map(note => (
      <button
        key={note}
        className={`note-btn ${value === note ? 'selected' : ''}`}
        onClick={() => onChange(note)}
      >
        {note}
      </button>
    ))}
  </div>
);

const TypeSelector = ({ value, onChange }) => (
  <div className="type-selector">
    <button
      className={`type-btn ${value === 'chord' ? 'selected' : ''}`}
      onClick={() => onChange('chord')}
    >
      Chord
    </button>
    <button
      className={`type-btn ${value === 'interval' ? 'selected' : ''}`}
      onClick={() => onChange('interval')}
    >
      Interval
    </button>
  </div>
);

const TriadSelector = ({ options, value, onChange }) => (
  <div className="dropdown-container">
    <select 
      className="dropdown"
      value={value}
      onChange={(e) => onChange(e.target.value)}
    >
      <option value="">Choose triad...</option>
      {Object.entries(options).map(([key, triad]) => (
        <option key={key} value={key}>
          {triad.displayName || key.charAt(0).toUpperCase() + key.slice(1)}
        </option>
      ))}
    </select>
  </div>
);

const ExtensionCategorySelector = ({ options, value, onChange }) => (
  <div className="dropdown-container">
    <select 
      className="dropdown"
      value={value}
      onChange={(e) => onChange(e.target.value)}
    >
      <option value="">Choose extension...</option>
      {options.map(category => (
        <option key={category} value={category}>
          {ChordFinderTaxonomy.extensionCategories[category]?.name || category}
        </option>
      ))}
    </select>
  </div>
);

const SeventhTypeSelector = ({ options, value, onChange }) => (
  <div className="dropdown-container">
    <select 
      className="dropdown"
      value={value}
      onChange={(e) => onChange(e.target.value)}
    >
      <option value="">Choose 7th type...</option>
      {options.map(option => (
        <option key={option.key} value={option.key}>
          {option.displayName || option.symbol}
        </option>
      ))}
    </select>
  </div>
);

const ExtendedTypeSelector = ({ options, value, onChange, triadQuality }) => (
  <div className="extended-type-selector">
    {options.map(option => (
      <div key={option.key} className="extended-option">
        <button
          className={`extended-btn ${value === option.key ? 'selected' : ''}`}
          onClick={() => onChange(option.key)}
        >
          {option.symbol}
        </button>
        {option.variants && (
          <div className="variant-buttons">
            {Object.entries(option.variants).map(([variantKey, variant]) => (
              <button
                key={variantKey}
                className={`variant-btn ${variantKey === option.defaultVariant ? 'default' : ''}`}
                onClick={() => onChange(option.key, variantKey)}
              >
                {variant.symbol}
              </button>
            ))}
          </div>
        )}
      </div>
    ))}
  </div>
);

const AddedNoteSelector = ({ mode, value, onChange }) => {
  const options = ChordFinderUI.filterByMode(
    ChordFinderTaxonomy.extensionCategories.added.options,
    mode,
    'added'
  );

  return (
    <div className="dropdown-container">
      <select 
        className="dropdown"
        value={value}
        onChange={(e) => onChange(e.target.value)}
      >
        <option value="">Choose added note...</option>
        {Object.entries(options).map(([key, option]) => (
          <option key={key} value={key}>
            {option.symbol}
          </option>
        ))}
      </select>
    </div>
  );
};

const IntervalSelector = ({ options, value, onChange }) => (
  <div className="interval-grid">
    {Object.entries(options).map(([key, interval]) => (
      <button
        key={key}
        className={`interval-btn ${value === key ? 'selected' : ''}`}
        onClick={() => onChange(key)}
        title={interval.description}
      >
        {interval.symbol}
      </button>
    ))}
  </div>
);

const AdvancedChordOverlay = ({ selection, onChange, onClose }) => (
  <div className="advanced-overlay show">
    <div className="overlay-content">
      <button className="close-btn" onClick={onClose}>×</button>
      <h3>Advanced Chord Builder</h3>
      <div className="advanced-sections">
        <div className="alterations-section">
          <h4>Alterations</h4>
          {/* Alteration controls */}
        </div>
        <div className="voicing-section">
          <h4>Voicings</h4>
          {/* Voicing controls */}
        </div>
        <div className="inversion-section">
          <h4>Inversions</h4>
          {/* Inversion controls */}
        </div>
      </div>
    </div>
  </div>
);

export default ChordFinderPanel;
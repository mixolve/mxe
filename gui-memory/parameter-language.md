# Parameter Language

## Naming Rules

- use uppercase in visible UI labels when the control is part of the settled MX6 GUI language
- use hyphenated words where that improves scan speed
- use short technical names instead of long descriptive names
- all bypass buttons are named exactly `BYPASS`

## Current Section Vocabulary

### HALF-WAVE

- `L-UP-THRESHOLD`
- `L-UP-MAKEUP`
- `L-DOWN-THRESHOLD`
- `L-DOWN-MAKEUP`
- `R-UP-THRESHOLD`
- `R-UP-MAKEUP`
- `R-DOWN-THRESHOLD`
- `R-DOWN-MAKEUP`
- `BYPASS`

### DUAL-MONO

- `L-THRESHOLD`
- `L-TENSION`
- `L-RELEASE`
- `L-MAKEUP`
- `R-THRESHOLD`
- `R-TENSION`
- `R-RELEASE`
- `R-MAKEUP`
- `BYPASS`

### FF/GLOBAL

- `TENSION`
- `RELEASE`
- `BYPASS`
- `INPUT-GAIN`
- `IN-RIGHT`
- `IN-LEFT`
- `WIDE`
- `MORPH`
- `PEAK-HOLD`
- `TEN-FLOOR`
- `TEN-HYST`
- `DELTA`

## Naming Intent

- `INPUT-GAIN` is global input trim
- `IN-RIGHT` and `IN-LEFT` are side-specific input trims
- `WIDE` is stereo width control
- `DELTA` stays explicit and short

## What To Preserve In Another Plugin

- section titles
- uppercase style
- `BYPASS` unification
- compact scan-friendly labels
- value formatting style

## What May Change Per Plugin

- exact section set
- top-row mode buttons
- per-plugin tool sections
- parameter counts and grouping

# Source Map

This is where the current GUI rules live in code.

## Main UI Implementation

- `src/PluginEditor.cpp`

Contains:
- color and metric constants
- embedded font usage
- value formatting
- reusable GUI primitives used by the editor
- section accordion logic
- top row monitor logic
- footer and empty-state layout

## Parameter Formatting

- `src/PluginProcessor.cpp`

Contains:
- parameter string formatting used by APVTS float parameters
- parameter ranges and defaults
- parameter IDs and layout definitions

## DSP Side Only Relevant To GUI Labels

- `src/dsp/DspCore.h`
- `src/dsp/DspCore.cpp`

Relevant because:
- GUI labels map to fields in `DspCore::Parameters`
- new controls such as `IN-RIGHT`, `IN-LEFT`, `WIDE` must match these structures

## Embedded Font Assets

- `assets/fonts/SometypeMono-Regular.ttf`
- `assets/fonts/SometypeMono-Bold.ttf`
- `assets/fonts/SometypeMono-OFL.txt`

## Build Integration For Embedded Fonts

- `CMakeLists.txt`

Check for:
- BinaryData generation
- asset inclusion for the font files

# mx6

JUCE scaffold for porting the DSP from [`jsfx/V3.jsfx`](jsfx/V3.jsfx) into a macOS plugin target.

## Current state

- `external/JUCE` contains a checked out copy of JUCE `8.0.12`
- `src/PluginProcessor.*` provides the plugin shell and parameter layout
- `src/dsp/DspCore.*` is the isolated DSP port target and now contains a direct C++ port of the `V3.jsfx` processing path
- `src/PluginEditor.*` embeds a generic parameter editor so every mapped control is visible immediately
- `tools/mx6_raw_render.cpp` builds a CLI renderer that runs `DspCore` directly, without host PDC
- `sdk-shims/Foundation/NSGarbageCollector.h` works around a missing legacy header in Xcode 26.3's macOS SDK
- The project builds only a `VST3` plugin target
- After each successful `VST3` build, the bundle is deployed to `/Library/Audio/Plug-Ins/ALL`

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build -j 4
```

This produces:

- `build/mx6_artefacts/VST3/mx6.vst3`
- `build/mx6_raw_render_artefacts/mx6_raw_render`

## VSCode

Use VSCode as the editor and drive the build with CMake/Ninja.

- Recommended extensions: `ms-vscode.cpptools`, `ms-vscode.cmake-tools`
- Configure once: `cmake -S . -B build -G Ninja`
- Build: `cmake --build build -j 4`
- Output plugin: `build/mx6_artefacts/VST3/mx6.vst3`
- Deployed copy: `/Library/Audio/Plug-Ins/ALL/mx6.vst3`

## Recommended next porting order

1. Compare the C++ output against `V3.jsfx` with null tests at a few settings
2. Add explicit regression tests for default settings, bypass states, and delta mode
3. Refactor the DSP into smaller units only after the null tests pass

## Parity workflow

Use the raw renderer for DSP parity so host latency compensation does not contaminate the comparison.

```bash
tools/run_reaper_parity_default.sh
```

That script:

- renders `V3.jsfx` inside REAPER
- renders `mx6` through `mx6_raw_render`
- writes the null test report to `tmp/parity_default/null_stats.txt`
- writes the alignment report to `tmp/parity_default/alignment_stats.txt`

## Notes

- Parameter IDs already mirror the active JSFX slider names
- `DspCore` mirrors the JSFX lookahead path, and the VST3 reports latency based on `peakHoldHz`
- REAPER parity against the VST3 itself is misleading because host PDC shifts the rendered output earlier; use `mx6_raw_render` for DSP checks

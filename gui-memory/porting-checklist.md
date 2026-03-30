# Porting Checklist

## Minimum GUI Transfer

Copy or recreate:
- embedded `Sometype Mono` font setup
- color tokens
- geometry constants
- `BoxTextButton`
- value box component and its interactions
- parameter row component
- section accordion behavior
- changed-state outline logic
- value formatting

## Required Visual Rules

- fixed-width compact editor
- square controls
- white text only
- grey surfaces
- `#9999ff` active accent
- `#ffcc99` changed accent
- no rounded corners
- no gradients

## Required Interaction Rules

- drag on value box
- wheel / trackpad on value box
- right-click direct entry
- `Shift + click` reset
- close active text edit on outside click

## Parameter Layer Rules

- float parameters should expose the same formatted string style
- default values matter because changed-state compares against defaults
- `BYPASS` and `DELTA` must stay excluded from changed-state highlighting

## When Porting To Another Plugin

1. Move the style tokens first.
2. Move the reusable box/button/value components second.
3. Recreate parameter labels and sections from data, not ad hoc layout code.
4. Reconnect to the new plugin's `APVTS` parameter IDs.
5. Recheck:
   - fixed width
   - footer
   - empty state behavior
   - top row behavior
   - reset behavior
   - drag/wheel feel

## If You Want True Reuse Later

Best next step:
- extract current GUI primitives into `src/ui/`
- keep plugin-specific section definitions separate from reusable widgets

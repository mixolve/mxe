# Interactions

## Value Box

Current behavior:
- left-drag changes value
- right-click opens direct text entry
- `Shift + click` resets to default
- wheel / trackpad adjusts value
- open text entry commits on click outside
- open text entry commits on return
- open text entry commits on focus loss
- escape discards text edit

Intent:
- drag is the fast gesture
- wheel / trackpad is the precise gesture
- direct entry is explicit, not accidental

## Drag

- drag sensitivity is faster than wheel
- drag operates on the value box itself
- drag begins on first left mouse down
- no modifier-based acceleration

## Wheel / Trackpad

- wheel and macOS trackpad both go through the same logical path
- smooth devices accumulate motion before stepping
- wheel changes follow legal parameter steps
- wheel should feel precise, not coarse

## Default Reset Policy

- reset exists only on value box
- parameter name label is passive

## Accordion Behavior

- only one vertical section open at a time
- opening one closes the others
- `FF/GLOBAL` stays open if clicked while already open

## Top Row Behavior

- `1..6` select per-band pages in multiband solo view
- `A` means all-bands output
- `S/M` toggles single-band vs multiband processing
- top row is excluded from changed-state coloring

## Changed-State Coloring

Changed-state uses outline logic, not text color.

Changed-state applies to:
- numeric controls
- relevant section headers

Changed-state does not apply to:
- `BYPASS` controls
- `DELTA`
- top row monitor buttons

## Empty State

When `ALL` is active:
- no band page is shown
- centered text reads `CLEAN-SPACE`

# Design System

## Core Rules

- fixed editor width: `294`
- fixed editor height: `500`
- no rounded corners anywhere
- all text is white
- background and box surfaces use the same grey base
- accent color is reserved for active state
- changed state uses a separate accent
- uppercase is used for section headers and top tabs
- numeric values use a fixed signed format

## Font

- family: `Sometype Mono`
- source: embedded font assets, not system dependency
- base UI size: `20 pt`
- regular for most text
- bold only when intentionally used by the current implementation

## Colors

- white: `#ffffff`
- black: `#000000`
- accent: `#9999ff`
- changed-accent: `#ffcc99`
- grey-950: `#121212`
- grey-900: `#1a1a1a`
- grey-800: `#242424`
- grey-700: `#363636`
- grey-500: `#707070`

Current practical usage:
- main background: `grey-800`
- section background: `grey-800`
- box fill: `grey-800`
- active outline: `accent`
- changed outline: `changed-accent`
- idle outline: `grey-500`

## Geometry

- editor inset X: `4 px`
- editor bottom inset: `4 px`
- footer height: `20 px`
- control gap between name and value: `2 px`
- value box width: `94 px`
- name box width is the remaining width of the control row
- all controls use square boxes

## Structural Layout

- top row: `1 2 3 4 5 6 A S/M`
- center area: one page visible at a time
- footer: `MX6 by MIXOLVE`
- `ALL` mode shows empty workspace with `CLEAN-SPACE`
- only one vertical section is open at a time
- `FF/GLOBAL` cannot close by clicking itself

## Value Formatting

- value display format: `%+08.1f`

Examples:
- `+0000.0`
- `-0096.0`
- `+0021.0`

Zero rule:
- visually normalize tiny values to `+0000.0`

## Section Titles

- `HALF-WAVE`
- `DUAL-MONO`
- `FF/GLOBAL`

## General Tone

- compact
- technical
- no descriptive helper text
- no comments or explanatory copy in the plugin UI
- no decorative gradients
- no animated decoration

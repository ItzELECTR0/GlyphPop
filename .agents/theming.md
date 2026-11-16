# Theming

The look is GlyphPop's own: a quiet monochrome card with one accent, small radii, 1 px outlines, the system font.
It must never imitate Windows or Fluent, and must never read a particular shell's or bar's config. Every colour,
radius and size in QML comes from `ui/Theme.qml`; do not hardcode colours elsewhere.

## Settings

`ThemeSource` owns these keys in `GlyphPop.conf` `[General]` and writes them as soon as the options page changes
them:

- `theme`: `material` (default) or the name of `~/.config/GlyphPop/themes/<name>.json`.
- `mode`: `system` (default), `light` or `dark`. System follows the portal's `color-scheme`; no preference means
  dark.
- `accent`: `default`, `system`, `orange`, `red`, `pink`, `purple`, `blue`, `teal`, `green`, `yellow`, `custom`.
- `customAccent`: `#rrggbb`.

Before the options page existed, `theme` held `auto`, `dark` or `light`. Those values are read as the mode (only
when `mode` is unset) and as `material`, and are left in the file untouched.

## Material

The built-in theme is Prismora's Material: surface and text `#121212`/`#e6e6e6` dark, `#f4f4f4`/`#161616` light,
primary white or black. Containers, outlines and muted text are mixes of surface and text (4, 7, 10, 15, 32, 17
and 62 percent), computed by `material()` in `themesource.cpp`.

## Accents

`default` is the theme's own `primary`: white or black for Material, whatever a theme file sets. `system` is the
portal's `accent-color` and is only offered while the desktop publishes one. `orange` is the colour GlyphPop
shipped with first (`#ff9433` dark, `#c8650a` light). Each named accent has a dark and a light value in `Accents`.
`on_primary` is black or white by luminance.

## Theme files

Watched live; the folder list is rescanned whenever the options page opens, because a folder created after
startup is not watched yet.

```json
{ "base": "dark", "radius": 10, "colors": { "surface": "#101820", "on_surface": "#dfe8f0", "primary": "#8ecfd4" },
  "light": { "surface": "#f0f4f8" } }
```

Everything is optional. `base` fixes the mode and greys out the Mode row. `colors` applies in both modes and a
`dark` or `light` object adds per-mode overrides. Containers and lines are re-derived from the file's `surface`
and `on_surface` before explicit overrides apply, so setting just those two gives a coherent theme.

Token names are Material roles, so any template generator can target them: `surface`,
`surface_container_low`, `surface_container`, `surface_container_high`, `surface_container_highest`,
`on_surface`, `on_surface_variant`, `outline`, `outline_variant`, `primary`, `on_primary`, `shadow`. Derived
values (`hover`, `pressed`, `selection`) are alpha blends in `Theme.qml`.

## Options page

`ui/OptionsPage.qml`, reached from the tune button or `glyphpop settings`, replaces the picker inside the card
(`Picker.options`). Esc goes back to the grid. Controls are drawn from plain QtQuick items (`Segmented`,
`IconButton`, `ColorPicker`, drawn `TuneIcon` and `BackIcon`) so nothing depends on an icon theme or a Qt Quick
Controls style.

## Scrolling

Scroll views take no mouse buttons (`acceptedButtons: Qt.NoButton`), so a mouse drag never scrolls them; touch and touchpads still do. `WheelScroll` moves `wheelScrollLines` times 24 px per wheel notch, because Flickable turns each wheel event into a speed-capped flick that crawls, worst with high-resolution wheels. It is a MouseArea under the content with `scrollGestureEnabled: false`, so controls get the wheel first and touchpad scrolls (synthesized events) fall through to Flickable's own kinetic scrolling. A WheelHandler cannot do this: on Wayland, wheel events carry Qt's touchpad device whenever the compositor offers pointer gestures, and the handler blocks before `onWheel` runs.

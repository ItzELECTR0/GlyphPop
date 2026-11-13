# GlyphPop

GlyphPop is an emoji, kaomoji and symbol picker for Wayland
Inspired by the Windows emoji picker with a familiar design and similar behaviour
It's written in Zig with a Qt Quick interface and tested on Hyprland (sway works too)

## LICENSE

[GPL-3.0-or-later](LICENSE).
The bundled Unicode data is under the [Unicode License v3](data/unicode/LICENSE).

## What can it do?

| What? | How? |
| --- | --- |
| Emoji | All of Emoji 18.0, searchable by name and keyword, with a skin tone picker |
| Kaomoji | A hand-picked set, grouped by mood ( ´ ▽ ` )ﾉ |
| Symbols | Punctuation, currency, maths, arrows, Greek, box drawing and more |
| Recently used | Your last picks come first in every tab |
| Themes | Light and dark that follow your system, a choice of accent colours, or your own theme file |

In apps that support Wayland's text input (GTK, Qt, Firefox and most native apps) the picker opens right under the caret and the emoji goes straight into the field. Everywhere else it opens at your mouse pointer and types the emoji for you. X11 apps running through xwayland-satellite don't accept that typing yet.

## Usage

GlyphPop keeps one instance running so the keybind is instant. Start it at login:

```lua
hl.on("hyprland.start", function ()
    hl.exec_cmd("glyphpop --daemon")
end)
```

and bind a key:

```lua
hl.bind("SUPER + period", hl.dsp.exec_cmd("glyphpop"))
```

```sh
glyphpop            # open, or close if it's open
glyphpop emoji      # open on a tab (also kaomoji, symbols)
glyphpop settings   # open the settings window
glyphpop close      # close the active window
glyphpop quit       # close any active window and kill the daemon
```

Type to search, arrows to move, `Enter` to insert, `Shift+Enter` or `Shift`+click to insert and keep it open (when it's at the caret), `Tab` to switch tabs, `Page Up`/`Page Down` to jump between categories, `Esc` to close.

### Hyprland permissions

If you use `ecosystem:enforce_permissions` with a keyboard deny rule, let GlyphPop's virtual keyboard through (it's only used outside text-input apps).
Put this before your deny rule and restart Hyprland:

```lua
hl.permission({ binary = "hl-virtual-keyboard-glyphpop", type = "keyboard", mode = "allow" })
```

### Options

Click the sliders button in the picker (or run `glyphpop settings`) to pick a theme, light or dark mode, and an accent colour.

### Themes

You can add your own themes as `~/.config/GlyphPop/themes/<name>.json`. They only need the colours you want to change, and setting just the surface and text colours is enough for a coherent theme:

```json
{ "base": "dark", "colors": { "surface": "#101820", "on_surface": "#dfe8f0", "primary": "#8ecfd4" } }
```

The colour names are Material roles (`surface`, `surface_container`, `on_surface`, `outline`, `primary`...), so you can point matugen, wallust or your bar's template system at it.

GlyphPop is an input method, and Wayland allows one per seat. If you run fcitx5 or ibus, GlyphPop still works but types everything through its virtual keyboard instead.

## Prerequisites

- Zig 0.17 or newer
- Qt 6.8 or newer with Quick, QML, D-Bus and Wayland client (private headers included)
- LayerShellQt
- xkbcommon
- CMake, Ninja and a C++20 compiler
- A colour emoji font, such as Noto Color Emoji

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/glyphpop
```

## Install on Arch

```sh
cd packaging
makepkg -si
```

The package is built from your checkout as it is, uncommitted changes included.

## Test

```sh
zig build test
```

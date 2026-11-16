# GlyphPop agent guide

GlyphPop is an emoji, kaomoji and symbol picker for Wayland, modelled on the Windows emoji panel's behaviour
(opens at the text caret, types into the focused field) but with its own Linux look. The UI is Qt Quick, the host
is C++ in `app/`, and the Unicode data, search and symbol/kaomoji sets live in Zig in `src/`.

- The look is GlyphPop's own: no Fluent or Windows styling, and no reading of any particular shell's config. See
  `.agents/theming.md`.
- Insertion goes through the input-method protocol first and a virtual keyboard second. Never add clipboard
  paste as a silent fallback.
- Hyprland is the target and sway is the exercised test desktop. Do not claim support for anything else.
- Build with CMake (see README). Core tests are `zig build test`.

## Topics

- `.agents/architecture.md`: read before changing the process model, the two window modes, the Zig/C++/QML split
  or the D-Bus commands.
- `.agents/input.md`: read before touching the input method, the popup role, the keyboard grab or the virtual
  keyboard.
- `.agents/unicode-data.md`: read before updating emoji, CLDR or symbol data, or changing search.
- `.agents/theming.md`: read before changing colours, sizes, fonts or the theme file format.
- `.agents/testing.md`: read before running the app, especially on the live desktop.
- `.agents/packaging.md`: read before changing anything in `packaging/` or `.github/workflows/`, or cutting a release.

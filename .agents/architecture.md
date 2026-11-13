# Architecture

## Process model

One resident instance owns the session bus name `io.github.ItzELECTR0.GlyphPop` (object
`/io/github/ItzELECTR0/GlyphPop`, method `Activate(as)`). Any later `glyphpop ...` fails to register the name,
forwards its arguments and exits, so a keybind costs one D-Bus call. `Instance::Activate` in `app/main.cpp`
handles `quit`; everything else goes to `Picker::handleCommand` (`toggle`, `emoji`, `kaomoji`, `symbols`,
`settings`, `close`). A cold `glyphpop` with no daemon becomes the daemon and opens immediately; `--daemon` starts hidden.

`main` forces `QT_QPA_PLATFORM=wayland` and sets `QT_WAYLAND_TEXT_INPUT_PROTOCOL=qt_text_input_method_v1`.
The second is deliberate: our own search field must never enable text-input-v3, or our own input method
activates for our own overlay. `none` is not a valid value there; Qt silently falls back to all protocols. Qt's
own protocol is offered by neither Hyprland nor sway, so nothing is bound.

## Layers

- `src/core.zig` (C ABI in `src/core.h`): parses the embedded Unicode files once at `gp_init`, owns emoji, tones,
  names, search, symbol ranges (`symbols.zig`) and kaomoji (`kaomoji.zig`). Pure: no I/O, no settings. Returned
  pointers are into static storage; search results stay valid until the next search.
- `app/picker.*`: the controller, registered as the `Picker` singleton in `GlyphPop.Core`. Owns mode, section,
  query, tone, selection, recents (QSettings `~/.config/GlyphPop/GlyphPop.conf`) and builds the model.
- `app/glyphmodel.*`: sections flattened into header rows and glyph rows so one `ListView` scrolls everything with
  delegate reuse. Cells also have a flat index, which is what selection and insert use.
- `app/inputmethod.*`, `app/popupshell.*`, `app/virtualkeyboard.*`: see `input.md`.
- `app/themesource.*`: see `theming.md`.
- `ui/`: plain QtQuick items, no Qt Quick Controls style. `App.qml` holds both windows as properties that C++ looks
  up by name (`popupWindow`, `overlayWindow`). The card component is `PickerCard`, not `Picker`, because a local
  type would shadow the singleton.

## Two window modes

When opened, `Picker::open` checks whether the input method is active (a text-input-v3 field has focus):

- Popup: `popupWindow` gets the input-method popup role, the compositor places it at the caret, and we grab the
  keyboard. Focus never leaves the app, so commits land in the right field. It closes on Esc, on insert (unless
  Shift), on deactivate, and on any change the app reports with cause `other` (a click that moves the caret).
- Overlay: `overlayWindow` is a full-output layer-shell surface on the overlay layer with exclusive keyboard. The
  card sits at the pointer, taken from the first hover event (90 ms fallback: centre). A click outside closes it.
  On insert it closes first, waits 150 ms for focus to return, then commits through the input method if that
  became active, else types through the virtual keyboard. Shift has no effect here.

The popup moves with the caret after each commit, because the compositor re-places it. That is visible when
Shift-inserting several glyphs.

## Emoji font

Qt's Unicode tables lag the emoji data (6.12 drew 🫫, Emoji 18.0, as tofu even though Noto had it), so emoji text
uses the family that fontconfig's `emoji` alias resolves to, exposed as `Picker.emojiFamily`. The same family is
used to hide emoji whose first codepoint the font lacks. If fewer than half pass, the alias is assumed missing and
nothing is filtered.

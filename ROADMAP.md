# Roadmap

## Confirm on live Hyprland

- The real app (not just the probe) in popup mode: placement, typing into search, click and Shift+click inserts.
- What Hyprland does when the caret is near the bottom or right edge: whether it flips or clamps the popup.
- Overlay mode: that the pointer position arrives on map, and that keyboard focus returns to the app after close.
- The virtual keyboard with the `hl-virtual-keyboard-glyphpop` permission rule, in a native app without text input.
- Chromium and Electron apps with and without `--enable-wayland-ime`.

## Behaviour

- Keep the popup still while Shift-inserting several glyphs instead of following the caret.
- Coexist with fcitx5 and ibus instead of falling back to the virtual keyboard.
- Typing into X11 apps when the X server ignores per-device keymaps (xwayland-satellite does).
- Open in overlay mode on the first toggle after a daemon restart because the new input method was not activated
  yet; re-check activation on toggle.

## Content

- Translations, including CLDR names and keywords in the user's language.
- Emoji the font cannot draw are hidden only by their first codepoint; ZWJ sequences the font lacks still show.

## Desktops

- KDE (input-method-v1 only) and GNOME (no input-method protocol for clients).
- Packaging for Arch.

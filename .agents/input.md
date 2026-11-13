# Input

Protocol XMLs are vendored in `protocols/` from wlroots (`input-method-unstable-v2`,
`virtual-keyboard-unstable-v1`); neither ships in wayland-protocols. CMake generates Qt wrappers with
`qt6_generate_wayland_protocol_client_sources(... PRIVATE_CODE ...)`, which emits `.c` files, so the project must
enable C as well as CXX or the interface tables silently go missing at link time.

## Input method

`InputMethod` binds `zwp_input_method_v2` on Qt's own `wl_display` and seat at startup and stays bound for the life
of the daemon. Only one input method can exist per seat; if another (fcitx5, ibus) already holds it we get
`unavailable`, log it, and every insert goes through the virtual keyboard.

State is double-buffered per protocol: `active` flips on `done`. Each `done` with change cause `other` (1) while
active emits `externalChange`; cause 0 is our own commit.

Verified on Hyprland (live, with a throwaway probe) and sway (Wardian):

- Binding while a field already has focus activates immediately.
- The popup surface is placed under the caret by the compositor.
- The popup receives pointer events; clicking it leaves keyboard focus in the app.
- A keyboard grab delivers all keys to us; nothing leaks into the field.
- `commit_string` + `commit(serial)` inserts into the field.

## Popup role

`PopupShellIntegration` is a `QWaylandShellIntegration` set per window with the private
`QWaylandWindow::setShellIntegration` (the same hook LayerShellQt uses). Qt creates the shell surface on every
show and destroys it on hide, so each open gets a fresh `zwp_input_popup_surface_v2`. This depends on QtWaylandClient
private headers and must be rebuilt when Qt's minor version changes.

## Keyboard grab

The popup never has keyboard focus, so `KeyboardGrab` translates grabbed keys with xkbcommon and injects them
into the popup window with `QWindowSystemInterface`, after marking it the focus window. Releases of keys that were
already down when the grab began (the shortcut that opened us) are dropped. Key repeat is done locally from
`repeat_info`.

Injected key events are flushed synchronously, which can run QML that closes the picker and releases the grab
while `KeyboardGrab` is still on the stack. `releaseKeyboard` therefore stops and releases immediately but
deletes on the next event-loop turn. Deleting synchronously crashed (timer fired on a freed object).

Pointer events carry Qt's own keyboard modifiers, which a popup never gets, so Shift+click asks
`InputMethod::grabbedModifiers()`. A synthetic click delivered in the same batch as the Shift release (Wardian with
no delay) loses Shift; a real held Shift works.

## Virtual keyboard

`VirtualKeyboard::type` creates a `zwp_virtual_keyboard_v1`, uploads a keymap mapping keys to the needed
codepoints (wtype's approach), presses them, and destroys it. Keys are only ever the unassigned evdev codes 84 and
195-199: a client that ignores our keymap (xwayland-satellite did) would otherwise read evdev 1 as Escape, which
closed an X11 dialog in testing. Six codes means text with more distinct codepoints goes in chunks, each with its
own keymap, paced 20 ms after the keymap and 8 ms per key.

On Hyprland the device is named `hl-virtual-keyboard-glyphpop` (`misc:name_vk_after_proc`, default true; read from
Hyprland's `protocols/VirtualKeyboard.cpp`). With `ecosystem:enforce_permissions` and a catch-all keyboard deny,
it needs an allow rule before the deny, and permissions only load at compositor start:

```lua
hl.permission({ binary = "hl-virtual-keyboard-glyphpop", type = "keyboard", mode = "allow" })
```

X11 apps under xwayland-satellite receive nothing (keymap ignored). Behaviour under real Xwayland is unverified.

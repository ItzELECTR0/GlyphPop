# Testing

## Headless first

Run GUI checks in a Wardian session (headless sway). Point `XDG_CONFIG_HOME` at a scratch directory so recents and
settings never touch the real `~/.config/GlyphPop`.

```sh
wardian -s NAME run --name gp -e XDG_CONFIG_HOME="$SCRATCH/cfg" -- build/glyphpop --daemon
wardian -s NAME run --name entry -- zenity --entry --text=Type
wardian -s NAME exec -- build/glyphpop
```

Useful targets:

- `zenity --entry`: GTK with text-input-v3, exercises popup mode.
- The same with `GTK_IM_MODULE=gtk-im-context-simple`: a Wayland app without text-input, exercises overlay mode
  and the virtual keyboard with a client that honours keymaps.
- `wardian run --x11 -- zenity --entry`: an X11 app through xwayland-satellite, which ignores the virtual
  keyboard's keymap. It must stay open (nothing typed is expected).

After restarting the daemon, click into the field again before toggling, or the new input method may not be
active yet and the overlay opens instead. `WAYLAND_DEBUG=1` on the daemon shows every protocol event in its log
and is the quickest way to see why the picker closed.

Wardian batches with no delay deliver Shift press, click and release before Qt processes the click, so Shift+click
looks broken there. Use `hold shift`, a short wait, the click, then `release`.

## Hyprland

Nested Hyprland does not start inside Wardian: aquamarine binds `xdg_wm_base` version 6 and Wardian's sway offers
5. Hyprland behaviour therefore needs the live session and the user's go-ahead for that run. What has and has not
been confirmed live is in `ROADMAP.md`.

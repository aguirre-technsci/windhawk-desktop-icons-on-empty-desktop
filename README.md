# windhawk-desktop-icons-on-empty-desktop

Windhawk mod: desktop icons are shown only when all windows are minimized.

If any window is visible (windowed, maximized or fullscreen), the desktop icons are hidden. When everything is minimized, they come back.

## Install

1. Install [Windhawk](https://windhawk.net/).
2. In Windhawk, click **Create a new mod**.
3. Replace the sample code with the contents of
   [`icons-only-on-empty-desktop.wh.cpp`](icons-only-on-empty-desktop.wh.cpp).
4. Compile with the button on the left or with `Ctrl+B`.

The mod targets `explorer.exe`, and the icons are restored when the mod is disabled or unloaded.

## How it works

A small background thread inside `explorer.exe` checks the open windows every 50 ms. Minimized, cloaked (other virtual desktop), tool and click-through windows are ignored, as are the taskbar and the desktop itself. The desktop icons view is hidden or shown accordingly.

## Notes

- While the icons are hidden, the desktop right-click menu is unavailable too.
- Invisible overlay windows from other programs may count as an open window. Check the mod's log output in Windhawk to see which window class triggered the change, and open an issue with it.

## License

MIT, see [LICENSE](LICENSE).

## Credits

[aguirre-technsci](https://github.com/aguirre-technsci), co-authored with Claude (Anthropic).

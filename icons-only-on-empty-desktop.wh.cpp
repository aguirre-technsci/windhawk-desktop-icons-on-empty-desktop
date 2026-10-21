// ==WindhawkMod==
// @id              icons-only-on-empty-desktop
// @name            Desktop icons only on empty desktop
// @description     Desktop icons are shown only when all windows are minimized
// @version         1.2
// @author          aguirre-technsci, Claude (Anthropic)
// @github          https://github.com/aguirre-technsci
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -ldwmapi
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Desktop icons only on empty desktop
If every window is minimized, the desktop icons are shown. If any window is
visible (windowed, maximized or fullscreen), the icons are hidden.
Icons are restored when the mod is unloaded.

Author: [aguirre-technsci](https://github.com/aguirre-technsci), co-authored
with Claude (Anthropic).

Notes:
- While the icons are hidden, the desktop's right-click menu is unavailable too
  (the whole icons view is hidden, same as other desktop-icon tools).
- Invisible overlay windows from other programs may count as an open window.
  Check the log output of the mod to see when the state changes.
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_CLOAKED
#define DWMWA_CLOAKED 14
#endif

// Plain HANDLEs have no destructor, so nothing runs at process exit; the
// cleanup is done explicitly in Wh_ModUninit.
HANDLE g_stop;
HANDLE g_thread;
WCHAR g_lastClass[64];  // class of the window that hid the icons, for the log

// The icons live in a SysListView32 under Progman or (after Win+D / wallpaper
// slideshow) under a WorkerW window.
static HWND FindIconsView() {
    HWND def = FindWindowExW(FindWindowW(L"Progman", nullptr), nullptr,
                             L"SHELLDLL_DefView", nullptr);
    for (HWND w = nullptr; !def && (w = FindWindowExW(nullptr, w, L"WorkerW",
                                                       nullptr));) {
        def = FindWindowExW(w, nullptr, L"SHELLDLL_DefView", nullptr);
    }
    return def ? FindWindowExW(def, nullptr, L"SysListView32", nullptr)
               : nullptr;
}

// Sets *lp = true when h is a real, on-screen app window.
// shortcut: owned windows are ignored, so a lone dialog whose owner is
// minimized does not hide the icons; revisit if that matters.
static BOOL CALLBACK EnumProc(HWND h, LPARAM lp) {
    if (!IsWindowVisible(h) || IsIconic(h)) return TRUE;

    LONG ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((ex & (WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT)) ||
        GetWindow(h, GW_OWNER)) {
        return TRUE;
    }
    if ((ex & WS_EX_NOACTIVATE) && !(ex & WS_EX_APPWINDOW)) return TRUE;

    // Cloaked = other virtual desktop or suspended UWP app.
    DWORD cloaked = 0;
    DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked) return TRUE;

    WCHAR cls[64];
    GetClassNameW(h, cls, 64);
    static const WCHAR* ignore[] = {L"Progman", L"WorkerW", L"Shell_TrayWnd",
                                    L"Shell_SecondaryTrayWnd",
                                    L"Windows.UI.Core.CoreWindow"};
    for (auto c : ignore) {
        if (!wcscmp(cls, c)) return TRUE;
    }

    wcscpy(g_lastClass, cls);
    *(bool*)lp = true;
    return FALSE;
}

// Only the explorer.exe process that owns the desktop acts.
static HWND OwnIconsView() {
    HWND icons = FindIconsView();
    DWORD pid = 0;
    if (icons) GetWindowThreadProcessId(icons, &pid);
    return pid == GetCurrentProcessId() ? icons : nullptr;
}

static DWORD WINAPI Loop(void*) {
    do {
        if (HWND icons = OwnIconsView()) {
            bool anyWindow = false;
            EnumWindows(EnumProc, (LPARAM)&anyWindow);
            // Visible but should hide, or hidden but should show.
            if ((IsWindowVisible(icons) != 0) == anyWindow) {
                Wh_Log(L"%ls icons (window class: %ls)", anyWindow ? L"Hide" : L"Show",
                       anyWindow ? g_lastClass : L"-");
                // Async: never block this thread on the desktop thread.
                ShowWindowAsync(icons, anyWindow ? SW_HIDE : SW_SHOW);
            }
        }
    } while (WaitForSingleObject(g_stop, 50) == WAIT_TIMEOUT);
    return 0;
}

BOOL Wh_ModInit() {
    g_stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stop) return FALSE;

    g_thread = CreateThread(nullptr, 0, Loop, nullptr, 0, nullptr);
    if (!g_thread) {
        CloseHandle(g_stop);
        return FALSE;
    }
    return TRUE;
}

void Wh_ModUninit() {
    // The thread must be gone before the mod DLL is unloaded.
    SetEvent(g_stop);
    WaitForSingleObject(g_thread, INFINITE);
    CloseHandle(g_thread);
    CloseHandle(g_stop);

    // Leave the desktop the way we found it.
    if (HWND icons = OwnIconsView()) ShowWindowAsync(icons, SW_SHOW);
}

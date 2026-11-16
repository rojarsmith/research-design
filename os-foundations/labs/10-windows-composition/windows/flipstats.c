/*
 * flipstats.c - Stage 10 companion: a small Direct3D 11 program with a
 * flip-model swap chain, and a log of what the documented APIs that need
 * no extra tools and no administrator rights say about how its frames
 * reached the screen.
 *
 * Build:  gcc -O2 -Wall -o flipstats.exe flipstats.c -ld3d11 -ldxgi -ldwmapi -luser32 -lgdi32 -ladvapi32
 *
 * Run:    flipstats.exe info                      the machine: Windows build, session, DXGI
 *                                                 adapters and outputs, display paths, DWM
 *                                                 timing; no window
 *         flipstats.exe windowed   [s] [options]  a 320x240 window near the bottom-right
 *                                                 corner, s seconds (default 4, at most 30)
 *         flipstats.exe overlap    [s] [options]  the same window with a second, opaque
 *                                                 window of this program over part of it
 *         flipstats.exe borderless [s] [options]  a window that covers the whole primary
 *                                                 monitor and takes the foreground; it
 *                                                 hides everything else on that monitor for
 *                                                 s seconds (Esc ends it early)
 * Options: --vsync 1     Present(1, 0): wait for one vblank (default)
 *          --vsync 0     Present(0, DXGI_PRESENT_ALLOW_TEARING) when the system supports
 *                        tearing, otherwise Present(0, 0)
 *          --sequential  DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL instead of FLIP_DISCARD
 *          --buffers N   swap-chain buffer count, 2..16 (default 2)
 *          --latency N   IDXGIDevice1::SetMaximumFrameLatency(N), 1..16
 *
 * Safety: the windowed and overlapped windows are at most 320x240 and 160x120,
 * are created with WS_EX_NOACTIVATE and shown with SW_SHOWNOACTIVATE (they do
 * not take the focus or the foreground), are not topmost, and are destroyed
 * when the run ends. A watchdog thread ends the process 5 s after the run
 * should have finished, whatever happens.
 *
 * What it logs, and where each number comes from:
 *   QueryPerformanceCounter   around every Present call: when the app submitted
 *                             and how long Present blocked
 *   GetLastPresentCount       the id DXGI gave each Present
 *   GetFrameStatistics        per change: the newest present that DXGI says was
 *                             shown, and the vblank count at which it was shown
 *   GetFrameStatisticsMedia   CompositionMode (COMPOSED / OVERLAY / NONE), an
 *                             interface documented for media swap chains
 *   DwmGetCompositionTimingInfo(NULL)  DWM's refresh rate and its counters of
 *                             refreshes and composed frames, sampled every 100 ms
 *   IDXGIOutput::WaitForVBlank  the output's vblank interval, measured before the run
 * None of these is a trace of the presentation path. The README says what each
 * one can and cannot prove, and how PresentMon answers the rest.
 */
#define _WIN32_WINNT 0x0A00
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <dwmapi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

#define MAX_SECONDS 30.0
#define WIN_W 320
#define WIN_H 240

static double qpf;
static int64_t qpc(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}
static double ms(int64_t ticks) { return (double)ticks * 1e3 / qpf; }

static void die(const char *what, HRESULT hr)
{
    printf("%s failed: 0x%08lx\n", what, (unsigned long)hr);
    exit(1);
}

static const char *u8(const WCHAR *w)
{
    static char buf[4][512];
    static int i;
    char *b = buf[i++ & 3];
    WideCharToMultiByte(CP_UTF8, 0, w, -1, b, sizeof buf[0], NULL, NULL);
    return b;
}

static const char *hrname(HRESULT hr)
{
    static char buf[4][32];
    static int i;
    switch (hr) {
    case S_OK: return "S_OK";
    case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
    case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
    case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED";
    case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND";
    case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
    case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
    case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
    case DXGI_STATUS_OCCLUDED: return "DXGI_STATUS_OCCLUDED";
    case DXGI_STATUS_MODE_CHANGED: return "DXGI_STATUS_MODE_CHANGED";
    case E_INVALIDARG: return "E_INVALIDARG";
    case E_NOINTERFACE: return "E_NOINTERFACE";
    case E_NOTIMPL: return "E_NOTIMPL";
    case E_FAIL: return "E_FAIL";
    }
    char *b = buf[i++ & 3];
    snprintf(b, sizeof buf[0], "0x%08lx", (unsigned long)hr);
    return b;
}

/* ---- growable arrays ---------------------------------------------------- */

#define PUSH(arr, n, cap, val)                                                                     \
    do {                                                                                           \
        if ((n) == (cap)) {                                                                        \
            (cap) = (cap) ? (cap) * 2 : 1024;                                                      \
            (arr) = realloc((arr), (size_t)(cap) * sizeof *(arr));                                 \
            if (!(arr)) {                                                                          \
                printf("out of memory\n");                                                         \
                exit(1);                                                                           \
            }                                                                                      \
        }                                                                                          \
        (arr)[(n)++] = (val);                                                                      \
    } while (0)

typedef struct { int64_t t0, t1; UINT pc; } frame_t;           /* one Present call */
typedef struct { int64_t seen; DXGI_FRAME_STATISTICS s; } stat_t; /* one change of the stats */
typedef struct { int64_t seen; UINT pc; int mode; UINT approved; } media_t;
typedef struct { int64_t seen; uint64_t cframe, crefresh; } dwm_t;

/* ---- system information ------------------------------------------------- */

static void print_windows_version(void)
{
    typedef LONG(WINAPI * rtlgetversion_t)(PRTL_OSVERSIONINFOW);
    RTL_OSVERSIONINFOW v = {.dwOSVersionInfoSize = sizeof v};
    rtlgetversion_t f = (rtlgetversion_t)(void (*)(void))GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
    DWORD ubr = 0, sz = sizeof ubr;
    RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"UBR",
                 RRF_RT_REG_DWORD, NULL, &ubr, &sz);
    if (f && f(&v) == 0)
        printf("Windows %lu.%lu build %lu.%lu (RtlGetVersion, UBR from the registry)\n",
               v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber, ubr);
}

static void print_session(void)
{
    DWORD sid = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &sid);
    DWORD console = WTSGetActiveConsoleSessionId();
    BOOL comp = FALSE;
    DwmIsCompositionEnabled(&comp);
    printf("session: this process is in session %lu; the console session is %lu; "
           "GetSystemMetrics(SM_REMOTESESSION) = %d%s\n",
           sid, console, GetSystemMetrics(SM_REMOTESESSION),
           GetSystemMetrics(SM_REMOTESESSION) ? " (a Remote Desktop session)" : "");
    printf("DwmIsCompositionEnabled: %s; QueryPerformanceFrequency: %.0f Hz\n",
           comp ? "TRUE" : "FALSE", qpf);
}

static const char *tech_name(DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY t)
{
    switch ((int)t) {
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HD15: return "VGA";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI: return "DVI";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI: return "HDMI";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_LVDS: return "LVDS";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL: return "DisplayPort (external)";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED: return "DisplayPort (embedded)";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_MIRACAST: return "Miracast";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED: return "indirect, wired";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_VIRTUAL: return "indirect, virtual";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_USB_TUNNEL: return "DisplayPort over USB";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL: return "internal";
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_OTHER: return "other";
    }
    return "?";
}

/* The active display paths, as QueryDisplayConfig reports them. If only is
 * not NULL, print only the path whose source is that GDI device name. */
static void print_display_paths(const WCHAR *only)
{
    UINT32 np = 0, nm = 0;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &np, &nm) != ERROR_SUCCESS) {
        printf("  GetDisplayConfigBufferSizes failed\n");
        return;
    }
    DISPLAYCONFIG_PATH_INFO *p = calloc(np, sizeof *p);
    DISPLAYCONFIG_MODE_INFO *m = calloc(nm, sizeof *m);
    LONG r = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &np, p, &nm, m, NULL);
    if (r != ERROR_SUCCESS) {
        printf("  QueryDisplayConfig failed: %ld\n", r);
        goto out;
    }
    for (UINT32 i = 0; i < np; i++) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME sn = {0};
        sn.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        sn.header.size = sizeof sn;
        sn.header.adapterId = p[i].sourceInfo.adapterId;
        sn.header.id = p[i].sourceInfo.id;
        DisplayConfigGetDeviceInfo(&sn.header);
        if (only && wcscmp(sn.viewGdiDeviceName, only) != 0)
            continue;
        DISPLAYCONFIG_TARGET_DEVICE_NAME tn = {0};
        tn.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        tn.header.size = sizeof tn;
        tn.header.adapterId = p[i].targetInfo.adapterId;
        tn.header.id = p[i].targetInfo.id;
        DisplayConfigGetDeviceInfo(&tn.header);
        DISPLAYCONFIG_RATIONAL rr = p[i].targetInfo.refreshRate;
        printf("  path %u: adapter LUID %08lx:%08lx, source %s -> target \"%s\", %s; path refresh "
               "%u/%u = %.3f Hz",
               i, (unsigned long)p[i].sourceInfo.adapterId.HighPart,
               p[i].sourceInfo.adapterId.LowPart, u8(sn.viewGdiDeviceName),
               tn.monitorFriendlyDeviceName[0] ? u8(tn.monitorFriendlyDeviceName) : "(no name)",
               tech_name(tn.outputTechnology), rr.Numerator, rr.Denominator,
               rr.Denominator ? (double)rr.Numerator / rr.Denominator : 0.0);
        UINT32 ti = p[i].targetInfo.modeInfoIdx;
        if (ti != DISPLAYCONFIG_PATH_MODE_IDX_INVALID && ti < nm &&
            m[ti].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET) {
            DISPLAYCONFIG_VIDEO_SIGNAL_INFO *s = &m[ti].targetMode.targetVideoSignalInfo;
            printf("; signal %ux%u active, vsync %u/%u = %.3f Hz",
                   s->activeSize.cx, s->activeSize.cy, s->vSyncFreq.Numerator,
                   s->vSyncFreq.Denominator,
                   s->vSyncFreq.Denominator
                       ? (double)s->vSyncFreq.Numerator / s->vSyncFreq.Denominator
                       : 0.0);
        }
        printf("\n");
    }
out:
    free(p);
    free(m);
}

static void print_output(IDXGIOutput *out, const char *indent)
{
    DXGI_OUTPUT_DESC d;
    IDXGIOutput_GetDesc(out, &d);
    RECT r = d.DesktopCoordinates;
    DEVMODEW dm = {.dmSize = sizeof dm};
    EnumDisplaySettingsW(d.DeviceName, ENUM_CURRENT_SETTINGS, &dm);
    printf("%soutput %s: desktop (%ld,%ld)-(%ld,%ld), attached %s; EnumDisplaySettings: %lux%lu, "
           "%lu Hz, %lu bpp\n",
           indent, u8(d.DeviceName), r.left, r.top, r.right, r.bottom,
           d.AttachedToDesktop ? "yes" : "no", dm.dmPelsWidth, dm.dmPelsHeight,
           dm.dmDisplayFrequency, dm.dmBitsPerPel);
    IDXGIOutput2 *o2 = NULL;
    if (SUCCEEDED(IDXGIOutput_QueryInterface(out, &IID_IDXGIOutput2, (void **)&o2))) {
        printf("%s  IDXGIOutput2::SupportsOverlays: %s\n", indent,
               IDXGIOutput2_SupportsOverlays(o2) ? "TRUE" : "FALSE");
        IDXGIOutput2_Release(o2);
    }
    IDXGIOutput6 *o6 = NULL;
    if (SUCCEEDED(IDXGIOutput_QueryInterface(out, &IID_IDXGIOutput6, (void **)&o6))) {
        UINT f = 0;
        HRESULT hr = IDXGIOutput6_CheckHardwareCompositionSupport(o6, &f);
        printf("%s  IDXGIOutput6::CheckHardwareCompositionSupport: %s, flags 0x%x (%s%s%s)\n",
               indent, hrname(hr), f,
               (f & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_FULLSCREEN) ? "FULLSCREEN " : "",
               (f & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_WINDOWED) ? "WINDOWED " : "",
               (f & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_CURSOR_STRETCHED) ? "CURSOR_STRETCHED"
                                                                              : "");
        DXGI_OUTPUT_DESC1 d1;
        if (SUCCEEDED(IDXGIOutput6_GetDesc1(o6, &d1)))
            printf("%s  IDXGIOutput6::GetDesc1: %u bits per color, color space %d%s\n", indent,
                   d1.BitsPerColor, (int)d1.ColorSpace,
                   d1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ? " (HDR10)"
                   : d1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709  ? " (sRGB)"
                                                                               : "");
        IDXGIOutput6_Release(o6);
    }
}

static void print_adapter(IDXGIAdapter1 *a, UINT index)
{
    DXGI_ADAPTER_DESC1 d;
    IDXGIAdapter1_GetDesc1(a, &d);
    LARGE_INTEGER umd = {0};
    HRESULT hr = IDXGIAdapter1_CheckInterfaceSupport(a, &IID_IDXGIDevice, &umd);
    printf("adapter %u: \"%s\", LUID %08lx:%08lx, vendor 0x%04x device 0x%04x, %llu MB dedicated "
           "video memory%s\n",
           index, u8(d.Description), (unsigned long)d.AdapterLuid.HighPart,
           d.AdapterLuid.LowPart, d.VendorId, d.DeviceId,
           (unsigned long long)(d.DedicatedVideoMemory >> 20),
           (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? ", SOFTWARE"
           : (d.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? ", REMOTE"
                                                  : "");
    if (SUCCEEDED(hr))
        printf("  user-mode driver version (CheckInterfaceSupport): %u.%u.%u.%u\n",
               HIWORD(umd.HighPart), LOWORD(umd.HighPart), HIWORD(umd.LowPart),
               LOWORD(umd.LowPart));
    else
        printf("  user-mode driver version: CheckInterfaceSupport returned %s\n", hrname(hr));
    IDXGIOutput *o;
    UINT n = 0;
    for (UINT j = 0; IDXGIAdapter1_EnumOutputs(a, j, &o) != DXGI_ERROR_NOT_FOUND; j++, n++) {
        print_output(o, "  ");
        IDXGIOutput_Release(o);
    }
    if (n == 0)
        printf("  (no outputs: this adapter drives no display in this session)\n");
}

static void print_dwm(const char *label)
{
    DWM_TIMING_INFO t = {.cbSize = sizeof t};
    HRESULT hr = DwmGetCompositionTimingInfo(NULL, &t);
    if (FAILED(hr)) {
        printf("%sDwmGetCompositionTimingInfo(NULL): %s\n", label, hrname(hr));
        return;
    }
    printf("%sDwmGetCompositionTimingInfo(NULL): rateRefresh %u/%u = %.3f Hz, qpcRefreshPeriod "
           "%.3f ms, rateCompose %u/%u, cRefresh %llu, cFrame %llu, cFramesDisplayed %llu, "
           "cFramesDropped %llu, cFramesMissed %llu\n",
           label, t.rateRefresh.uiNumerator, t.rateRefresh.uiDenominator,
           t.rateRefresh.uiDenominator
               ? (double)t.rateRefresh.uiNumerator / t.rateRefresh.uiDenominator
               : 0.0,
           ms((int64_t)t.qpcRefreshPeriod), t.rateCompose.uiNumerator,
           t.rateCompose.uiDenominator, (unsigned long long)t.cRefresh,
           (unsigned long long)t.cFrame, (unsigned long long)t.cFramesDisplayed,
           (unsigned long long)t.cFramesDropped, (unsigned long long)t.cFramesMissed);
}

static int tearing_supported(IDXGIFactory1 *f1)
{
    IDXGIFactory5 *f5 = NULL;
    BOOL allow = FALSE;
    if (SUCCEEDED(IDXGIFactory1_QueryInterface(f1, &IID_IDXGIFactory5, (void **)&f5))) {
        if (FAILED(IDXGIFactory5_CheckFeatureSupport(f5, DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                     &allow, sizeof allow)))
            allow = FALSE;
        IDXGIFactory5_Release(f5);
    }
    return allow;
}

static void print_header(void)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    printf("date: %04u-%02u-%02u %02u:%02u:%02u UTC\n", st.wYear, st.wMonth, st.wDay, st.wHour,
           st.wMinute, st.wSecond);
    print_windows_version();
    print_session();
}

static int info_main(void)
{
    print_header();
    IDXGIFactory1 *f1;
    HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&f1);
    if (FAILED(hr))
        die("CreateDXGIFactory1", hr);
    printf("IDXGIFactory5::CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING): %s\n\n",
           tearing_supported(f1) ? "TRUE" : "FALSE");
    IDXGIAdapter1 *a;
    for (UINT i = 0; IDXGIFactory1_EnumAdapters1(f1, i, &a) != DXGI_ERROR_NOT_FOUND; i++) {
        print_adapter(a, i);
        IDXGIAdapter1_Release(a);
    }
    printf("\nQueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS):\n");
    print_display_paths(NULL);
    printf("\n");
    print_dwm("");
    /* A baseline: how fast DWM's counters move while this program shows nothing. */
    DWM_TIMING_INFO t0 = {.cbSize = sizeof t0}, t1 = {.cbSize = sizeof t1};
    if (SUCCEEDED(DwmGetCompositionTimingInfo(NULL, &t0))) {
        int64_t a = qpc();
        Sleep(2000);
        if (SUCCEEDED(DwmGetCompositionTimingInfo(NULL, &t1)))
            printf("over %.2f s with no window of this program: cRefresh +%llu, cFrame +%llu\n",
                   ms(qpc() - a) / 1e3, (unsigned long long)(t1.cRefresh - t0.cRefresh),
                   (unsigned long long)(t1.cFrame - t0.cFrame));
    }
    IDXGIFactory1_Release(f1);
    return 0;
}

/* ---- windows ------------------------------------------------------------ */

static volatile LONG g_quit;

static LRESULT CALLBACK wndproc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    switch (m) {
    case WM_MOUSEACTIVATE:
        if (GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_NOACTIVATE)
            return MA_NOACTIVATE;
        break;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE)
            g_quit = 1;
        return 0;
    case WM_CLOSE:
        g_quit = 1; /* the run loop ends and destroys the windows */
        return 0;
    }
    return DefWindowProcW(w, m, wp, lp);
}

static DWORD WINAPI watchdog(LPVOID arg)
{
    DWORD limit_ms = (DWORD)(uintptr_t)arg;
    Sleep(limit_ms);
    printf("watchdog: still running after %lu ms; ending the process\n", limit_ms);
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), 3);
    return 0;
}

/* Count visible, uncloaked windows above w in the z-order whose frames
 * intersect rect (screen coordinates). Only counts; no titles or classes. */
static void windows_above(HWND w, RECT rect, HWND own, int *own_n, int *other_n)
{
    *own_n = *other_n = 0;
    for (HWND h = GetWindow(w, GW_HWNDPREV); h; h = GetWindow(h, GW_HWNDPREV)) {
        if (!IsWindowVisible(h) || IsIconic(h))
            continue;
        BOOL cloaked = FALSE;
        DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloaked, sizeof cloaked);
        if (cloaked)
            continue;
        RECT r, i;
        if (FAILED(DwmGetWindowAttribute(h, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof r)))
            GetWindowRect(h, &r);
        if (IntersectRect(&i, &r, &rect)) {
            if (h == own)
                (*own_n)++;
            else
                (*other_n)++;
        }
    }
}

/* ---- statistics helpers ------------------------------------------------- */

static int cmpd(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void print_dist(const char *label, double *v, int n, const char *unit)
{
    if (n == 0) {
        printf("  %s: no values\n", label);
        return;
    }
    qsort(v, (size_t)n, sizeof *v, cmpd);
    printf("  %s (%d values): min %.3f, median %.3f, p90 %.3f, p99 %.3f, max %.3f %s\n", label,
           n, v[0], v[n / 2], v[(n * 90) / 100], v[(n * 99) / 100], v[n - 1], unit);
}

static const frame_t *find_frame(const frame_t *f, size_t n, UINT pc)
{
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (f[mid].pc < pc)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo < n && f[lo].pc == pc ? &f[lo] : NULL;
}

static void hue(unsigned i, float c[4])
{
    float h = (float)(i % 360) / 60.0f, x = 1.0f - fabsf(fmodf(h, 2.0f) - 1.0f);
    float r = 0, g = 0, b = 0;
    switch ((int)h) {
    case 0: r = 1, g = x; break;
    case 1: r = x, g = 1; break;
    case 2: g = 1, b = x; break;
    case 3: g = x, b = 1; break;
    case 4: r = x, b = 1; break;
    default: r = 1, b = x; break;
    }
    c[0] = 0.15f + 0.6f * r, c[1] = 0.15f + 0.6f * g, c[2] = 0.15f + 0.6f * b, c[3] = 1.0f;
}

/* ---- the run ------------------------------------------------------------ */

enum { MODE_WINDOWED, MODE_OVERLAP, MODE_BORDERLESS };
static const char *mode_names[] = {"windowed", "overlap", "borderless"};

struct opts {
    int mode, vsync, sequential, buffers, latency;
    double seconds;
};

static int run_main(const struct opts *o)
{
    print_header();
    printf("configuration: %s, %.1f s, vsync %d, %s, %d buffers%s\n", mode_names[o->mode],
           o->seconds, o->vsync,
           o->sequential ? "DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL" : "DXGI_SWAP_EFFECT_FLIP_DISCARD",
           o->buffers, o->latency ? "" : ", default frame latency");

    IDXGIFactory1 *f1;
    IDXGIFactory2 *f2;
    HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&f1);
    if (FAILED(hr))
        die("CreateDXGIFactory1", hr);
    hr = IDXGIFactory1_QueryInterface(f1, &IID_IDXGIFactory2, (void **)&f2);
    if (FAILED(hr))
        die("QueryInterface(IDXGIFactory2)", hr);
    int tearing = tearing_supported(f1);
    printf("tearing support (DXGI_FEATURE_PRESENT_ALLOW_TEARING): %s\n", tearing ? "TRUE" : "FALSE");

    /* Where the window goes, and which monitor that is. */
    RECT wr;
    HMONITOR mon;
    if (o->mode == MODE_BORDERLESS) {
        POINT origin = {0, 0};
        mon = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = {.cbSize = sizeof mi};
        GetMonitorInfoW(mon, &mi);
        wr = mi.rcMonitor;
    } else {
        RECT work;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        RECT c = {0, 0, WIN_W, WIN_H};
        AdjustWindowRectEx(&c, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_NOACTIVATE);
        int w = c.right - c.left, h = c.bottom - c.top;
        wr.right = work.right - 24;
        wr.bottom = work.bottom - 24;
        wr.left = wr.right - w;
        wr.top = wr.bottom - h;
        POINT centre = {(wr.left + wr.right) / 2, (wr.top + wr.bottom) / 2};
        mon = MonitorFromPoint(centre, MONITOR_DEFAULTTOPRIMARY);
    }

    /* The DXGI adapter and output that own that monitor. */
    IDXGIAdapter1 *adapter = NULL, *a;
    IDXGIOutput *output = NULL;
    for (UINT i = 0; !output && IDXGIFactory1_EnumAdapters1(f1, i, &a) != DXGI_ERROR_NOT_FOUND;
         i++) {
        IDXGIOutput *out;
        for (UINT j = 0; IDXGIAdapter1_EnumOutputs(a, j, &out) != DXGI_ERROR_NOT_FOUND; j++) {
            DXGI_OUTPUT_DESC d;
            IDXGIOutput_GetDesc(out, &d);
            if (d.Monitor == mon) {
                output = out;
                adapter = a;
                break;
            }
            IDXGIOutput_Release(out);
        }
        if (!output)
            IDXGIAdapter1_Release(a);
    }
    if (!output) {
        printf("no DXGI output owns the window's monitor; using the default adapter\n");
    } else {
        DXGI_ADAPTER_DESC1 ad;
        IDXGIAdapter1_GetDesc1(adapter, &ad);
        printf("adapter for this monitor: \"%s\", LUID %08lx:%08lx\n", u8(ad.Description),
               (unsigned long)ad.AdapterLuid.HighPart, ad.AdapterLuid.LowPart);
        print_output(output, "");
        DXGI_OUTPUT_DESC od;
        IDXGIOutput_GetDesc(output, &od);
        print_display_paths(od.DeviceName);

        /* The output's vblank interval, measured before any window exists:
         * up to 61 WaitForVBlank calls or 1 s, whichever comes first. */
        double iv[64];
        int n = 0;
        HRESULT hv = S_OK;
        int64_t prev = 0, begin = qpc();
        for (int k = 0; k < 61 && qpc() - begin < (int64_t)qpf; k++) {
            hv = IDXGIOutput_WaitForVBlank(output);
            int64_t t = qpc();
            if (FAILED(hv))
                break;
            if (k > 0)
                iv[n++] = ms(t - prev);
            prev = t;
        }
        if (FAILED(hv))
            printf("IDXGIOutput::WaitForVBlank: %s\n", hrname(hv));
        else if (n > 0) {
            double sum = 0;
            for (int k = 0; k < n; k++)
                sum += iv[k];
            printf("IDXGIOutput::WaitForVBlank: %d intervals, mean %.3f ms (%.2f Hz)", n, sum / n,
                   1000.0 * n / sum);
            qsort(iv, (size_t)n, sizeof iv[0], cmpd);
            printf(", min %.3f, median %.3f, max %.3f ms\n", iv[0], iv[n / 2], iv[n - 1]);
        }
    }
    print_dwm("before: ");

    /* Device and swap chain. */
    ID3D11Device *dev;
    ID3D11DeviceContext *ctx;
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                                  D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL got;
    hr = D3D11CreateDevice((IDXGIAdapter *)adapter,
                           adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, NULL,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, 4, D3D11_SDK_VERSION, &dev,
                           &got, &ctx);
    if (FAILED(hr))
        die("D3D11CreateDevice", hr);
    IDXGIDevice1 *dxdev;
    UINT maxlat = 0;
    if (SUCCEEDED(ID3D11Device_QueryInterface(dev, &IID_IDXGIDevice1, (void **)&dxdev))) {
        if (o->latency)
            IDXGIDevice1_SetMaximumFrameLatency(dxdev, (UINT)o->latency);
        IDXGIDevice1_GetMaximumFrameLatency(dxdev, &maxlat);
        IDXGIDevice1_Release(dxdev);
    }
    printf("D3D11 device: feature level %u_%u, maximum frame latency %u\n", (got >> 12) & 0xf,
           (got >> 8) & 0xf, maxlat);

    WNDCLASSW wc = {.lpfnWndProc = wndproc,
                    .hInstance = GetModuleHandleW(NULL),
                    .hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW),
                    .lpszClassName = L"lab10-flipstats"};
    RegisterClassW(&wc);
    WNDCLASSW wc2 = wc;
    wc2.lpszClassName = L"lab10-overlap";
    wc2.hbrBackground = CreateSolidBrush(RGB(48, 48, 48));
    RegisterClassW(&wc2);

    DWORD style, exstyle;
    if (o->mode == MODE_BORDERLESS) {
        style = WS_POPUP;
        exstyle = 0;
    } else {
        style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
        exstyle = WS_EX_NOACTIVATE; /* never the foreground window; no taskbar button */
    }
    HWND hwnd = CreateWindowExW(exstyle, wc.lpszClassName, L"lab 10 flipstats", style, wr.left,
                                wr.top, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL,
                                wc.hInstance, NULL);
    if (!hwnd)
        die("CreateWindowEx", HRESULT_FROM_WIN32(GetLastError()));
    RECT cr;
    GetClientRect(hwnd, &cr);
    UINT cw = (UINT)cr.right, ch = (UINT)cr.bottom;
    HWND over = NULL;
    if (o->mode == MODE_OVERLAP) {
        /* An owned popup always stays above its owner in the z-order. It
         * covers the lower-right part of the client area and sticks out. */
        POINT p = {(LONG)cw - 120, (LONG)ch - 90};
        ClientToScreen(hwnd, &p);
        over = CreateWindowExW(WS_EX_NOACTIVATE, wc2.lpszClassName, L"lab 10 overlap",
                               WS_POPUP | WS_BORDER, p.x, p.y, 160, 120, hwnd, NULL, wc.hInstance,
                               NULL);
    }
    if (o->mode == MODE_BORDERLESS) {
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
    } else {
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        if (over)
            ShowWindow(over, SW_SHOWNOACTIVATE);
    }

    DXGI_SWAP_CHAIN_DESC1 sd = {
        .Width = cw,
        .Height = ch,
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .SampleDesc = {1, 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = (UINT)o->buffers,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = o->sequential ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL : DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
        .Flags = tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0,
    };
    IDXGISwapChain1 *sc;
    hr = IDXGIFactory2_CreateSwapChainForHwnd(f2, (IUnknown *)dev, hwnd, &sd, NULL, NULL, &sc);
    if (FAILED(hr))
        die("CreateSwapChainForHwnd", hr);
    IDXGIFactory2_MakeWindowAssociation(f2, hwnd, DXGI_MWA_NO_ALT_ENTER);
    IDXGISwapChainMedia *scm = NULL;
    HRESULT hqm = IDXGISwapChain1_QueryInterface(sc, &IID_IDXGISwapChainMedia, (void **)&scm);
    ID3D11Texture2D *bb;
    ID3D11RenderTargetView *rtv;
    IDXGISwapChain1_GetBuffer(sc, 0, &IID_ID3D11Texture2D, (void **)&bb);
    hr = ID3D11Device_CreateRenderTargetView(dev, (ID3D11Resource *)bb, NULL, &rtv);
    if (FAILED(hr))
        die("CreateRenderTargetView", hr);
    ID3D11DeviceContext1 *ctx1 = NULL;
    ID3D11DeviceContext_QueryInterface(ctx, &IID_ID3D11DeviceContext1, (void **)&ctx1);

    RECT client_screen = cr;
    MapWindowPoints(hwnd, NULL, (POINT *)&client_screen, 2);
    printf("window: client %ux%u at (%ld,%ld), DPI %u, %s, %s%s\n", cw, ch, client_screen.left,
           client_screen.top, GetDpiForWindow(hwnd),
           o->mode == MODE_BORDERLESS ? "WS_POPUP covering the monitor" : "WS_EX_NOACTIVATE",
           sd.Flags ? "swap chain flag ALLOW_TEARING" : "no swap chain flags",
           over ? "; overlapping window 160x120 owned by it" : "");
    IDXGIOutput *contain = NULL;
    if (SUCCEEDED(IDXGISwapChain1_GetContainingOutput(sc, &contain))) {
        DXGI_OUTPUT_DESC d;
        IDXGIOutput_GetDesc(contain, &d);
        printf("IDXGISwapChain::GetContainingOutput: %s\n", u8(d.DeviceName));
        IDXGIOutput3 *o3;
        if (SUCCEEDED(IDXGIOutput_QueryInterface(contain, &IID_IDXGIOutput3, (void **)&o3))) {
            UINT fl = 0;
            HRESULT hc = IDXGIOutput3_CheckOverlaySupport(o3, DXGI_FORMAT_B8G8R8A8_UNORM,
                                                          (IUnknown *)dev, &fl);
            printf("IDXGIOutput3::CheckOverlaySupport(B8G8R8A8_UNORM): %s, flags 0x%x%s%s\n",
                   hrname(hc), fl, (fl & DXGI_OVERLAY_SUPPORT_FLAG_DIRECT) ? " DIRECT" : "",
                   (fl & DXGI_OVERLAY_SUPPORT_FLAG_SCALING) ? " SCALING" : "");
            IDXGIOutput3_Release(o3);
        }
        IDXGIOutput_Release(contain);
    }
    printf("IDXGISwapChainMedia: %s\n", SUCCEEDED(hqm) ? "available" : hrname(hqm));

    UINT sync = o->vsync ? 1 : 0;
    UINT pflags = (!o->vsync && tearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    printf("Present(%u, %s)\n\n", sync, pflags ? "DXGI_PRESENT_ALLOW_TEARING" : "0");

    /* The loop. */
    frame_t *fr = NULL;
    size_t nfr = 0, cfr = 0;
    stat_t *stv = NULL;
    size_t nst = 0, cst = 0;
    media_t *mdv = NULL;
    size_t nmd = 0, cmd = 0;
    dwm_t *dwv = NULL;
    size_t ndw = 0, cdw = 0;
    HRESULT serr[4] = {0}, merr[4] = {0}, perr = S_OK;
    unsigned long serrn[4] = {0}, merrn[4] = {0}, stat_ok = 0, media_ok = 0, occluded = 0;
    int own_above = -1, other_above = -1, foreground = -1;
    float white[4] = {1, 1, 1, 1};

    int64_t start = qpc(), end = start + (int64_t)(o->seconds * qpf), next_dwm = start;
    int64_t drain_end = 0;
    for (unsigned f = 0;; f++) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        int64_t now = qpc();
        int presenting = !g_quit && now < end;
        if (!presenting && !drain_end)
            drain_end = now + (int64_t)(0.25 * qpf); /* keep reading statistics for 250 ms */
        if (!presenting && now >= drain_end)
            break;
        if (own_above < 0 && now - start > (int64_t)(0.3 * qpf)) {
            windows_above(hwnd, client_screen, over, &own_above, &other_above);
            foreground = GetForegroundWindow() == hwnd;
        }
        if (presenting) {
            float c[4];
            hue(f, c);
            ID3D11DeviceContext_ClearRenderTargetView(ctx, rtv, c);
            if (ctx1) { /* a white bar that moves 4 px per frame shows tearing */
                LONG x = (LONG)((f * 4) % (cw > 16 ? cw - 16 : 1));
                D3D11_RECT r = {x, 0, x + 16, (LONG)ch};
                ID3D11DeviceContext1_ClearView(ctx1, (ID3D11View *)rtv, white, &r, 1);
            }
            frame_t rec;
            rec.t0 = qpc();
            hr = IDXGISwapChain1_Present(sc, sync, pflags);
            rec.t1 = qpc();
            if (hr == DXGI_STATUS_OCCLUDED)
                occluded++;
            else if (FAILED(hr)) {
                perr = hr;
                printf("Present returned %s", hrname(hr));
                if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
                    printf("; GetDeviceRemovedReason: %s",
                           hrname(ID3D11Device_GetDeviceRemovedReason(dev)));
                printf("\n");
                g_quit = 1;
                continue;
            }
            IDXGISwapChain1_GetLastPresentCount(sc, &rec.pc);
            PUSH(fr, nfr, cfr, rec);
        } else {
            Sleep(1);
        }

        DXGI_FRAME_STATISTICS s;
        HRESULT hs = IDXGISwapChain1_GetFrameStatistics(sc, &s);
        if (hs == S_OK) {
            stat_ok++;
            if (nst == 0 || s.PresentCount != stv[nst - 1].s.PresentCount ||
                s.PresentRefreshCount != stv[nst - 1].s.PresentRefreshCount) {
                stat_t v = {qpc(), s};
                PUSH(stv, nst, cst, v);
            }
        } else {
            for (int k = 0; k < 4; k++)
                if (serr[k] == hs || serrn[k] == 0) {
                    serr[k] = hs;
                    serrn[k]++;
                    break;
                }
        }
        if (scm) {
            DXGI_FRAME_STATISTICS_MEDIA sm;
            HRESULT hm = IDXGISwapChainMedia_GetFrameStatisticsMedia(scm, &sm);
            if (hm == S_OK) {
                media_ok++;
                if (nmd == 0 || sm.PresentCount != mdv[nmd - 1].pc) {
                    media_t v = {qpc(), sm.PresentCount, (int)sm.CompositionMode,
                                 sm.ApprovedPresentDuration};
                    PUSH(mdv, nmd, cmd, v);
                }
            } else {
                for (int k = 0; k < 4; k++)
                    if (merr[k] == hm || merrn[k] == 0) {
                        merr[k] = hm;
                        merrn[k]++;
                        break;
                    }
            }
        }
        if (now >= next_dwm) {
            DWM_TIMING_INFO t = {.cbSize = sizeof t};
            if (SUCCEEDED(DwmGetCompositionTimingInfo(NULL, &t))) {
                dwm_t v = {now, t.cFrame, t.cRefresh};
                PUSH(dwv, ndw, cdw, v);
            }
            next_dwm = now + (int64_t)(0.1 * qpf);
        }
    }
    int64_t stop = qpc();

    /* Close the windows first, then report. */
    if (ctx1)
        ID3D11DeviceContext1_Release(ctx1);
    ID3D11RenderTargetView_Release(rtv);
    ID3D11Texture2D_Release(bb);
    if (scm)
        IDXGISwapChainMedia_Release(scm);
    IDXGISwapChain1_Release(sc);
    if (over)
        DestroyWindow(over);
    DestroyWindow(hwnd);
    printf("windows closed %.0f ms after the first Present\n", ms(stop - start));
    print_dwm("after:  ");

    /* 1. Present calls. */
    printf("\nPresent calls (QueryPerformanceCounter around each call):\n");
    if (nfr == 0) {
        printf("  none\n");
        return 1;
    }
    double run_s = ms(fr[nfr - 1].t1 - fr[0].t0) / 1e3;
    printf("  %zu calls in %.2f s = %.1f per second; DXGI_STATUS_OCCLUDED %lu; last error %s\n",
           nfr, run_s, nfr / run_s, occluded, hrname(perr));
    double *v = malloc(nfr * sizeof *v);
    for (size_t i = 0; i < nfr; i++)
        v[i] = ms(fr[i].t1 - fr[i].t0);
    print_dist("time inside Present", v, (int)nfr, "ms");
    printf("  GetLastPresentCount: %u after the first call, %u after the last\n", fr[0].pc,
           fr[nfr - 1].pc);
    printf("  foreground window is ours: %s; windows above ours that intersect its client "
           "area: %d of this program, %d others\n",
           foreground < 0 ? "?" : foreground ? "yes" : "no", own_above, other_above);

    /* 2. Frame statistics. */
    printf("\nGetFrameStatistics:\n  S_OK %lu", stat_ok);
    for (int k = 0; k < 4; k++)
        if (serrn[k])
            printf(", %s %lu", hrname(serr[k]), serrn[k]);
    printf("; %zu distinct values\n", nst);
    for (size_t i = 0; i < nst && i < 5; i++) {
        printf("  sample %zu: PresentCount %u, PresentRefreshCount %u, SyncRefreshCount %u, "
               "SyncQPCTime ",
               i, stv[i].s.PresentCount, stv[i].s.PresentRefreshCount,
               stv[i].s.SyncRefreshCount);
        if (stv[i].s.SyncQPCTime.QuadPart)
            printf("%+.3f ms from the first Present\n",
                   ms(stv[i].s.SyncQPCTime.QuadPart - fr[0].t0));
        else
            printf("0\n");
    }
    double period = 0;
    if (nst >= 2) {
        const DXGI_FRAME_STATISTICS *a0 = &stv[0].s, *a1 = &stv[nst - 1].s;
        UINT dr = a1->SyncRefreshCount - a0->SyncRefreshCount;
        if (dr && a0->SyncQPCTime.QuadPart)
            period = ms(a1->SyncQPCTime.QuadPart - a0->SyncQPCTime.QuadPart) / dr;
        UINT dp = a1->PresentCount - a0->PresentCount;
        UINT dpr = a1->PresentRefreshCount - a0->PresentRefreshCount;
        printf("  from the first to the last sample: PresentCount +%u, PresentRefreshCount +%u, "
               "SyncRefreshCount +%u\n",
               dp, dpr, dr);
        printf("  presents never reported as the newest one shown: %lu of %u (the statistics "
               "were read after every Present call)\n",
               (unsigned long)(dp - (UINT)(nst - 1)), dp);
        /* When did each reported present first appear in the statistics? This
         * needs no vblank fields; its resolution is the time between reads. */
        double *seen = malloc(nst * sizeof *seen);
        int nseen = 0;
        for (size_t i = 1; i < nst; i++) {
            const frame_t *pf = find_frame(fr, nfr, stv[i].s.PresentCount);
            if (pf)
                seen[nseen++] = ms(stv[i].seen - pf->t0);
        }
        print_dist("Present call to the first read that reported it", seen, nseen, "ms");
        free(seen);
        if (dpr == 0 || period <= 0) {
            printf("  the vblank fields did not advance: these statistics say which present "
                   "DXGI counted as shown, but not at which vblank\n");
            goto media;
        }
        printf("  vblank period from SyncQPCTime / SyncRefreshCount: %.3f ms (%.2f Hz)\n", period,
               1000.0 / period);
        unsigned long late = 0, late_refreshes = 0;
        for (size_t i = 1; i < nst; i++) {
            UINT p = stv[i].s.PresentCount - stv[i - 1].s.PresentCount;
            UINT r = stv[i].s.PresentRefreshCount - stv[i - 1].s.PresentRefreshCount;
            if (sync && r > p * sync) {
                late++;
                late_refreshes += r - p * sync;
            }
        }
        if (sync)
            printf("  steps where the screen kept the previous frame longer than %u vblank(s): "
                   "%lu, %lu extra vblanks in total\n",
                   sync, late, late_refreshes);
        /* Present-to-vblank latency of each present that was seen. */
        double *lat = malloc(nst * sizeof *lat);
        int nl = 0;
        for (size_t i = 0; i < nst; i++) {
            const frame_t *pf = find_frame(fr, nfr, stv[i].s.PresentCount);
            if (!pf || period <= 0)
                continue;
            double shown = ms(stv[i].s.SyncQPCTime.QuadPart - pf->t0) +
                           ((double)stv[i].s.PresentRefreshCount -
                            (double)stv[i].s.SyncRefreshCount) *
                               period;
            lat[nl++] = shown;
        }
        print_dist("Present call to the vblank DXGI reports it at", lat, nl, "ms");
        free(lat);
    }

    /* 3. Media statistics. */
media:
    printf("\nGetFrameStatisticsMedia:");
    if (!scm)
        printf(" not available (%s)\n", hrname(hqm));
    else {
        unsigned long mode[5] = {0};
        for (size_t i = 0; i < nmd; i++)
            mode[mdv[i].mode >= 0 && mdv[i].mode < 4 ? mdv[i].mode : 4]++;
        printf("\n  S_OK %lu", media_ok);
        for (int k = 0; k < 4; k++)
            if (merrn[k])
                printf(", %s %lu", hrname(merr[k]), merrn[k]);
        printf("; per distinct PresentCount: COMPOSED %lu, OVERLAY %lu, NONE %lu, "
               "COMPOSITION_FAILURE %lu, other %lu; ApprovedPresentDuration %u\n",
               mode[0], mode[1], mode[2], mode[3], mode[4], nmd ? mdv[nmd - 1].approved : 0);
    }

    /* 4. DWM counters and a per-second timeline. */
    if (ndw >= 2) {
        double dsec = ms(dwv[ndw - 1].seen - dwv[0].seen) / 1e3;
        printf("\nDWM over %.2f s: cRefresh +%llu (%.1f/s), cFrame +%llu (%.1f composed frames/s, "
               "all windows)\n",
               dsec, (unsigned long long)(dwv[ndw - 1].crefresh - dwv[0].crefresh),
               (dwv[ndw - 1].crefresh - dwv[0].crefresh) / dsec,
               (unsigned long long)(dwv[ndw - 1].cframe - dwv[0].cframe),
               (dwv[ndw - 1].cframe - dwv[0].cframe) / dsec);
    }
    printf("\n  second  presents  stats changes  PresentRefreshCount  DWM cFrame  DWM cRefresh  "
           "media C/O/N\n");
    int nsec = (int)ceil(ms(stop - start) / 1e3);
    UINT last_r = nst ? stv[0].s.PresentRefreshCount : 0;
    uint64_t last_f = ndw ? dwv[0].cframe : 0, last_c = ndw ? dwv[0].crefresh : 0;
    size_t fi = 0, si = 0, di = 0, mi = 0;
    for (int b = 0; b < nsec; b++) {
        int64_t lim = start + (int64_t)((b + 1) * qpf);
        unsigned long np = 0, ns = 0, mc[3] = {0};
        UINT r = last_r;
        uint64_t df = last_f, dc = last_c;
        for (; fi < nfr && fr[fi].t0 < lim; fi++)
            np++;
        for (; si < nst && stv[si].seen < lim; si++, ns++)
            r = stv[si].s.PresentRefreshCount;
        for (; di < ndw && dwv[di].seen < lim; di++)
            df = dwv[di].cframe, dc = dwv[di].crefresh;
        for (; mi < nmd && mdv[mi].seen < lim; mi++)
            if (mdv[mi].mode >= 0 && mdv[mi].mode < 3)
                mc[mdv[mi].mode]++;
        printf("  %6d  %8lu  %13lu  %19u  %10llu  %12llu  %lu/%lu/%lu\n", b, np, ns, r - last_r,
               (unsigned long long)(df - last_f), (unsigned long long)(dc - last_c), mc[0], mc[1],
               mc[2]);
        last_r = r, last_f = df, last_c = dc;
    }
    free(v);
    free(fr);
    free(stv);
    free(mdv);
    free(dwv);
    ID3D11DeviceContext_Release(ctx);
    ID3D11Device_Release(dev);
    if (output)
        IDXGIOutput_Release(output);
    if (adapter)
        IDXGIAdapter1_Release(adapter);
    IDXGIFactory2_Release(f2);
    IDXGIFactory1_Release(f1);
    return 0;
}

static void usage(void)
{
    printf("usage: flipstats info\n"
           "       flipstats windowed|overlap|borderless [seconds] [--vsync 0|1] [--sequential]\n"
           "                 [--buffers N] [--latency N]\n");
    exit(2);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    qpf = (double)f.QuadPart;
    /* Per-monitor DPI awareness: otherwise Windows scales the window's
     * pixels on a scaled monitor, and DWM has to compose it. */
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (argc < 2)
        usage();
    if (strcmp(argv[1], "info") == 0)
        return info_main();
    struct opts o = {.vsync = 1, .buffers = 2, .seconds = 4.0};
    if (strcmp(argv[1], "windowed") == 0)
        o.mode = MODE_WINDOWED;
    else if (strcmp(argv[1], "overlap") == 0)
        o.mode = MODE_OVERLAP;
    else if (strcmp(argv[1], "borderless") == 0)
        o.mode = MODE_BORDERLESS;
    else
        usage();
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--vsync") == 0 && i + 1 < argc)
            o.vsync = atoi(argv[++i]) != 0;
        else if (strcmp(argv[i], "--sequential") == 0)
            o.sequential = 1;
        else if (strcmp(argv[i], "--buffers") == 0 && i + 1 < argc)
            o.buffers = atoi(argv[++i]);
        else if (strcmp(argv[i], "--latency") == 0 && i + 1 < argc)
            o.latency = atoi(argv[++i]);
        else if (argv[i][0] != '-')
            o.seconds = atof(argv[i]);
        else
            usage();
    }
    if (o.seconds < 1)
        o.seconds = 1;
    if (o.seconds > MAX_SECONDS)
        o.seconds = MAX_SECONDS;
    if (o.buffers < 2 || o.buffers > 16)
        o.buffers = 2;
    if (o.latency < 0 || o.latency > 16)
        o.latency = 0;
    /* The watchdog: setup, the run, 250 ms of draining, and 5 s to spare. */
    CreateThread(NULL, 0, watchdog, (LPVOID)(uintptr_t)(DWORD)((o.seconds + 5.0) * 1000), 0, NULL);
    return run_main(&o);
}

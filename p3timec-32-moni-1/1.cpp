#include <windows.h>
#include <tchar.h>
#include <stdio.h>    // Include for _snwprintf
#include <algorithm>  // Include for std::min
#include <math.h>     // Include for sin and cos

#define WINDOW_CLASS_NAME _T("P3ClockWindowClass")
#define TIMER_ID 1

#define COLOR_BLUE  RGB(0, 162, 232)
#define COLOR_GREEN RGB(0, 200, 0)
#define COLOR_BLACK RGB(0, 0, 0)
#define PI 3.14159265358979323846

HFONT g_hFont = NULL; // Global font handle for digital clock

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Set a timer to trigger WM_TIMER message every second
            SetTimer(hwnd, TIMER_ID, 1000, NULL);
            break;
        }

        case WM_SIZE: {
            int windowWidth = LOWORD(lParam);
            int windowHeight = HIWORD(lParam);

            // Ensure valid window dimensions to prevent division by zero or invalid drawing
            if (windowWidth < 1 || windowHeight < 1) {
                break;
            }

            // Dynamically calculate font size based on window dimensions
            // Digital clock will occupy the right half
            int digitalClockWidth = windowWidth / 2;
            int digitalClockHeight = windowHeight;

            int fontSizeFromHeight = static_cast<int>(digitalClockHeight / 1.5);
            int fontSizeFromWidth = static_cast<int>(digitalClockWidth / 4.5);

            int newFontSize = std::min(fontSizeFromHeight, fontSizeFromWidth); 

            // Ensure minimum font size
            if (newFontSize < 1) {
                newFontSize = 1;
            }

            // If a font already exists, delete it to prevent memory leaks
            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL; // Set handle to NULL
            }

            // Create new font. Negative value for height means character height in pixels.
            g_hFont = CreateFont(
                -newFontSize,        // Font height (negative for character height)
                0,                   // Width (0 for automatic selection)
                0,                   // Escapement angle
                0,                   // Orientation angle
                FW_BOLD,             // Font weight: bold
                FALSE,               // Italic
                FALSE,               // Underline
                FALSE,               // Strikeout
                DEFAULT_CHARSET,     // Character set
                OUT_TT_PRECIS,       // Output precision
                CLIP_DEFAULT_PRECIS, // Clipping precision
                PROOF_QUALITY,       // Output quality
                VARIABLE_PITCH | FF_SWISS, // Font pitch and family
                _T("Arial")          // Font name
            );

            // Trigger window repaint to apply new font size
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_ERASEBKGND:
            // Prevent background erasing to avoid flicker before double-buffered drawing.
            // Return TRUE to tell Windows we've handled background erasing.
            return TRUE; 

        case WM_TIMER: {
            // Invalidate client area to force repaint on timer tick
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps); // Get device context for the window

            RECT clientRect;
            GetClientRect(hwnd, &clientRect); // Get client area dimensions

            // --- Implement double buffering to eliminate flicker ---
            HDC hdcMem = NULL;
            HBITMAP hbmBuffer = NULL;
            HBITMAP hbmOld = NULL;

            // Ensure valid client area dimensions before creating bitmap
            if (clientRect.right > clientRect.left && clientRect.bottom > clientRect.top) {
                // 1. Create a memory DC compatible with the window's DC (for off-screen drawing)
                hdcMem = CreateCompatibleDC(hdc);
                if (hdcMem) {
                    // 2. Create a bitmap compatible with the window's DC, and select it into the memory DC
                    hbmBuffer = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
                    if (hbmBuffer) {
                        hbmOld = (HBITMAP)SelectObject(hdcMem, hbmBuffer);

                        // 3. Perform all drawing operations on the memory DC
                        // Fill the entire background with black
                        HBRUSH hBrush = CreateSolidBrush(COLOR_BLACK);
                        FillRect(hdcMem, &clientRect, hBrush); // Fill background on memory DC
                        DeleteObject(hBrush); // Delete brush after use

                        SYSTEMTIME st;
                        GetLocalTime(&st); // Get current local time

                        COLORREF textColor;
                        // Determine text and hand color based on time
                        if (st.wHour == 0) {
                            textColor = COLOR_GREEN; // Green at midnight (0 AM)
                        } else {
                            textColor = COLOR_BLUE;  // Blue at other times
                        }

                        // --- Draw Analog Clock (Left Half) ---
                        RECT analogRect = {0, 0, clientRect.right / 2, clientRect.bottom}; // Left half of the window
                        int centerX = analogRect.left + (analogRect.right - analogRect.left) / 2;
                        int centerY = analogRect.top + (analogRect.bottom - analogRect.top) / 2;
                        int radius = std::min(analogRect.right - analogRect.left, analogRect.bottom - analogRect.top) / 2 - 20; // Leave margin

                        // 1. Save original GDI objects before custom drawing
                        HGDIOBJ hOldPenAnalog = SelectObject(hdcMem, GetStockObject(DC_PEN));
                        HGDIOBJ hOldBrushAnalog = SelectObject(hdcMem, GetStockObject(DC_BRUSH));

                        // 2. Fill the clock face with black
                        HBRUSH hBlackFaceBrush = CreateSolidBrush(COLOR_BLACK);
                        SelectObject(hdcMem, hBlackFaceBrush);
                        // Draw with a NULL_PEN so no outline is drawn for the fill operation
                        SelectObject(hdcMem, GetStockObject(NULL_PEN)); 
                        Ellipse(hdcMem, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
                        DeleteObject(hBlackFaceBrush); // Delete the black brush

                        // 3. Draw the colored border of the clock face
                        HPEN hBorderPen = CreatePen(PS_SOLID, 2, textColor);
                        SelectObject(hdcMem, hBorderPen);
                        // Ensure HOLLOW_BRUSH is selected so the border draw doesn't refill the circle
                        SelectObject(hdcMem, GetStockObject(HOLLOW_BRUSH)); 
                        Ellipse(hdcMem, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
                        DeleteObject(hBorderPen); // Delete the pen after use

                        // Draw Roman numerals for hours
                        const TCHAR* romanNumerals[] = {
                            _T(""), // Dummy for 0 index
                            _T("I"), _T("II"), _T("III"), _T("IV"), _T("V"), _T("VI"),
                            _T("VII"), _T("VIII"), _T("IX"), _T("X"), _T("XI"), _T("XII")
                        };

                        int numeralFontSize = radius / 5; // Font size for numerals, relative to clock radius
                        if (numeralFontSize < 8) numeralFontSize = 8; // Minimum numeral font size

                        HFONT hFontNumerals = CreateFont(
                            -numeralFontSize,
                            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_SWISS, _T("Arial")
                        );
                        HFONT hOldFontNumerals = (HFONT)SelectObject(hdcMem, hFontNumerals);

                        SetTextColor(hdcMem, textColor); // Numerals color same as clock hands
                        SetBkMode(hdcMem, TRANSPARENT);
                        
                        // --- 羅馬數字在時鐘內部 ---
                        int numeralInnerRadius = static_cast<int>(radius * 0.75); // 調整為在時鐘內部，離邊緣更近一些
                        for (int i = 1; i <= 12; ++i) {
                            // 計算角度，從12點方向開始，順時針
                            // 減去 90 度是為了讓 12 點位於上方，而不是右側
                            double hourMarkAngle = i * 30.0; 
                            double hourMarkRad = (hourMarkAngle - 90.0) * PI / 180.0; 

                            int numX = centerX + static_cast<int>(numeralInnerRadius * cos(hourMarkRad));
                            int numY = centerY + static_cast<int>(numeralInnerRadius * sin(hourMarkRad));

                            // 為每個數字創建一個小的矩形區域進行繪製，並使用 DT_CENTER | DT_VCENTER 居中
                            // 調整矩形大小以更好地適應數字並確保居中
                            RECT numRect = {numX - numeralFontSize, numY - numeralFontSize / 2, numX + numeralFontSize, numY + numeralFontSize / 2}; 
                            DrawText(hdcMem, romanNumerals[i], -1, &numRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
                        }

                        SelectObject(hdcMem, hOldFontNumerals); // Restore old font
                        DeleteObject(hFontNumerals); // Delete numeral font


                        // Calculate hand angles (from 12 o'clock position, clockwise)
                        // Second hand: 6 degrees per second
                        double secAngle = st.wSecond * 6.0;
                        // Minute hand: 6 degrees per minute + 0.1 degrees per second
                        double minAngle = st.wMinute * 6.0 + st.wSecond * 0.1;
                        // Hour hand: 30 degrees per hour + 0.5 degrees per minute
                        double hourAngle = (st.wHour % 12) * 30.0 + st.wMinute * 0.5;

                        // Draw Second Hand (longest, thinnest)
                        HPEN hSecPen = CreatePen(PS_SOLID, 1, textColor); // Second hand color matches digital clock
                        SelectObject(hdcMem, hSecPen);
                        MoveToEx(hdcMem, centerX, centerY, NULL);
                        LineTo(hdcMem, 
                            centerX + static_cast<int>(radius * 0.9 * sin(secAngle * PI / 180.0)),
                            centerY - static_cast<int>(radius * 0.9 * cos(secAngle * PI / 180.0)) // Y-axis inverted in GDI
                        );
                        DeleteObject(hSecPen); // Delete pen after use

                        // Draw Minute Hand (medium length, medium thickness)
                        HPEN hMinPen = CreatePen(PS_SOLID, 3, textColor); // Minute hand color matches digital clock
                        SelectObject(hdcMem, hMinPen);
                        MoveToEx(hdcMem, centerX, centerY, NULL);
                        LineTo(hdcMem, 
                            centerX + static_cast<int>(radius * 0.7 * sin(minAngle * PI / 180.0)),
                            centerY - static_cast<int>(radius * 0.7 * cos(minAngle * PI / 180.0))
                        );
                        DeleteObject(hMinPen); // Delete pen after use

                        // Draw Hour Hand (shortest, thickest)
                        HPEN hHourPen = CreatePen(PS_SOLID, 5, textColor); // Hour hand color matches digital clock
                        SelectObject(hdcMem, hHourPen);
                        MoveToEx(hdcMem, centerX, centerY, NULL);
                        LineTo(hdcMem, 
                            centerX + static_cast<int>(radius * 0.5 * sin(hourAngle * PI / 180.0)),
                            centerY - static_cast<int>(radius * 0.5 * cos(hourAngle * PI / 180.0))
                        );
                        DeleteObject(hHourPen); // Delete pen after use

                        // Restore original GDI objects
                        SelectObject(hdcMem, hOldPenAnalog); 
                        SelectObject(hdcMem, hOldBrushAnalog);


                        // --- Draw Digital Clock (Right Half) ---
                        RECT digitalRect = {clientRect.right / 2, 0, clientRect.right, clientRect.bottom}; // Right half of the window
                        TCHAR timeString[16]; // Buffer for formatted time string
                        _snwprintf(timeString, sizeof(timeString) / sizeof(TCHAR), _T("%02d:%02d:%02d"), st.wHour, st.wMinute, st.wSecond);

                        SetTextColor(hdcMem, textColor); // Set text color on memory DC
                        SetBkMode(hdcMem, TRANSPARENT);   // Set background mode to transparent on memory DC

                        HFONT hOldFont = NULL;
                        if (g_hFont) {
                            hOldFont = (HFONT)SelectObject(hdcMem, g_hFont); // Select current font into memory DC
                        } else {
                            // If g_hFont is NULL (font creation failed or not initialized), use system default font
                            hOldFont = (HFONT)SelectObject(hdcMem, GetStockObject(DEFAULT_GUI_FONT)); 
                        }

                        // Draw time text on memory DC
                        DrawText(hdcMem, timeString, -1, &digitalRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

                        SelectObject(hdcMem, hOldFont); // Restore old font to memory DC (important cleanup)

                        // 4. Copy the content of the memory DC to the actual window DC in one go
                        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

                        // 5. Clean up memory DC and bitmap resources
                        SelectObject(hdcMem, hbmOld); // Select original bitmap back
                        DeleteObject(hbmBuffer);      // Delete buffer bitmap
                    }
                    DeleteDC(hdcMem);             // Delete memory DC
                }
            }
            // --- End Double Buffering ---

            EndPaint(hwnd, &ps); // End painting
            break;
        }

        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID); // Stop timer
            // Release font resource
            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL;
            }
            PostQuitMessage(0); // Post quit message
            break;
        }

        default:
            // Handle all other messages
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSEX wc;
    // 使用 memset 進行完整的零初始化，這是消除所有警告的最可靠方法
    memset(&wc, 0, sizeof(WNDCLASSEX)); 
    wc.cbSize        = sizeof(WNDCLASSEX); 
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    // Add window icon
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION); // Load a standard application icon
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION); // Use the same standard icon for small icon
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Window background brush (recommended even with WM_ERASEBKGND)
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, _T("Window registration failed!"), _T("Error"), MB_ICONERROR | MB_OK);
        return 0;
    }

    // Create window
    HWND hwnd = CreateWindowEx(
        0,                  // Extended window style
        WINDOW_CLASS_NAME,  // Window class name
        _T("P3 Clock"),     // Window title
        WS_OVERLAPPEDWINDOW,// Window style: overlapped window (standard resizable window)
        CW_USEDEFAULT,      // Initial X position (system default)
        CW_USEDEFAULT,      // Initial Y position (system default)
        800,                // Initial width (adjusted for side-by-side clocks)
        400,                // Initial height
        NULL,               // Parent window handle
        NULL,               // Menu handle
        hInstance,          // Application instance handle
        NULL                // Creation parameters
    );

    if (!hwnd) {
        MessageBox(NULL, _T("Window creation failed!"), _T("Error"), MB_ICONERROR | MB_OK);
        return 0;
    }

    // Show and update window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); // Translate virtual-key messages
        DispatchMessage(&msg);  // Dispatch message to window procedure
    }

    return (int)msg.wParam;
}

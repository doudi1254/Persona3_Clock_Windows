#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <algorithm>

#define WINDOW_CLASS_NAME _T("P3ClockWindowClass")
#define TIMER_ID 1

#define COLOR_BLUE  RGB(0, 162, 232)
#define COLOR_GREEN RGB(0, 200, 0)
#define COLOR_BLACK RGB(0, 0, 0)

HFONT g_hFont = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            SetTimer(hwnd, TIMER_ID, 1000, NULL);
            break;
        }

        case WM_SIZE: {
            int windowWidth = LOWORD(lParam);
            int windowHeight = HIWORD(lParam);

            if (windowWidth < 1 || windowHeight < 1) {
                break;
            }

            int fontSizeFromHeight = windowHeight / 1.5;
            int fontSizeFromWidth = windowWidth / 4.5;

            int newFontSize = std::min(fontSizeFromHeight, fontSizeFromWidth); 

            if (newFontSize < 1) {
                newFontSize = 1;
            }

            if (g_hFont) {
                DeleteObject(g_hFont);
            }

            g_hFont = CreateFont(
                -newFontSize,
                0,
                0,
                0,
                FW_BOLD,
                FALSE,
                FALSE,
                FALSE,
                DEFAULT_CHARSET,
                OUT_TT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                PROOF_QUALITY,
                VARIABLE_PITCH | FF_SWISS,
                _T("Arial")
            );

            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_TIMER: {
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            HBRUSH hBrush = CreateSolidBrush(COLOR_BLACK);
            FillRect(hdc, &clientRect, hBrush);
            DeleteObject(hBrush);

            SYSTEMTIME st;
            GetLocalTime(&st);

            TCHAR timeString[16];
            _stprintf_s(timeString, _T("%02d:%02d:%02d"), st.wHour, st.wMinute, st.wSecond);

            COLORREF textColor;
            if (st.wHour == 0) {
                textColor = COLOR_GREEN;
            } else if (st.wHour == 1 && st.wMinute == 0 && st.wSecond == 0) {
                textColor = COLOR_BLUE;
            } else {
                textColor = COLOR_BLUE;
            }

            SetTextColor(hdc, textColor);
            SetBkMode(hdc, TRANSPARENT);

            HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);

            DrawText(hdc, timeString, -1, &clientRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            SelectObject(hdc, hOldFont);

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID);
            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL;
            }
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, _T("視窗註冊失敗!"), _T("錯誤"), MB_ICONERROR | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        0,
        WINDOW_CLASS_NAME,
        _T("P3 Clock"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBox(NULL, _T("視窗創建失敗!"), _T("錯誤"), MB_ICONERROR | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
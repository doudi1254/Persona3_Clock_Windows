#include <windows.h>
#include <tchar.h>
#include <stdio.h>    // 包含 _snwprintf 所需的頭文件
#include <algorithm>

// 為了避免某些編譯器對安全函式（如 _stprintf_s/_snprintf_s）的警告，
// 可以在文件開頭定義 _CRT_SECURE_NO_WARNINGS
// 但由於我們將直接使用 _snwprintf，這個宏變得不是那麼必要，
// 只是作為一個常見的 Win32 開發提示保留
// #define _CRT_SECURE_NO_WARNINGS 1 

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

            // 使用浮點數計算時，需要顯式轉換以避免截斷錯誤
            // 或者確保結果在轉換為 int 之前是正確的
            int fontSizeFromHeight = static_cast<int>(windowHeight / 1.5);
            int fontSizeFromWidth = static_cast<int>(windowWidth / 4.5);

            int newFontSize = std::min(fontSizeFromHeight, fontSizeFromWidth); 

            if (newFontSize < 1) {
                newFontSize = 1;
            }

            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL; // 設置為 NULL 以避免野指針
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
            // --- 關鍵修正：將 swprintf_s 替換為 _snwprintf ---
            // _snwprintf 是 MinGW 環境下更常見的寬字元安全格式化函式
            // 它的參數順序與 swprintf_s 相同，第二個參數是緩衝區大小
            _snwprintf(timeString, sizeof(timeString) / sizeof(TCHAR), _T("%02d:%02d:%02d"), st.wHour, st.wMinute, st.wSecond);

            COLORREF textColor;
            // 邏輯修改：根據時間設定顏色
            // 如果小時是 0，顏色為綠色
            // 其他情況，顏色為藍色
            if (st.wHour == 0) {
                textColor = COLOR_GREEN;
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
    // --- 修正 WNDCLASSEX 結構體初始化警告 ---
    // 最佳實踐：將整個結構體零初始化，然後再設定 cbSize
    WNDCLASSEX wc = {0}; 
    wc.cbSize        = sizeof(WNDCLASSEX); 
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    // 其餘成員現在會被零初始化

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

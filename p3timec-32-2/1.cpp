#include <windows.h>
#include <tchar.h>
#include <stdio.h>    // 包含 _snwprintf 所需的頭文件
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

            int fontSizeFromHeight = static_cast<int>(windowHeight / 1.5);
            int fontSizeFromWidth = static_cast<int>(windowWidth / 4.5);

            int newFontSize = std::min(fontSizeFromHeight, fontSizeFromWidth); 

            if (newFontSize < 1) {
                newFontSize = 1;
            }

            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL;
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

            // 當視窗大小改變時，觸發重繪
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_ERASEBKGND:
            // --- 關鍵修改1：阻止背景擦除 ---
            // 返回 TRUE 告訴 Windows，我們已經處理了背景擦除，不需要它再做了。
            // 這在雙緩衝中非常重要，因為我們會在離屏緩衝區中處理背景填充。
            return TRUE; 

        case WM_TIMER: {
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // --- 關鍵修改2：實現雙緩衝 ---
            // 1. 創建一個與窗口 DC 兼容的記憶體 DC
            HDC hdcMem = CreateCompatibleDC(hdc);

            // 2. 創建一個與窗口 DC 兼容的位圖，並選入記憶體 DC
            HBITMAP hbmBuffer = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmBuffer);

            // 3. 在記憶體 DC 上進行所有繪圖操作
            // 填充背景
            HBRUSH hBrush = CreateSolidBrush(COLOR_BLACK);
            FillRect(hdcMem, &clientRect, hBrush); // 在記憶體 DC 上填充
            DeleteObject(hBrush);

            SYSTEMTIME st;
            GetLocalTime(&st);

            TCHAR timeString[16];
            _snwprintf(timeString, sizeof(timeString) / sizeof(TCHAR), _T("%02d:%02d:%02d"), st.wHour, st.wMinute, st.wSecond);

            COLORREF textColor;
            if (st.wHour == 0) {
                textColor = COLOR_GREEN;
            } else {
                textColor = COLOR_BLUE;
            }

            SetTextColor(hdcMem, textColor); // 在記憶體 DC 上設定文字顏色
            SetBkMode(hdcMem, TRANSPARENT);   // 在記憶體 DC 上設定背景模式

            HFONT hOldFont = (HFONT)SelectObject(hdcMem, g_hFont); // 在記憶體 DC 上選入字體

            DrawText(hdcMem, timeString, -1, &clientRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER); // 在記憶體 DC 上繪製文字

            SelectObject(hdcMem, hOldFont); // 將舊字體選回記憶體 DC

            // 4. 將記憶體 DC 的內容一次性複製到實際窗口 DC
            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

            // 5. 清理記憶體 DC 和位圖
            SelectObject(hdcMem, hbmOld); // 選回舊位圖
            DeleteObject(hbmBuffer);      // 刪除位圖
            DeleteDC(hdcMem);             // 刪除記憶體 DC
            // --- 雙緩衝結束 ---

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
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // 雖然有了 WM_ERASEBKGND，這個設置仍然可以保留
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

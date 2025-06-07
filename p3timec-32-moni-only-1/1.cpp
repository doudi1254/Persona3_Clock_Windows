#include <windows.h>
#include <tchar.h>
#include <stdio.h>    // 包含 _snwprintf 所需的頭文件 (儘管此版本不再用於數字時鐘)
#include <algorithm>  // 包含 std::min 所需的頭文件
#include <math.h>     // 包含 sin 和 cos 所需的頭文件

#define WINDOW_CLASS_NAME _T("P3ClockWindowClass")
#define TIMER_ID 1

#define COLOR_BLUE  RGB(0, 162, 232)
#define COLOR_GREEN RGB(0, 200, 0)
#define COLOR_BLACK RGB(0, 0, 0)
#define PI 3.14159265358979323846

// 注意：在此版本中，g_hFont 主要用於通用字體創建，羅馬數字有自己的字體。
// 如果不再需要數字時鐘的動態字體，此全局字體句柄可以被移除。
HFONT g_hFont = NULL; 

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // 在視窗創建時設置定時器，每秒觸發一次 WM_TIMER 消息
            SetTimer(hwnd, TIMER_ID, 1000, NULL);
            break;
        }

        case WM_SIZE: {
            int windowWidth = LOWORD(lParam);
            int windowHeight = HIWORD(lParam);

            // 確保視窗尺寸有效，避免除以零或無效繪圖
            if (windowWidth < 1 || windowHeight < 1) {
                break;
            }

            // 此處的字體大小計算最初用於數字時鐘，
            // 在此版本中，由於只有模擬時鐘，可以為羅馬數字字體調整邏輯。
            // 暫時保留，但實際字體將在 WM_PAINT 中為羅馬數字單獨創建。
            int fontSizeFromHeight = static_cast<int>(windowHeight / 1.5);
            int fontSizeFromWidth = static_cast<int>(windowWidth / 4.5); // 原本是為數字時鐘寬度設計

            int newFontSize = std::min(fontSizeFromHeight, fontSizeFromWidth); 
            if (newFontSize < 1) {
                newFontSize = 1;
            }

            // 如果已有字體，先刪除舊字體以避免內存洩漏
            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL; 
            }

            // 創建新字體，但實際用於數字的羅馬數字字體將在 WM_PAINT 中創建。
            // 此處的 g_hFont 在此版本中作用不大，可以根據需要移除或重構。
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

            // 觸發視窗重繪
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_ERASEBKGND:
            // 阻止背景擦除，避免閃爍
            return TRUE; 

        case WM_TIMER: {
            // 定時器觸發時，強制重繪
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps); // 獲取視窗的設備上下文

            RECT clientRect;
            GetClientRect(hwnd, &clientRect); // 獲取視窗客戶區尺寸

            // --- 實現雙緩衝以消除閃爍 ---
            HDC hdcMem = NULL;
            HBITMAP hbmBuffer = NULL;
            HBITMAP hbmOld = NULL;

            // 確保客戶區有有效尺寸
            if (clientRect.right > clientRect.left && clientRect.bottom > clientRect.top) {
                hdcMem = CreateCompatibleDC(hdc);
                if (hdcMem) {
                    hbmBuffer = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
                    if (hbmBuffer) {
                        hbmOld = (HBITMAP)SelectObject(hdcMem, hbmBuffer);

                        // 在記憶體 DC 上填充整個背景為黑色
                        HBRUSH hBrush = CreateSolidBrush(COLOR_BLACK);
                        FillRect(hdcMem, &clientRect, hBrush); 
                        DeleteObject(hBrush); 

                        SYSTEMTIME st;
                        GetLocalTime(&st); // 獲取當前本地時間

                        COLORREF textColor;
                        // 根據時間設定顏色
                        if (st.wHour == 0) {
                            textColor = COLOR_GREEN; // 午夜 0 點顯示綠色
                        } else {
                            textColor = COLOR_BLUE;  // 其他時間顯示藍色
                        }

                        // --- 繪製模擬時鐘 (佔據整個視窗客戶區) ---
                        // 時鐘中心點位於視窗中心
                        int centerX = clientRect.left + (clientRect.right - clientRect.left) / 2;
                        int centerY = clientRect.top + (clientRect.bottom - clientRect.top) / 2;
                        // 半徑基於視窗較短邊，並留出邊距
                        int radius = std::min(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top) / 2 - 20; 
                        if (radius < 10) radius = 10; // 最小半徑

                        // 1. 保存原始 GDI 對象
                        HGDIOBJ hOldPenAnalog = SelectObject(hdcMem, GetStockObject(DC_PEN));
                        HGDIOBJ hOldBrushAnalog = SelectObject(hdcMem, GetStockObject(DC_BRUSH));

                        // 2. 填充時鐘錶盤為黑色 (它已經被背景填充為黑色，但為清晰起見，這裡可以再次填充)
                        // 注意：如果整個背景已經是黑色，這一步可能重複，但確保了圓盤內部是黑色。
                        HBRUSH hBlackFaceBrush = CreateSolidBrush(COLOR_BLACK);
                        SelectObject(hdcMem, hBlackFaceBrush);
                        SelectObject(hdcMem, GetStockObject(NULL_PEN)); // 不繪製填充圓的邊框
                        Ellipse(hdcMem, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
                        DeleteObject(hBlackFaceBrush); 

                        // 3. 繪製錶盤邊框
                        HPEN hBorderPen = CreatePen(PS_SOLID, 2, textColor);
                        SelectObject(hdcMem, hBorderPen);
                        SelectObject(hdcMem, GetStockObject(HOLLOW_BRUSH)); // 確保只繪製邊框，不填充
                        Ellipse(hdcMem, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
                        DeleteObject(hBorderPen); 

                        // 繪製羅馬數字小時刻度
                        const TCHAR* romanNumerals[] = {
                            _T(""), // 索引 0 無用
                            _T("I"), _T("II"), _T("III"), _T("IV"), _T("V"), _T("VI"),
                            _T("VII"), _T("VIII"), _T("IX"), _T("X"), _T("XI"), _T("XII")
                        };

                        int numeralFontSize = radius / 5; // 字體大小相對半徑
                        if (numeralFontSize < 8) numeralFontSize = 8; // 最小字體大小

                        HFONT hFontNumerals = CreateFont(
                            -numeralFontSize,
                            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_SWISS, _T("Arial")
                        );
                        HFONT hOldFontNumerals = (HFONT)SelectObject(hdcMem, hFontNumerals);

                        SetTextColor(hdcMem, textColor); // 羅馬數字顏色與指針顏色同步
                        SetBkMode(hdcMem, TRANSPARENT);
                        
                        // 羅馬數字在時鐘內部
                        int numeralInnerRadius = static_cast<int>(radius * 0.75); // 調整為在時鐘內部
                        for (int i = 1; i <= 12; ++i) {
                            // 計算角度 (從12點方向開始，順時針)
                            // 減去 90 度是為了讓 12 點位於上方 (0度在GDI中是右側)
                            double hourMarkAngle = i * 30.0; 
                            double hourMarkRad = (hourMarkAngle - 90.0) * PI / 180.0; 

                            int numX = centerX + static_cast<int>(numeralInnerRadius * cos(hourMarkRad));
                            int numY = centerY + static_cast<int>(numeralInnerRadius * sin(hourMarkRad));

                            // 為每個數字創建一個小的矩形區域，並居中繪製
                            RECT numRect = {numX - numeralFontSize, numY - numeralFontSize / 2, numX + numeralFontSize, numY + numeralFontSize / 2}; 
                            DrawText(hdcMem, romanNumerals[i], -1, &numRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
                        }

                        SelectObject(hdcMem, hOldFontNumerals); // 選回舊字體
                        DeleteObject(hFontNumerals); // 刪除羅馬數字字體


                        // 計算指針角度（從12點方向開始，順時針）
                        // 秒針：每秒 6 度
                        double secAngle = st.wSecond * 6.0;
                        // 分針：每分鐘 6 度 + 每秒 0.1 度
                        double minAngle = st.wMinute * 6.0 + st.wSecond * 0.1;
                        // 時針：每小時 30 度 + 每分鐘 0.5 度
                        double hourAngle = (st.wHour % 12) * 30.0 + st.wMinute * 0.5;

                        // 繪製秒針 (最長，最細)
                        HPEN hSecPen = CreatePen(PS_SOLID, 1, textColor); 
                        SelectObject(hdcMem, hSecPen);
                        MoveToEx(hdcMem, centerX, centerY, NULL);
                        LineTo(hdcMem, 
                            centerX + static_cast<int>(radius * 0.9 * sin(secAngle * PI / 180.0)),
                            centerY - static_cast<int>(radius * 0.9 * cos(secAngle * PI / 180.0)) // Y軸在GDI中是反向的
                        );
                        DeleteObject(hSecPen); 

                        // 繪製分針 (中等長度，中等粗細)
                        HPEN hMinPen = CreatePen(PS_SOLID, 3, textColor); 
                        SelectObject(hdcMem, hMinPen);
                        MoveToEx(hdcMem, centerX, centerY, NULL);
                        LineTo(hdcMem, 
                            centerX + static_cast<int>(radius * 0.7 * sin(minAngle * PI / 180.0)),
                            centerY - static_cast<int>(radius * 0.7 * cos(minAngle * PI / 180.0))
                        );
                        DeleteObject(hMinPen); 

                        // 繪製時針 (最短，最粗)
                        HPEN hHourPen = CreatePen(PS_SOLID, 5, textColor); 
                        SelectObject(hdcMem, hHourPen);
                        MoveToEx(hdcMem, centerX, centerY, NULL);
                        LineTo(hdcMem, 
                            centerX + static_cast<int>(radius * 0.5 * sin(hourAngle * PI / 180.0)),
                            centerY - static_cast<int>(radius * 0.5 * cos(hourAngle * PI / 180.0))
                        );
                        DeleteObject(hHourPen); 

                        // 恢復原始 GDI 對象
                        SelectObject(hdcMem, hOldPenAnalog); 
                        SelectObject(hdcMem, hOldBrushAnalog);

                        // 4. 將記憶體 DC 的內容一次性複製到實際視窗 DC
                        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

                        // 5. 清理記憶體 DC 和位圖資源
                        SelectObject(hdcMem, hbmOld); // 選回原始位圖
                        DeleteObject(hbmBuffer);      // 刪除緩衝位圖
                    }
                    DeleteDC(hdcMem);             // 刪除記憶體 DC
                }
            }
            // --- 雙緩衝結束 ---

            EndPaint(hwnd, &ps); // 結束繪圖
            break;
        }

        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID); // 停止定時器
            // 釋放字體資源
            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL;
            }
            PostQuitMessage(0); // 發送退出消息
            break;
        }

        default:
            // 處理所有其他消息
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 註冊視窗類
    WNDCLASSEX wc;
    // 使用 memset 進行完整的零初始化
    memset(&wc, 0, sizeof(WNDCLASSEX)); 
    wc.cbSize        = sizeof(WNDCLASSEX); 
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    // 添加視窗圖標
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION); 
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION); 
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); 
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, _T("視窗註冊失敗!"), _T("錯誤"), MB_ICONERROR | MB_OK);
        return 0;
    }

    // 創建視窗
    HWND hwnd = CreateWindowEx(
        0,                  
        WINDOW_CLASS_NAME,  
        _T("P3 Clock"),     
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,      
        CW_USEDEFAULT,      
        400,                // 寬度調整為一個時鐘的大小
        400,                // 保持高度
        NULL,               
        NULL,               
        hInstance,          
        NULL                
    );

    if (!hwnd) {
        MessageBox(NULL, _T("視窗創建失敗!"), _T("錯誤"), MB_ICONERROR | MB_OK);
        return 0;
    }

    // 顯示和更新視窗
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循環
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); 
        DispatchMessage(&msg);  
    }

    return (int)msg.wParam;
}


#include "DeviceDetectionServer.h"
#include "font.h"
#include "device_detection/device_detection.h"

#include "application.hpp"
#include <memory>
#include <vector>
#include <shellapi.h>

HINSTANCE g_instance;

std::shared_ptr<application> app;
std::shared_ptr<font> font_ubuntu;
std::vector<std::string> devices;

HBITMAP hBitmap;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Set center
        RECT rc;
        GetWindowRect(hwnd, &rc);
        int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
        int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;
        SetWindowPos(hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        // Set background color
        HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)brush);

        auto [devices_list, devices_json] = device_detection::detect_and_get_json_str();
        devices = devices_list;

        hBitmap = LoadBitmap(g_instance, MAKEINTRESOURCE(IDB_NOT_FOUND)); 
  
        auto ubuntu_font = std::make_shared<font>(g_instance, IDR_UBUNTU_R);
        app->start(hwnd, std::move(devices_json));

    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        SelectObject(hdc, GetStockObject(DC_PEN));
        SetDCPenColor(hdc, 0x00eeeeee);
        MoveToEx(hdc, 16, 1, NULL);
        LineTo(hdc, 568, 1);

        if (devices.empty()) 
        {
            BITMAP bitmap;
            HDC hdcMem = CreateCompatibleDC(hdc);
            HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmap);

            GetObject(hBitmap, sizeof(bitmap), &bitmap);
            BitBlt(hdc, 268, 64, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);

            RECT title_rect{ 32, 135, 568, 150 };
            SetTextColor(hdc, 0x003a3939);
            SetBkColor(hdc, 0x00ffffff);
            auto title_font = font_ubuntu->get(hwnd, 14, true);
            SelectObject(hdc, title_font);
            DrawText(hdc, L"No devices found", -1, &title_rect, DT_SINGLELINE | DT_NOCLIP | DT_CENTER);
            DeleteObject(title_font);
        }
        else
        {
            RECT title_rect{ 32, 24, 568, 150 };
            SetTextColor(hdc, 0x00959595);
            SetBkColor(hdc, 0x00ffffff);

            auto title_font = font_ubuntu->get(hwnd, 9);
            SelectObject(hdc, title_font);
            std::string title_string = "Detected devices (" + std::to_string(devices.size()) + ")";
            DrawText(hdc, std::wstring(title_string.begin(), title_string.end()).data(), -1, &title_rect, DT_SINGLELINE | DT_NOCLIP);
            DeleteObject(title_font);

            auto text_font = font_ubuntu->get(hwnd, 11);
            SelectObject(hdc, text_font);

            for (int i = 0; i < devices.size() && i < 6; ++i) 
            {
                SetDCPenColor(hdc, 0x00eeeeee);
                RoundRect(hdc, 16, 47 + i*(34 + 8), 568, 81 + i * (34 + 8), 8, 8); //width 568, height 34

                RECT num_rect{ 32, 47 + 9 + i * (34 + 8), 90, 81 + 9 + i * (34 + 8) };
                SetTextColor(hdc, 0x00959595);
                std::string num_string = std::to_string(i + 1) + "#";
                DrawText(hdc, std::wstring(num_string.begin(), num_string.end()).data(), -1, &num_rect, DT_SINGLELINE | DT_NOCLIP);

                RECT dev_rect{ 60, 47 + 9 + i * (34 + 8), 560, 81 + 9 + i * (34 + 8) };
                SetTextColor(hdc, 0x003a3939);
                std::string &dev_string = devices[i];
                DrawText(hdc, std::wstring(dev_string.begin(), dev_string.end()).data(), -1, &dev_rect, DT_SINGLELINE);
            }
            DeleteObject(text_font);


            if (devices.size() > 6) 
            {
                auto dot_font = font_ubuntu->get(hwnd, 14);
                SelectObject(hdc, dot_font);
                RECT num_rect{ 32, 47 + 6 * (34 + 8), 90, 81 + 6 * (34 + 8) };
                SetTextColor(hdc, 0x00959595);
                DrawText(hdc, L"...", -1, &num_rect, DT_SINGLELINE | DT_NOCLIP);
                DeleteObject(dot_font);
            }
        }

        SetDCPenColor(hdc, 0x00eeeeee);
        MoveToEx(hdc, 16, 338, NULL);
        LineTo(hdc, 568, 338);

        RECT footer_rect{ 16, 352, 568, 380 };
        auto footer_font = font_ubuntu->get(hwnd, 10);
        SetTextColor(hdc, 0x00000000);
        SetBkColor(hdc, 0x00ffffff);
        SelectObject(hdc, footer_font);
        DrawText(hdc, L"Please wait for application to close ...", -1, &footer_rect, DT_SINGLELINE | DT_NOCLIP);
        DeleteObject(footer_font);

        EndPaint(hwnd, &ps);
    }
    break;
    case WM_CLOSE:
    {
        DestroyWindow(hwnd);
    }
    break;
    case WM_DESTROY:
    {
        font_ubuntu = nullptr;
        app->stop();
        app = nullptr;
        DeleteObject(hBitmap);
        PostQuitMessage(0);
    }
    break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

//https://www.catch22.net/tuts/win32/self-deleting-executables#
BOOL SelfDelete()
{
    TCHAR szFile[MAX_PATH], szCmd[MAX_PATH];

    if ((GetModuleFileName(0, szFile, MAX_PATH) != 0) &&
        (GetShortPathName(szFile, szFile, MAX_PATH) != 0))
    {
        lstrcpy(szCmd, L"/c del ");
        lstrcat(szCmd, szFile);
        lstrcat(szCmd, L" >> NUL");

        if ((GetEnvironmentVariable(L"ComSpec", szFile, MAX_PATH) != 0) &&
            ((INT)ShellExecute(0, 0, szFile, szCmd, 0, SW_HIDE) > 32))
            return TRUE;
    }

    return FALSE;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_instance = hInstance;

    const wchar_t szUniqueNamedMutex[] = L"com_nicehash_device_detection_server";
    HANDLE hHandle = CreateMutex(NULL, TRUE, szUniqueNamedMutex);
    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        return(1);
    }

    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    app = std::make_shared<application>();

    const wchar_t* g_szClassName = L"device_detection_server";

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON));

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        0,
        g_szClassName,
        L"NiceHash Device Detector",
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 420,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    ReleaseMutex(hHandle);
    CloseHandle(hHandle);

    SelfDelete();

    return static_cast<int>(Msg.wParam);
}

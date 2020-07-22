// DeviceDetectionServer.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "DeviceDetectionServer.h"

#include "application.hpp"
#include <memory>
#include "device_detection/device_detection.h"


std::shared_ptr<application> app;
HWND hdnw_label;
std::string g_devices;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        auto [devices_list, devices_json] = device_detection::detect_and_get_json_str();
        g_devices = devices_list;
        app->start(hWnd, std::move(devices_json));
    }
    break;
    case WM_CLOSE:
    {
        DestroyWindow(hWnd);
    }
    break;
    case WM_DESTROY:
    {
        app->stop();
        app = nullptr;
        PostQuitMessage(0);
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DEVICEDETECTIONSERVER));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        g_szClassName,
        L"NiceHash Device Detector",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    hdnw_label = CreateWindow(L"static", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        10, 10, 600, 380,
        hwnd, (HMENU)(501),
        hInstance, NULL);
    SetWindowText(hdnw_label, std::wstring(g_devices.begin(), g_devices.end()).data());

    HWND hdnw_label_static = CreateWindow(L"static", L"Wait for application to close.",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        10, 400, 600, 35,
        hwnd, (HMENU)(501),
        hInstance, NULL);

    HANDLE hMyFont = INVALID_HANDLE_VALUE; // Here, we will (hopefully) get our font handle
    HRSRC  hFntRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_FONT2), RT_FONT);
    if (hFntRes) { // If we have found the resource ... 
        HGLOBAL hFntMem = LoadResource(hInstance, hFntRes); // Load it
        if (hFntMem != nullptr) {
            void* FntData = LockResource(hFntMem); // Lock it into accessible memory
            DWORD nFonts = 0, len = SizeofResource(hInstance, hFntRes);
            hMyFont = AddFontMemResourceEx(FntData, len, nullptr, &nFonts); // Fake install font!
        }
    }

    LOGFONT MyLogFont = { -8, 0,   0, 0, 400, FALSE, FALSE, FALSE, ANSI_CHARSET,
                   OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                   VARIABLE_PITCH | FF_SWISS, L"Ubuntu-R" };
    MyLogFont.lfCharSet = DEFAULT_CHARSET;
    MyLogFont.lfHeight = -20;
    HFONT hFont = CreateFontIndirect(&MyLogFont);
    SendMessage(hdnw_label, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hdnw_label_static, WM_SETFONT, (WPARAM)hFont, TRUE);
    // release
    RemoveFontMemResourceEx(hMyFont);


    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    ReleaseMutex(hHandle);
    CloseHandle(hHandle);
    return Msg.wParam;
}
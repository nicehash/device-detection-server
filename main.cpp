#include "application.hpp"
#include <windows.h>
#include <memory>

std::shared_ptr<application> app;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {
    case WM_CREATE:
        app->start();
        break;
    case WM_CLOSE:
        app->stop();
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        app = nullptr;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
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
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_TRANSPARENT,
        g_szClassName,
        L"",
        0,
        -1000, -1000, 0, 0,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    long wAttr = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, wAttr | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, 0, 0x02);

    ShowWindow(hwnd, SW_MINIMIZE);
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

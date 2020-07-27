#include "font.h"


font::font(HINSTANCE instance, int resource_id_font)
{
    hMyFont = INVALID_HANDLE_VALUE; // Here, we will (hopefully) get our font handle
    hFntRes = FindResource(instance, MAKEINTRESOURCE(resource_id_font), RT_FONT);
    if (hFntRes) { // If we have found the resource ... 
        HGLOBAL hFntMem = LoadResource(instance, hFntRes); // Load it
        if (hFntMem != nullptr) {
            void* FntData = LockResource(hFntMem); // Lock it into accessible memory
            DWORD nFonts = 0, len = SizeofResource(instance, hFntRes);
            hMyFont = AddFontMemResourceEx(FntData, len, nullptr, &nFonts); // Fake install font!
        }
    }
}

font::~font()
{
    if (hMyFont != INVALID_HANDLE_VALUE) {
        RemoveFontMemResourceEx(hMyFont);
    }
}

HFONT font::get(HWND hwnd, int size, bool bold)
{
    BYTE lfWeight = bold ? FW_BOLD : FW_NORMAL;
    LOGFONT MyLogFont = { -MulDiv(size, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 72), 0, 0, 0, lfWeight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Ubuntu-R" };
    return CreateFontIndirect(&MyLogFont);
}
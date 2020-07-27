#pragma once

#include <Windows.h>


class font
{
public:
	font(HINSTANCE instance, int resource_id_font);
	~font();

	HFONT get(HWND hwnd, int size, bool bold = false);

private:
	HANDLE hMyFont;
	HRSRC  hFntRes;
};


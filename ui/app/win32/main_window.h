#pragma once

#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <windows.h>

#define ID_QUIT 0x70

class MainWindow {
public:
    HWND hwnd;

    MainWindow() {}

    // TODO: Change this.
    PCWSTR className() const {
        return L"Clock Window Class";
    }
    BOOL create(PCWSTR lpWindowName, DWORD dwStyle);
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HDC ghDC;
    HGLRC ghRC;
};

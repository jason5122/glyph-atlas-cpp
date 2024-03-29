#pragma once

#include "ui/win32/base_window.h"

class MainWindow : public BaseWindow<MainWindow> {
public:
    PCWSTR ClassName() const;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

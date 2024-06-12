#pragma once

#include "config/color_scheme.h"
#include "gui/window.h"

class SimpleText;

class EditorWindow : public gui::Window {
public:
    EditorWindow(SimpleText& parent, int width, int height, int wid);

    void onOpenGLActivate(int width, int height) override;
    void onDraw() override;
    void onResize(int width, int height) override;
    void onClose() override;

private:
    int wid;
    SimpleText& parent;

    config::ColorScheme color_scheme;

    void updateWindowTitle();
};

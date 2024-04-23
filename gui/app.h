#pragma once

#include "gui/key.h"
#include "gui/modifier_key.h"
#include <memory>

#include "gui/gtk/dummy_window.h"

class App {
public:
    DummyWindow* dummy_window;

    class Window {
    public:
        Window(App& parent, int width, int height);
        ~Window();
        void show();
        void close();
        void redraw();

        int width();
        int height();
        int scaleFactor();

        virtual void onOpenGLActivate(int width, int height) {}
        virtual void onDraw() {}
        virtual void onResize(int width, int height) {}
        virtual void onScroll(float dx, float dy) {}
        virtual void onLeftMouseDown(float mouse_x, float mouse_y) {}
        virtual void onLeftMouseDrag(float mouse_x, float mouse_y) {}
        virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) {}
        virtual void onClose() {}

    private:
        App& parent;

        class impl;
        std::unique_ptr<impl> pimpl;
    };

    App();
    ~App();
    void run();
    void quit();

    virtual void onLaunch() {}

protected:
    class impl;
    std::unique_ptr<impl> pimpl;
};

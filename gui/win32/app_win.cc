#include "gui/app.h"
#include "gui/win32/dummy_context.h"
#include "gui/win32/main_window.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <memory>
#include <windows.h>

class App::impl {
public:
    DummyContext dummy_context;
};

App::App() : pimpl{new impl{}} {}

void App::run() {
    pimpl->dummy_context.initialize();

    this->onLaunch();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void App::quit() {
    PostQuitMessage(0);
}

App::~App() {}

class App::Window::impl {
public:
    impl(App::Window& app_window, DummyContext& dummy_context)
        : main_window(app_window, dummy_context) {}

    MainWindow main_window;

    // TODO: Implement unique class name creation in a better way.
    int wid = 0;
};

App::Window::Window(App& parent, int width, int height)
    : pimpl{new impl{*this, parent.pimpl->dummy_context}}, parent(parent) {
    // pimpl->main_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW, pimpl->wid++);
}

App::Window::~Window() {}

void App::Window::show() {
    pimpl->main_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW, pimpl->wid++);

    // TODO: Sync this with requested width/height.
    int width = 1200;
    int height = 600;

    // FIXME: This doesn't animate like ShowWindow().
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        // .showCmd = SW_MAXIMIZE,
        .showCmd = SW_NORMAL,
        .rcNormalPosition = RECT{0, 0, width * 2, height * 2},
    };
    SetWindowPlacement(pimpl->main_window.m_hwnd, &placement);
}

void App::Window::close() {
    pimpl->main_window.destroy();
}

void App::Window::redraw() {
    pimpl->main_window.redraw();
}

int App::Window::width() {
    return pimpl->main_window.width();
}

int App::Window::height() {
    return pimpl->main_window.height();
}

int App::Window::scaleFactor() {
    return pimpl->main_window.scaleFactor();
}

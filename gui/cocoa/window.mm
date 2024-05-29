#include "gui/cocoa/pimpl_mac.h"
#include "gui/window.h"

namespace gui {

Window::Window(App& app) : pimpl{new impl{}}, app(app) {
    // NSRect frame = NSMakeRect(500, 0, width, height);
    NSRect frame = NSScreen.mainScreen.visibleFrame;

    frame.origin.y = 300;
    frame.size.height -= 300;

    std::cerr << "scale factor: " << NSScreen.mainScreen.backingScaleFactor << '\n';

    DisplayGL* displaygl = app.pimpl->displaygl.get();
    pimpl->window_controller = [[WindowController alloc] initWithFrame:frame
                                                             appWindow:this
                                                             displayGl:displaygl];

    // Implement window cascading.
    // if (NSEqualPoints(parent.pimpl->cascading_point, NSZeroPoint)) {
    //     NSPoint point = pimpl->ns_window.frame.origin;
    //     parent.pimpl->cascading_point = [pimpl->ns_window cascadeTopLeftFromPoint:point];

    //     [pimpl->ns_window center];
    // } else {
    //     parent.pimpl->cascading_point =
    //         [pimpl->ns_window cascadeTopLeftFromPoint:parent.pimpl->cascading_point];
    // }
}

Window::~Window() {
    std::cerr << "Window::~Window()\n";
    [pimpl->window_controller release];
}

void Window::show() {
    [pimpl->window_controller show];
}

void Window::close() {
    [pimpl->window_controller close];
}

void Window::redraw() {
    [pimpl->window_controller redraw];
}

int Window::width() {
    return [pimpl->window_controller getWidth];
}

int Window::height() {
    return [pimpl->window_controller getHeight];
}

int Window::scaleFactor() {
    return [pimpl->window_controller getScaleFactor];
}

bool Window::isDarkMode() {
    return [pimpl->window_controller isDarkMode];
}

}

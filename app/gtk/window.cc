#include "app/window.h"

#include "app/gtk/impl_gtk.h"

#include <fmt/base.h>

namespace app {

Window::Window(App& app, int width, int height)
    : pimpl{new impl{app.pimpl->app, this, app.pimpl->context}} {}

Window::~Window() {}

void Window::show() {
    pimpl->main_window.show();
}

void Window::close() {
    pimpl->main_window.close();
}

void Window::redraw() {
    pimpl->main_window.redraw();
}

int Window::width() const {
    return pimpl->main_window.width();
}

int Window::height() const {
    return pimpl->main_window.height();
}

int Window::scale() const {
    return pimpl->main_window.scaleFactor();
}

bool Window::isDarkMode() const {
    // return pimpl->main_window.isDarkMode();
    return false;
}

void Window::setTitle(std::string_view title) {
    pimpl->main_window.setTitle(title);
}

// TODO: Implement this.
void Window::setFilePath(std::string_view path) {}

// TODO: Implement this.
std::optional<std::string> Window::openFilePicker() const {
    return {};
}

void Window::setCursorStyle(CursorStyle style) {
    GdkCursor* cursor = nullptr;
    if (style == CursorStyle::kArrow) {
        cursor = gdk_cursor_new_from_name("default", nullptr);
    } else if (style == CursorStyle::kIBeam) {
        cursor = gdk_cursor_new_from_name("text", nullptr);
    } else if (style == CursorStyle::kResizeLeftRight) {
        cursor = gdk_cursor_new_from_name("ew-resize", nullptr);
    } else if (style == CursorStyle::kResizeUpDown) {
        cursor = gdk_cursor_new_from_name("ns-resize", nullptr);
    }

    if (cursor) {
        GtkWidget* gtk_window = pimpl->main_window.gtkWindow();
        gtk_widget_set_cursor(gtk_window, cursor);
        g_object_unref(cursor);
    }
}

namespace {
gboolean TickCallback(GtkWidget* widget, GdkFrameClock* frame_clock, gpointer user_data) {
    Window* app_window = static_cast<Window*>(user_data);

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    double fps = gdk_frame_clock_get_fps(frame_clock);
    fmt::println("fps = {}, frame_time = {}", fps, frame_time);
    app_window->onFrame(frame_time);

    return G_SOURCE_CONTINUE;
}
}  // namespace

// TODO: Refactor this.
void Window::setAutoRedraw(bool auto_redraw) {
    // fmt::println("!{} and {} = {}", auto_redraw, pimpl->has_tick_callback,
    //              !auto_redraw && pimpl->has_tick_callback);

    GtkWidget* gtk_window = pimpl->main_window.glArea();
    if (!auto_redraw && pimpl->has_tick_callback) {
        fmt::println("remove");
        gtk_widget_remove_tick_callback(gtk_window, pimpl->tick_callback_id);
        pimpl->has_tick_callback = false;
        pimpl->tick_callback_id = 0;
    } else if (auto_redraw && !pimpl->has_tick_callback) {
        fmt::println("add");
        guint id = gtk_widget_add_tick_callback(gtk_window, &TickCallback, this, nullptr);
        pimpl->has_tick_callback = true;
        pimpl->tick_callback_id = id;
    }
}

// TODO: Implement.
int Window::framesPerSecond() const {
    return 60;
}

// void Window::createMenuDebug() const {
//     GtkWidget* window = pimpl->main_window.gtkWindow();

//     GMenu* menu = g_menu_new();
//     GMenuItem* item = g_menu_item_new("Expelliarmus", "app.expelliarmus");
//     g_menu_append_item(menu, item);
//     g_menu_append(menu, "Incendio", "app.incendio");

//     GdkRectangle rect{
//         .x = 0,
//         .y = 0,
//         .width = 0,
//         .height = 0,
//     };

//     GtkWidget* popover_menu = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
//     gtk_popover_set_has_arrow(GTK_POPOVER(popover_menu), false);
//     gtk_popover_set_pointing_to(GTK_POPOVER(popover_menu), &rect);
//     gtk_widget_set_parent(popover_menu, window);
//     // gtk_widget_set_visible(popover_menu, true);
//     gtk_popover_popup(GTK_POPOVER(popover_menu));

//     fmt::println("Context menu closed.");
// }

}  // namespace app

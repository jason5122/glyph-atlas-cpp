#include "renderer/renderer.h"
#include "status_bar_widget.h"

namespace gui {

StatusBarWidget::StatusBarWidget(const renderer::Size& size) : Widget{size} {}

void StatusBarWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer::g_renderer->getRectRenderer().addRect({offset.x, screen_size.height - size.height},
                                                    {screen_size.width, size.height},
                                                    {199, 203, 209, 255});
}

void StatusBarWidget::scroll(const renderer::Point& delta) {}

void StatusBarWidget::leftMouseDown(const renderer::Point& mouse, const renderer::Point& offset) {}

void StatusBarWidget::leftMouseDrag(const renderer::Point& mouse, const renderer::Point& offset) {}

}

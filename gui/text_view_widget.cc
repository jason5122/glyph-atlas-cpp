#include "renderer/renderer.h"
#include "text_view_widget.h"
#include <cmath>

#include <iostream>

namespace gui {

TextViewWidget::TextViewWidget(const renderer::Size& size) : Widget{size} {}

void TextViewWidget::draw() {
    renderer::TextRenderer& text_renderer = renderer::g_renderer->getTextRenderer();
    renderer::RectRenderer& rect_renderer = renderer::g_renderer->getRectRenderer();
    renderer::SelectionRenderer& selection_renderer = renderer::g_renderer->getSelectionRenderer();

    // TODO: Add this to parameters.
    int line_number_offset = 100;

    int longest_line = 0;
    renderer::Point end_caret_pos;
    text_renderer.renderText(size, scroll_offset, buffer, position, start_caret, end_caret,
                             longest_line, end_caret_pos);

    // Add selections.
    // TODO: Batch this in with text renderer (or better yet, unify into one text layout step).
    auto selections = selection_renderer.getSelections(buffer, start_caret, end_caret);

    renderer::Point selection_offset = position - scroll_offset;
    selection_offset.x += line_number_offset;
    selection_renderer.createInstances(selection_offset, selections);

    // Add vertical scroll bar.
    int line_count = buffer.lineCount();
    int line_height = text_renderer.lineHeight();
    int vbar_width = 15;
    int visible_lines = std::ceil(static_cast<float>(size.height) / line_height);
    int max_scrollbar_y = (line_count + visible_lines) * line_height;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scrollbar_y);
    float vbar_percent = static_cast<float>(scroll_offset.y) / (line_count * line_height);

    renderer::Point coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRoundedRect(coords + position, {vbar_width, vbar_height},
                                 {190, 190, 190, 255}, 5);

    // Add caret.
    int caret_width = 4;
    int caret_height = line_height;

    int extra_padding = 8;
    caret_height += extra_padding * 2;

    const renderer::Point caret_pos{
        .x = end_caret_pos.x - caret_width / 2 - scroll_offset.x + line_number_offset,
        .y = end_caret_pos.y - extra_padding - scroll_offset.y,
    };
    rect_renderer.addRect(caret_pos + position, {caret_width, caret_height}, {95, 180, 180, 255});
}

void TextViewWidget::scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) {
    renderer::TextRenderer& text_renderer = renderer::g_renderer->getTextRenderer();
    int max_y = buffer.lineCount() * text_renderer.lineHeight();

    // scroll_offset.x += delta.x;
    scroll_offset.y += delta.y;
    scroll_offset.y = std::clamp(scroll_offset.y, 0, max_y);
}

void TextViewWidget::leftMouseDown(const renderer::Point& mouse_pos) {
    std::cerr << "TextViewWidget::leftMouseDown()\n";

    renderer::Movement& movement = renderer::g_renderer->getMovement();

    // TODO: Add this to parameters.
    int line_number_offset = 100;

    renderer::Point new_coords = mouse_pos - position + scroll_offset;
    new_coords.x -= line_number_offset;

    movement.setCaretInfo(buffer, new_coords, end_caret);
    start_caret = end_caret;
}

void TextViewWidget::leftMouseDrag(const renderer::Point& mouse_pos) {
    renderer::Movement& movement = renderer::g_renderer->getMovement();

    // TODO: Add this to parameters.
    int line_number_offset = 100;

    renderer::Point new_coords = mouse_pos - position + scroll_offset;
    new_coords.x -= line_number_offset;

    movement.setCaretInfo(buffer, new_coords, end_caret);
}

void TextViewWidget::setContents(const std::string& text) {
    buffer.setContents(text);
}

}

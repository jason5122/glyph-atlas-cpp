#include "gui/renderer/renderer.h"
#include "label_widget.h"

namespace gui {

LabelWidget::LabelWidget(const Size& size, int left_padding, int right_padding)
    : Widget{size}, left_padding{left_padding}, right_padding{right_padding} {}

void LabelWidget::setText(const std::string& str8, const Rgb& color) {
    GlyphCache& ui_glyph_cache = Renderer::instance().getUiGlyphCache();

    label_line_layout = ui_glyph_cache.rasterizer().layoutLine(str8);
    this->color = color;
}

void LabelWidget::addLeftIcon(size_t icon_id) {
    left_side_icons.emplace_back(icon_id);
}

void LabelWidget::addRightIcon(size_t icon_id) {
    right_side_icons.emplace_back(icon_id);
}

void LabelWidget::draw() {
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    ImageRenderer& image_renderer = Renderer::instance().getImageRenderer();

    constexpr Rgba temp_color{223, 227, 230, 255};
    constexpr Rgba folder_icon_color{142, 142, 142, 255};

    // rect_renderer.addRect(position, size, temp_color);

    // Draw all left side icons.
    Point left_offset{.x = left_padding};
    for (size_t icon_id : left_side_icons) {
        Size image_size = image_renderer.getImageSize(icon_id);

        Point icon_position = centerVertically(image_size.height) + left_offset;
        image_renderer.addImage(icon_id, icon_position, folder_icon_color);

        left_offset.x += image_size.width;
    }

    // Draw all right side icons.
    Point right_offset{.x = right_padding};
    for (size_t icon_id : right_side_icons) {
        Size image_size = image_renderer.getImageSize(icon_id);

        right_offset.x += image_size.width;

        Point icon_position = centerVertically(image_size.height) - right_offset;
        icon_position += Point{size.width, 0};
        image_renderer.addImage(icon_id, icon_position, folder_icon_color);
    }

    Point text_position = centerVertically(text_renderer.uiLineHeight()) + left_offset;
    text_renderer.addUiText(text_position, color, label_line_layout);
}

Point LabelWidget::centerVertically(int widget_height) {
    Point centered_point = position;
    centered_point.y += size.height / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}

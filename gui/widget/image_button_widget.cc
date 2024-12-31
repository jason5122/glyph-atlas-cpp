#include "image_button_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

ImageButtonWidget::ImageButtonWidget(size_t image_id,
                                     const Rgba& text_color,
                                     const Rgba& bg_color,
                                     int padding)
    : image_id(image_id), text_color(text_color), bg_color(bg_color), padding(padding) {
    const auto& glyph_cache = gui::Renderer::instance().getGlyphCache();
    auto& image = glyph_cache.getImage(image_id);

    size = image.size;
    size += {padding * 2, padding * 2};
}

void ImageButtonWidget::draw() {
    // TODO: Formalize this.
    if (getState() && bg_color.a > 0) {
        auto& rect_renderer = Renderer::instance().getRectRenderer();
        rect_renderer.addRect(position, size, bg_color, Layer::kTwo, 4);
    }

    auto pos = position;
    pos += {padding, padding};

    auto& image_renderer = Renderer::instance().getImageRenderer();
    image_renderer.insertInBatch(image_id, pos, text_color);
}

}  // namespace gui

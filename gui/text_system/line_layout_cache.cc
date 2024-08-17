#include "gui/renderer/renderer.h"
#include "line_layout_cache.h"

namespace gui {

const font::LineLayout& LineLayoutCache::getLineLayout(std::string_view str8) {
    static constexpr XXH64_hash_t seed = 0;
    XXH64_hash_t hash = XXH64(str8.data(), str8.length(), seed);

    if (auto it = cache.find(hash); it != cache.end()) {
        return it->second;
    } else {
        const auto& glyph_cache = Renderer::instance().getGlyphCache();
        const auto& font_rasterizer = font::FontRasterizer::instance();

        auto layout = font_rasterizer.layoutLine(glyph_cache.mainFontId(), str8);
        auto inserted = cache.emplace(hash, std::move(layout));

        // TODO: Track max width in a better way.
        max_width = std::max(layout.width, max_width);

        return inserted.first->second;
    }
}

int LineLayoutCache::maxWidth() const {
    return max_width;
}

}

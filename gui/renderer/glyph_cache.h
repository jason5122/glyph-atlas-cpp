#pragma once

#include "font/font_rasterizer.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/opengl_types.h"
#include "third_party/emhash/hash_table5.hpp"
#include <unordered_map>
#include <vector>

namespace gui {

class GlyphCache {
public:
    GlyphCache();

    struct Glyph {
        GLuint tex_id;
        Vec4 glyph;
        Vec4 uv;
        bool colored;
        size_t page;
    };

    const Glyph& getGlyph(size_t font_id,
                          uint32_t glyph_id,
                          const font::FontRasterizer& font_rasterizer);

    void setMainFontId(size_t font_id);
    void setUIFontId(size_t font_id);
    size_t mainFontId() const;
    size_t uiFontId() const;
    const std::vector<Atlas>& atlasPages() const;

private:
    size_t main_font_id;
    size_t ui_font_id;

    std::vector<Atlas> atlas_pages;
    size_t current_page = 0;

    std::vector<emhash5::HashMap<uint32_t, Glyph>> cache;

    Glyph loadGlyph(const font::RasterizedGlyph& rglyph);
};

}  // namespace gui

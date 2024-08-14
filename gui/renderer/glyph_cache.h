#pragma once

#include "font/font_rasterizer.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/opengl_types.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace gui {

class GlyphCache {
public:
    GlyphCache(const std::string& main_font_name_utf8,
               int main_font_size,
               const std::string& ui_font_name_utf8,
               int ui_font_size);

    struct Glyph {
        GLuint tex_id;
        Vec4 glyph;
        Vec4 uv;
        int32_t advance;
        bool colored;
        size_t page;
    };

    Glyph& getMainGlyph(size_t font_id, uint32_t glyph_id);
    Glyph& getUIGlyph(size_t font_id, uint32_t glyph_id);
    int mainLineHeight() const;
    int uiLineHeight() const;

    // TODO: Refactor FontRasterizer out of GlyphCache.
    const font::FontRasterizer& mainRasterizer() const;
    const font::FontRasterizer& uiRasterizer() const;

    // TODO: Make this private.
    std::vector<Atlas> atlas_pages;

private:
    font::FontRasterizer main_font_rasterizer;
    font::FontRasterizer ui_font_rasterizer;

    // std::vector<Atlas> atlas_pages;
    size_t current_page = 0;

    std::unordered_map<size_t, std::unordered_map<uint32_t, Glyph>> main_cache;
    std::unordered_map<size_t, std::unordered_map<uint32_t, Glyph>> ui_cache;

    Glyph loadGlyph(const font::RasterizedGlyph& rglyph);
};

}

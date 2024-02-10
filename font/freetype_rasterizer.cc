#include "freetype_rasterizer.h"
#include <iostream>
#include <vector>

#include <ft2build.h>

bool FreeTypeRasterizer::setup(const char* font_path) {
    FT_Error error;

    FT_Library ft;
    error = FT_Init_FreeType(&ft);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_Init_FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    error = FT_New_Face(ft, font_path, 0, &face);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_New_Face error: " << FT_Error_String(error) << '\n';
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 32);

    float ascent = std::round(static_cast<float>(face->ascender) / 64);
    float descent = std::round(static_cast<float>(face->descender) / 64);
    float glyph_height = std::round(static_cast<float>(face->height) / 64);

    // FIXME: This is a hack to match Core Text's metrics.
    ascent *= 2;
    descent *= 2;
    glyph_height *= 2;

    float global_glyph_height = ascent - descent;

    this->line_height = std::max(glyph_height, global_glyph_height) + 2;
    this->descent = descent;

    hb_blob_t* blob = hb_blob_create_from_file(font_path);
    hb_face_t* face = hb_face_create(blob, 0);
    harfbuzz_font = hb_font_create(face);

    hb_face_destroy(face);
    hb_blob_destroy(blob);

    return true;
}

hb_codepoint_t FreeTypeRasterizer::getGlyphIndex(const char* utf8_str) {
    hb_buffer_t* buf;
    buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, utf8_str, -1, 0, -1);

    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));

    hb_shape(harfbuzz_font, buf, NULL, 0);

    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);

    hb_codepoint_t glyph_index = 0;
    for (unsigned int i = 0; i < glyph_count; i++) {
        glyph_index = glyph_info[i].codepoint;
        break;
    }

    hb_buffer_destroy(buf);
    return glyph_index;
}

RasterizedGlyph FreeTypeRasterizer::rasterizeUTF8(const char* utf8_str) {
    auto t1 = std::chrono::high_resolution_clock::now();

    FT_Error error;

    hb_codepoint_t glyph_index = this->getGlyphIndex(utf8_str);

    if (glyph_index == 0) {
        // std::cerr << "glyph_index is 0 for " << utf8_str << '\n';
    } else {
        std::cerr << "glyph_index is NOT 0 for " << utf8_str << '\n';
    }

    // TODO: Handle errors.
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_COLOR);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_Load_Glyph error: " << FT_Error_String(error) << '\n';
    }

    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_Render_Glyph error: " << FT_Error_String(error) << '\n';
    }

    FT_Bitmap bitmap = face->glyph->bitmap;
    unsigned char* buffer = bitmap.buffer;
    unsigned int rows = bitmap.rows;
    unsigned int width = bitmap.width;
    int pitch = bitmap.pitch;
    FT_Pixel_Mode pixel_mode = static_cast<FT_Pixel_Mode>(bitmap.pixel_mode);

    bool colored = false;

    std::vector<uint8_t> packed_buffer;
    if (pixel_mode == FT_PIXEL_MODE_GRAY) {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < width; j++) {
                unsigned char pixel_brightness = buffer[i * pitch + j];
                packed_buffer.push_back(pixel_brightness);
                packed_buffer.push_back(pixel_brightness);
                packed_buffer.push_back(pixel_brightness);
            }
        }
    } else if (pixel_mode == FT_PIXEL_MODE_BGRA) {
        size_t pixels = rows * width * 4;
        packed_buffer.reserve(pixels);
        for (size_t i = 0; i < pixels; i += 4) {
            packed_buffer.push_back(buffer[i + 2]);
            packed_buffer.push_back(buffer[i + 1]);
            packed_buffer.push_back(buffer[i]);
            packed_buffer.push_back(buffer[i + 3]);
        }

        colored = true;
    }
    // TODO: Support more pixel bitmap modes.
    else {
        std::cerr << "Pixel bitmap is unsupported! " << pixel_mode << '\n';
    }

    float advance = static_cast<float>(face->glyph->advance.x) / 64;
    int32_t top = face->glyph->bitmap_top;

    // TODO: Apply this transformation in glyph atlas, not in rasterizer.
    top -= descent;

    fprintf(stderr, "%d %d %d %d\n", face->glyph->bitmap_left, top, static_cast<int32_t>(width),
            static_cast<int32_t>(rows));

    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    fprintf(stderr, "FreeType rasterize: %lld µs\n", duration);

    return RasterizedGlyph{
        colored,
        face->glyph->bitmap_left,
        top,
        static_cast<int32_t>(width),
        static_cast<int32_t>(rows),
        advance,
        packed_buffer,
    };
}

FreeTypeRasterizer::~FreeTypeRasterizer() {
    hb_font_destroy(harfbuzz_font);
}

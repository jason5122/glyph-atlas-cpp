#include "base/rgb.h"
#include "text_renderer.h"
#include "ui/renderer/opengl_types.h"
#include "util/file_util.h"
#include "util/opengl_error_util.h"
#include <cmath>
#include <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

struct InstanceData {
    Vec2 coords;
    Vec2 bg_size;
    Vec4 glyph;
    Vec4 uv;
    Rgba color;
    Rgba bg_color;
    uint8_t is_atlas = 0;
};

extern "C" TSLanguage* tree_sitter_cpp();
extern "C" TSLanguage* tree_sitter_glsl();
extern "C" TSLanguage* tree_sitter_json();
extern "C" TSLanguage* tree_sitter_scheme();

void TextRenderer::setup(float width, float height, std::string main_font_name, int font_size) {
    fs::path font_path = ResourcePath() / "fonts/SourceCodePro-Regular.ttf";

    atlas.setup();
    // ct_rasterizer.setup(main_font_name, font_size);
    ft_rasterizer.setup(font_path.c_str(), font_size);
    highlighter.setLanguage("source.scheme");

    // this->line_height = ct_rasterizer.line_height;
    this->line_height = ft_rasterizer.line_height;

    // std::cerr << "ct_rasterizer = " << ct_rasterizer.line_height << '\n';
    std::cerr << "ft_rasterizer = " << ft_rasterizer.line_height << '\n';

    shader_program.link(ResourcePath() / "shaders/text_vert.glsl",
                        ResourcePath() / "shaders/text_frag.glsl");
    this->resize(width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);

    glUseProgram(shader_program.id);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_height"), line_height);

    GLuint indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3,  // second triangle
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * BATCH_MAX, nullptr, GL_STATIC_DRAW);

    GLuint index = 0;

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, coords));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, glyph));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, uv));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, is_atlas));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

const char* read(void* payload, uint32_t byte_index, TSPoint position, uint32_t* bytes_read) {
    Buffer* buffer = (Buffer*)payload;
    if (position.row >= buffer->lineCount()) {
        *bytes_read = 0;
        return "";
    }

    const size_t BUFSIZE = 256;
    static char buf[BUFSIZE];

    std::string line_str;
    buffer->getLineContent(&line_str, position.row);

    size_t len = line_str.size();
    size_t bytes_copied = std::min(len - position.column, BUFSIZE);

    memcpy(buf, &line_str[0] + position.column, bytes_copied);
    *bytes_read = (uint32_t)bytes_copied;
    if (bytes_copied < BUFSIZE) {
        // Add the final \n.
        // If it didn't fit, read() will be called again on the same line with the column advanced.
        buf[bytes_copied] = '\n';
        (*bytes_read)++;
    }
    return buf;
}

void TextRenderer::parseBuffer(Buffer& buffer) {
    TSInput input = {&buffer, read, TSInputEncodingUTF8};
    highlighter.parse(input);
    highlighter.getHighlights();
}

void TextRenderer::editBuffer(Buffer& buffer, size_t bytes) {
    size_t start_byte = buffer.byteOfLine(cursor_end_line) + cursor_end_col_offset;
    size_t old_end_byte = buffer.byteOfLine(cursor_end_line) + cursor_end_col_offset;
    size_t new_end_byte = buffer.byteOfLine(cursor_end_line) + cursor_end_col_offset + bytes;
    highlighter.edit(start_byte, old_end_byte, new_end_byte);
}

float TextRenderer::getGlyphAdvance(std::string utf8_str) {
    if (!glyph_cache.count(utf8_str)) {
        this->loadGlyph(utf8_str);
    }

    AtlasGlyph glyph = glyph_cache[utf8_str];
    return std::round(glyph.advance);
}

std::pair<float, size_t> TextRenderer::closestBoundaryForX(std::string line_str, float x) {
    size_t offset;
    size_t ret;
    float total_advance = 0;
    for (offset = 0; offset < line_str.size(); offset += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

        std::string utf8_str;
        for (size_t i = 0; i < ret; i++) {
            utf8_str += line_str[offset + i];
        }

        if (!glyph_cache.count(utf8_str)) {
            this->loadGlyph(utf8_str);
        }

        AtlasGlyph glyph = glyph_cache[utf8_str];

        float glyph_center = total_advance + glyph.advance / 2;
        if (glyph_center >= x) {
            return {total_advance, offset};
        }

        total_advance += std::round(glyph.advance);
    }
    return {total_advance, offset};
}

void TextRenderer::renderText(Buffer& buffer, float scroll_x, float scroll_y) {
    glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll_x, scroll_y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;
    int range_idx = 0;

    size_t scroll_line = scroll_y / line_height;

    size_t visible_lines = std::ceil((height - 60 - 40) / line_height);
    size_t byte_offset = buffer.byteOfLine(scroll_line);
    size_t size = std::min(static_cast<size_t>(scroll_line + visible_lines), buffer.lineCount());
    for (size_t line_index = scroll_line; line_index < size; line_index++) {
        std::string line_str;
        buffer.getLineContent(&line_str, line_index);

        size_t ret;
        float total_advance = 0;
        for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

            Rgb text_color = BLACK;

            if (range_idx < highlighter.highlight_ranges.size()) {
                while (byte_offset >= highlighter.highlight_ranges[range_idx].second) {
                    range_idx++;
                }

                if (highlighter.highlight_ranges[range_idx].first <= byte_offset &&
                    byte_offset < highlighter.highlight_ranges[range_idx].second) {
                    text_color = highlighter.highlight_colors[range_idx];
                }
            }

            std::string utf8_str;
            for (size_t i = 0; i < ret; i++) {
                utf8_str += line_str[offset + i];
            }

            if (!glyph_cache.count(utf8_str)) {
                this->loadGlyph(utf8_str);
            }

            AtlasGlyph glyph = glyph_cache[utf8_str];

            float glyph_center_x = total_advance + glyph.advance / 2;
            uint8_t bg_a = this->isGlyphInSelection(line_index, glyph_center_x) ? 255 : 0;

            std::cerr << "glyph: " << glyph.glyph << '\n';
            std::cerr << "uv: " << glyph.uv << '\n';

            instances.push_back(InstanceData{
                .coords = Vec2{total_advance, line_index * line_height},
                .bg_size = Vec2{glyph.advance, line_height},
                .glyph = glyph.glyph,
                .uv = glyph.uv,
                .color = Rgba::fromRgb(text_color, glyph.colored),
                .bg_color = Rgba::fromRgb(YELLOW, bg_a),
            });

            total_advance += std::round(glyph.advance);
        }
        byte_offset++;
        longest_line_x = std::max(total_advance, longest_line_x);
    }

    // instances.push_back(InstanceData{
    //     .coords = Vec2{width - Atlas::ATLAS_SIZE - 400, 10 * line_height},
    //     .bg_size = Vec2{Atlas::ATLAS_SIZE, Atlas::ATLAS_SIZE},
    //     .glyph = Vec4{0, 0, Atlas::ATLAS_SIZE, Atlas::ATLAS_SIZE},
    //     .uv = Vec4{0, 0, 1.0, 1.0},
    //     .color = Rgba::fromRgb(BLACK, false),
    //     .bg_color = Rgba::fromRgb(YELLOW, 255),
    //     .is_atlas = true,
    // });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glUniform1i(glGetUniformLocation(shader_program.id, "rendering_pass"), 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());
    glUniform1i(glGetUniformLocation(shader_program.id, "rendering_pass"), 1);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}

bool TextRenderer::isGlyphInSelection(int row, float glyph_center_x) {
    int start_row, end_row;
    float start_x, end_x;

    if (cursor_start_line == cursor_end_line) {
        if (cursor_start_x <= cursor_end_x) {
            start_row = cursor_start_line;
            end_row = cursor_end_line;
            start_x = cursor_start_x;
            end_x = cursor_end_x;
        } else {
            start_row = cursor_end_line;
            end_row = cursor_start_line;
            start_x = cursor_end_x;
            end_x = cursor_start_x;
        }
    } else if (cursor_start_line < cursor_end_line) {
        start_row = cursor_start_line;
        end_row = cursor_end_line;
        start_x = cursor_start_x;
        end_x = cursor_end_x;
    } else {
        start_row = cursor_end_line;
        end_row = cursor_start_line;
        start_x = cursor_end_x;
        end_x = cursor_start_x;
    }

    if (start_row < row && row < end_row) {
        return true;
    }
    if (start_row == end_row) {
        if (row == start_row && start_x <= glyph_center_x && glyph_center_x <= end_x) {
            return true;
        }
    } else {
        if (row == start_row && start_x <= glyph_center_x) {
            return true;
        }
        if (row == end_row && glyph_center_x <= end_x) {
            return true;
        }
    }
    return false;
}

void TextRenderer::setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                                      float drag_y) {
    float x;
    size_t offset;

    cursor_start_line = cursor_y / line_height;
    if (cursor_start_line > buffer.lineCount()) cursor_start_line = buffer.lineCount();

    std::string start_line_str;
    buffer.getLineContent(&start_line_str, cursor_start_line);
    std::tie(x, offset) = this->closestBoundaryForX(start_line_str, cursor_x);
    cursor_start_col_offset = offset;
    cursor_start_x = x;

    cursor_end_line = drag_y / line_height;
    if (cursor_end_line > buffer.lineCount()) cursor_end_line = buffer.lineCount();

    std::string end_line_str;
    buffer.getLineContent(&end_line_str, cursor_end_line);
    std::tie(x, offset) = this->closestBoundaryForX(end_line_str, drag_x);
    cursor_end_col_offset = offset;
    cursor_end_x = x;
}

void TextRenderer::loadGlyph(std::string utf8_str) {
    // RasterizedGlyph glyph = ct_rasterizer.rasterizeUTF8(utf8_str.c_str());
    RasterizedGlyph glyph = ft_rasterizer.rasterizeUTF8(utf8_str.c_str());
    AtlasGlyph atlas_glyph = atlas.insertGlyph(glyph);
    glyph_cache.insert({utf8_str, atlas_glyph});
}

void TextRenderer::resize(float new_width, float new_height) {
    width = new_width;
    height = new_height;

    glViewport(0, 0, width, height);
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), width, height);
}

TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

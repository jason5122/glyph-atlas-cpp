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

extern "C" TSLanguage* tree_sitter_cpp();
extern "C" TSLanguage* tree_sitter_glsl();
extern "C" TSLanguage* tree_sitter_json();
extern "C" TSLanguage* tree_sitter_scheme();

void TextRenderer::setup(float width, float height, std::string main_font_name, int font_size) {
    fs::path font_path = ResourcePath() / "fonts/SourceCodePro-Regular.ttf";

    atlas.setup();
    ct_rasterizer.setup(main_font_name, font_size);

    this->line_height = ct_rasterizer.line_height;

    shader_program.link(ResourcePath() / "shaders/text_vert.glsl",
                        ResourcePath() / "shaders/text_frag.glsl");
    this->resize(width, height);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(RendererInstanceData) * BATCH_MAX, nullptr,
                 GL_STATIC_DRAW);

    GLuint index = 0;

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(RendererInstanceData),
                          (void*)offsetof(RendererInstanceData, coords));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(RendererInstanceData),
                          (void*)offsetof(RendererInstanceData, bg_size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(RendererInstanceData),
                          (void*)offsetof(RendererInstanceData, glyph));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(RendererInstanceData),
                          (void*)offsetof(RendererInstanceData, uv));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(RendererInstanceData),
                          (void*)offsetof(RendererInstanceData, color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(RendererInstanceData),
                          (void*)offsetof(RendererInstanceData, bg_color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(RendererInstanceData),
                          (void*)offsetof(RendererInstanceData, is_atlas));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    for (char ch = 'A'; ch < 'z'; ch++) {
        std::string str(1, ch);
        this->loadGlyph(str);
    }
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

void TextRenderer::layoutText(Buffer& buffer) {
    layout_instances.clear();
    line_to_instance_mapping.clear();

    size_t byte_offset = 0;
    size_t instance = 0;
    for (size_t line_index = 0; line_index < buffer.lineCount(); line_index++) {
        std::string line_str;
        buffer.getLineContent(&line_str, line_index);

        line_to_instance_mapping.push_back(instance);

        size_t ret;
        float total_advance = 0;
        for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

            Rgb text_color = BLACK;

            std::string utf8_str(line_str.substr(offset, ret));

            if (!glyph_cache.count(utf8_str)) {
                this->loadGlyph(utf8_str);
            }

            AtlasGlyph glyph = glyph_cache[utf8_str];

            float glyph_center_x = total_advance + glyph.advance / 2;
            uint8_t bg_a = this->isGlyphInSelection(line_index, glyph_center_x) ? 255 : 0;

            layout_instances.push_back(RendererInstanceData{
                .coords = Vec2{total_advance, line_index * line_height},
                .bg_size = Vec2{glyph.advance, line_height},
                .glyph = glyph.glyph,
                .uv = glyph.uv,
                .color = Rgba::fromRgb(text_color, glyph.colored),
                .bg_color = Rgba::fromRgb(YELLOW, bg_a),
                .offset = byte_offset,
            });
            instance++;

            total_advance += std::round(glyph.advance);
        }
        byte_offset++;
        longest_line_x = std::max(total_advance, longest_line_x);
    }

    fprintf(stderr, "layout_instances.size() = %zu\n", layout_instances.size());
    fprintf(stderr, "line_to_instance_mapping.size() = %zu\n", line_to_instance_mapping.size());
}

void TextRenderer::renderText(float scroll_x, float scroll_y, SyntaxHighlighter& highlighter) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll_x, scroll_y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    size_t start_line = scroll_y / line_height;
    size_t visible_lines = std::ceil((height - 60) / line_height);
    size_t end_line = std::min(start_line + visible_lines, line_to_instance_mapping.size() - 1);

    size_t start_offset = line_to_instance_mapping.at(start_line);
    size_t end_offset = line_to_instance_mapping.at(end_line);
    size_t num_instances = end_offset - start_offset;

    fprintf(stderr, "start_line = %zu, visible_lines = %zu\n", start_line, visible_lines);
    fprintf(stderr, "num_instances = %zu\n", num_instances);

    highlighter.idx = 0;
    highlighter.getHighlights(0, 1000);

    std::vector<RendererInstanceData> instances;
    for (size_t i = start_offset; i < end_offset; i++) {
        RendererInstanceData layout_instance = layout_instances[i];

        Rgb text_color = BLACK;
        if (highlighter.isByteOffsetInRange(layout_instance.offset)) {
            text_color = highlighter.highlight_colors[highlighter.idx];
        }
        layout_instance.color = Rgba::fromRgb(text_color, 0);

        instances.push_back(layout_instance);
    }

    instances.push_back(RendererInstanceData{
        .coords = Vec2{width - Atlas::ATLAS_SIZE - 400 + scroll_x, 10 * line_height + scroll_y},
        .bg_size = Vec2{Atlas::ATLAS_SIZE, Atlas::ATLAS_SIZE},
        .glyph = Vec4{0, 0, Atlas::ATLAS_SIZE, Atlas::ATLAS_SIZE},
        .uv = Vec4{0, 0, 1.0, 1.0},
        .color = Rgba::fromRgb(BLACK, false),
        .bg_color = Rgba::fromRgb(YELLOW, 255),
        .is_atlas = true,
    });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(RendererInstanceData) * instances.size(),
                    &instances[0]);

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
    if (cursor_start_line > buffer.lineCount()) {
        std::cerr << "cursor went past last line\n";
        cursor_start_line = buffer.lineCount() - 1;
    }

    std::string start_line_str;
    buffer.getLineContent(&start_line_str, cursor_start_line);
    std::tie(x, offset) = this->closestBoundaryForX(start_line_str, cursor_x);
    cursor_start_col_offset = offset;
    cursor_start_x = x;

    cursor_end_line = drag_y / line_height;
    if (cursor_end_line > buffer.lineCount()) {
        std::cerr << "cursor went past last line\n";
        cursor_end_line = buffer.lineCount() - 1;
    }

    std::string end_line_str;
    buffer.getLineContent(&end_line_str, cursor_end_line);
    std::tie(x, offset) = this->closestBoundaryForX(end_line_str, drag_x);
    cursor_end_col_offset = offset;
    cursor_end_x = x;
}

void TextRenderer::loadGlyph(std::string utf8_str) {
    RasterizedGlyph glyph = ct_rasterizer.rasterizeUTF8(utf8_str.c_str());
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

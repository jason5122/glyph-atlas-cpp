#include "base/rgb.h"
#include "renderer/opengl_error_util.h"
#include "selection_renderer.h"
#include <cstdint>

#include "build/buildflag.h"

namespace renderer {

SelectionRenderer::SelectionRenderer() {}

SelectionRenderer::~SelectionRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

void SelectionRenderer::setup() {
    std::string vert_source =
#include "renderer/shaders/selection_vert.glsl"
        ;
    std::string frag_source =
#include "renderer/shaders/selection_frag.glsl"
        ;

    shader_program.link(vert_source, frag_source);

    glUseProgram(shader_program.id);
    glUniform1i(glGetUniformLocation(shader_program.id, "r"), kCornerRadius);
    glUniform1i(glGetUniformLocation(shader_program.id, "thickness"), kBorderThickness);

    GLuint indices[] = {
        0, 1, 3,  // First triangle.
        1, 2, 3,  // Second triangle.
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * kBatchMax, nullptr, GL_STREAM_DRAW);

    GLuint index = 0;

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, coords));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, border_color));
    glVertexAttribDivisor(index++, 1);

    // To prevent OpenGL from casting our ints to floats, we need to use either GL_FLOAT or
    // glVertexAttribIPointer().
    // https://stackoverflow.com/a/55972160
    glEnableVertexAttribArray(index);
    glVertexAttribIPointer(index, 4, GL_INT, sizeof(InstanceData),
                           (void*)offsetof(InstanceData, border_info));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SelectionRenderer::createInstances(Size& size,
                                        Point& scroll,
                                        Point& editor_offset,
                                        renderer::GlyphCache& main_glyph_cache,
                                        std::vector<Selection>& selections,
                                        int line_number_offset) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset.x,
                editor_offset.y);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_number_offset"), line_number_offset);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    auto create =
        [this, &main_glyph_cache](int start, int end, int line,
                                  uint32_t border_flags = kLeft | kRight | kTop | kBottom,
                                  uint32_t bottom_border_offset = 0,
                                  uint32_t top_border_offset = 0, uint32_t hide_background = 0) {
            instances.emplace_back(InstanceData{
                .coords =
                    {
                        .x = static_cast<float>(start),
                        .y = static_cast<float>(main_glyph_cache.lineHeight() * line),
                    },
                .size =
                    {
                        .x = static_cast<float>(end - start),
                        .y = static_cast<float>(main_glyph_cache.lineHeight() + kBorderThickness),
                    },
                .color = Rgba::fromRgb(base::colors::selection_focused, 0),
                .border_color = Rgba::fromRgb(base::colors::selection_border, 0),
                // .border_color = Rgba::fromRgb(base::colors::red, 0),
                // .color = Rgba::fromRgb(base::colors::yellow, 0),
                // .border_color = Rgba::fromRgb(base::Rgb{0, 0, 0}, 0),
                .border_info =
                    IVec4{
                        .x = border_flags,
                        .y = bottom_border_offset,
                        .z = top_border_offset,
                        .w = hide_background,
                    },
            });
        };

    size_t selections_size = selections.size();
    for (size_t i = 0; i < selections_size; i++) {
        uint32_t flags = kLeft | kRight;
        uint32_t bottom_border_offset = 0;
        uint32_t top_border_offset = 0;

        if (i == 0) {
            flags |= kTop | kTopLeftInwards | kTopRightInwards;

            if (selections_size > 1 && selections[i].start > 0) {
                int end = selections[i].start;
                if (selections[i + 1].end >= selections[i].start) {
                    end -= kCornerRadius + 2;
                }
                create(2, end, selections[i].line, kBottom, 0, 0, 1);
            }
        }
        if (i == selections_size - 1) {
            flags |= kBottom | kBottomLeftInwards | kBottomRightInwards;
        }

        if (i > 0) {
            if (selections[i - 1].start > 0) {
                flags |= kTopLeftInwards;
            }

            if (selections[i].end > selections[i - 1].end) {
                flags |= kTopRightInwards;

                flags |= kTop;
                top_border_offset = selections[i - 1].end;
            } else if (selections[i].end < selections[i - 1].end) {
                flags |= kTopRightOutwards;
            }
        }
        if (i + 1 < selections_size) {
            if (selections[i].start > selections[i + 1].start) {
                flags |= kBottomLeftOutwards;
            }

            if (selections[i].end > selections[i + 1].end) {
                flags |= kBottomRightInwards;

                flags |= kBottom;
                bottom_border_offset = selections[i + 1].end - selections[i].start;
            } else if (selections[i].end < selections[i + 1].end) {
                flags |= kBottomRightOutwards;
            }
        }

        create(selections[i].start, selections[i].end, selections[i].line, flags,
               bottom_border_offset, top_border_offset);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}

void SelectionRenderer::render(int rendering_pass) {
    glUseProgram(shader_program.id);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    glUniform1i(glGetUniformLocation(shader_program.id, "rendering_pass"), rendering_pass);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    glCheckError();
}

void SelectionRenderer::destroyInstances() {
    instances.clear();
}

}

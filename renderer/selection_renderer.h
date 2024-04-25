#pragma once

#include "font/rasterizer.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include <glad/glad.h>

namespace renderer {
class SelectionRenderer {
public:
    SelectionRenderer();
    ~SelectionRenderer();
    void setup(FontRasterizer& font_rasterizer);
    void render(Size& size, Point& scroll, Point& editor_offset, FontRasterizer& font_rasterizer);

private:
    static const int kBatchMax = 65536;
    static const int kTabCornerRadius = 6;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;
};
}

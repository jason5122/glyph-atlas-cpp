#import "Renderer.h"
#import "model/Font.h"
#import "model/Rasterizer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"

#include <ft2build.h>
#include FT_FREETYPE_H

Renderer::Renderer(float width, float height) : width(width), height(height) {}

void Renderer::init() {
    setup_shaders();

    glm::mat4 projection = glm::ortho(0.0f, width, 0.0f, height);
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));

    bool success = load_glyphs();
    if (!success) {
        logDefault(@"Renderer", @"error loading font glyphs");
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::setup_shaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vertSource = readFile(resourcePath("text_vert.glsl"));
    const GLchar* fragSource = readFile(resourcePath("text_frag.glsl"));
    glShaderSource(vertexShader, 1, &vertSource, nullptr);
    glShaderSource(fragmentShader, 1, &fragSource, nullptr);
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

bool Renderer::load_glyphs() {
    FT_Error error;

    FT_Library ft;
    error = FT_Init_FreeType(&ft);
    if (error != FT_Err_Ok) {
        logError(@"Renderer", @"%s", FT_Error_String(error));
        return false;
    }

    FT_Face face;
    error = FT_New_Face(ft, resourcePath("SourceCodePro-Regular.ttf"), 0, &face);
    if (error != FT_Err_Ok) {
        logError(@"Renderer", @"%s", FT_Error_String(error));
        return false;
    }

    Font menlo = Font(CFSTR("Menlo"), 48);
    Metrics metrics = menlo.metrics();
    Rasterizer rasterizer = Rasterizer();
    logDefault(@"Renderer", @"average_advance = %f, line_height = %f", metrics.average_advance,
               metrics.line_height);

    float cell_width = floor(metrics.average_advance + 1);
    float cell_height = floor(metrics.line_height + 2);
    logDefault(@"Renderer", @"cell_width = %f, cell_height = %f", cell_width, cell_height);

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char ch = 'a'; ch <= 'c'; ch++) {
        CGGlyph glyph_index = menlo.get_glyph([NSString stringWithFormat:@"%c", ch]);
        RasterizedGlyph rasterized_glyph = rasterizer.rasterize_glyph(glyph_index);

        error = FT_Load_Char(face, ch, FT_LOAD_RENDER);
        if (error != FT_Err_Ok) {
            logError(@"Renderer", @"%s", FT_Error_String(error));
            return false;
        }

        GLsizei width = face->glyph->bitmap.width;
        GLsizei height = face->glyph->bitmap.rows;
        GLsizei top = face->glyph->bitmap_top;
        GLsizei left = face->glyph->bitmap_left;
        // GLsizei advance = face->glyph->advance.x >> 6;
        const void* buffer = face->glyph->bitmap.buffer;
        // GLsizei width = rasterized_glyph.width;
        // GLsizei height = rasterized_glyph.height;
        // GLsizei top = rasterized_glyph.top;
        // GLsizei left = rasterized_glyph.left;
        GLsizei advance = metrics.average_advance + 1;
        // const void* buffer = &rasterized_glyph.buffer;

        if (face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
            logDefault(@"Renderer", @"correct pixel mode");
        } else {
            logError(@"Renderer", @"wrong pixel mode!");
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        //              buffer);

        logDefault(@"Renderer", @"%dx%d versus %dx%d", width, height, rasterized_glyph.width,
                   rasterized_glyph.height);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {texture, glm::ivec2(width, height), glm::ivec2(left, top), advance};
        characters.insert({ch, character});
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

void Renderer::render_text(std::string text, float x, float y, float scale, glm::vec3 color) {
    glUseProgram(shaderProgram);
    glUniform3fv(glGetUniformLocation(shaderProgram, "textColor"), 1, glm::value_ptr(color));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    for (const char c : text) {
        Character ch = characters[c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},      //
            {xpos, ypos, 0.0f, 1.0f},          //
            {xpos + w, ypos, 1.0f, 1.0f},      //
            {xpos, ypos + h, 0.0f, 0.0f},      //
            {xpos + w, ypos, 1.0f, 1.0f},      //
            {xpos + w, ypos + h, 1.0f, 0.0f},  //
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.tex_id);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
                        vertices);  // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        logDefault(@"Renderer", @"advance = %f", (ch.advance >> 6) * scale);
        // x += (ch.advance >> 6) * scale;
        x += ch.advance;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Renderer::~Renderer() {
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

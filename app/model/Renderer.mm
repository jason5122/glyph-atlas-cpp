#import "Renderer.h"
#import "util/FileUtil.h"

#include <ft2build.h>
#include FT_FREETYPE_H

Renderer::Renderer() {}

bool Renderer::init() {
    FT_Library ft;
    FT_Init_FreeType(&ft);

    FT_Face face;
    FT_New_Face(ft, resourcePath("SourceCodePro-Regular.ttf"), 0, &face);

    FT_Set_Pixel_Sizes(face, 0, 48);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        // if (FT_Load_Char(face, c, FT_LOAD_RENDER)) return false;

        // unsigned int texture;
        // glGenTextures(1, &texture);
        // glBindTexture(GL_TEXTURE_2D, texture);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
        // face->glyph->bitmap.rows,
        //              0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Character character = {texture,
        //                        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        //                        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        //                        static_cast<unsigned int>(face->glyph->advance.x)};
        // characters.insert(std::pair<char, Character>(c, character));
    }
    // glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

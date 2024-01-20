#import "Renderer.h"
#import "model/Rasterizer.h"
#import "util/CTFontUtil.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import "util/OpenGLErrorUtil.h"

struct InstanceData {
    // grid_coords
    uint16_t col;
    uint16_t row;
    // glyph
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    // uv
    float uv_left;
    float uv_bot;
    float uv_width;
    float uv_height;
    // tex_coords
    float u;
    float v;
};

Renderer::Renderer(float width, float height, CTFontRef mainFont)
    : width(width), height(height), mainFont(mainFont) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // DEBUG: Draw shapes as wireframes.

    this->linkShaders();

    Metrics metrics = CTFontGetMetrics(mainFont);
    float cell_width = CGFloat_floor(metrics.average_advance + 1);
    float cell_height = CGFloat_floor(metrics.line_height + 2);

    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "resolution"), width, height);
    glUniform2f(glGetUniformLocation(shader_program, "cell_dim"), cell_width, cell_height);

    // Font experiments.
    CTFontRef appleEmojiFont = CTFontCreateWithName(CFSTR("Apple Color Emoji"), 16, nullptr);
    LogDefault(@"Renderer", @"colored? %d", CTFontIsColored(appleEmojiFont));

    NSDictionary* descriptorOptions = @{(id)kCTFontFamilyNameAttribute : @"Menlo"};
    CTFontDescriptorRef descriptor =
        CTFontDescriptorCreateWithAttributes((CFDictionaryRef)descriptorOptions);
    CFTypeRef keys[] = {kCTFontFamilyNameAttribute};
    CFSetRef mandatoryAttrs = CFSetCreate(kCFAllocatorDefault, keys, 1, &kCFTypeSetCallBacks);
    CFArrayRef fontDescriptors = CTFontDescriptorCreateMatchingFontDescriptors(descriptor, NULL);

    for (int i = 0; i < CFArrayGetCount(fontDescriptors); i++) {
        CTFontDescriptorRef descriptor =
            (CTFontDescriptorRef)CFArrayGetValueAtIndex(fontDescriptors, i);
        CFStringRef familyName =
            (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontFamilyNameAttribute);
        CFStringRef style =
            (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontStyleNameAttribute);
        LogDefault(@"Renderer", @"%@ %@", familyName, style);

        CTFontRef tempFont = CTFontCreateWithFontDescriptor(descriptor, 32, nullptr);
    }
    // End of font experiments.

    this->loadGlyphs();

    GLuint indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3,  // second triangle
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * BATCH_MAX, nullptr, GL_STATIC_DRAW);

    size_t size = 0;

    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)size);
    // glVertexAttribDivisor(0, 1);
    // size += 4 * sizeof(float);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(1, 1);
    size += 2 * sizeof(uint16_t);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_SHORT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(2, 1);
    size += 4 * sizeof(int16_t);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(3, 1);
    size += 4 * sizeof(float);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(4, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::renderText(std::string text, float x, float y) {
    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    AtlasGlyph glyph = glyph_cache[text[0]];
    float xpos = x + glyph.bearing.x;
    float ypos = y - (glyph.size.y - glyph.bearing.y);
    float w = glyph.size.x;
    float h = glyph.size.y;

    float vertices[4][4] = {
        {xpos + w, ypos + h, glyph.uv_width, 0.0f},         // bottom right
        {xpos + w, ypos, glyph.uv_width, glyph.uv_height},  // top right
        {xpos, ypos, 0.0f, glyph.uv_height},                // top left
        {xpos, ypos + h, 0.0f, 0.0f},                       // bottom left
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    std::vector<InstanceData> instances;
    for (uint16_t row = 0; row < text.size(); row++) {
        char ch = text[row];
        AtlasGlyph glyph = glyph_cache[ch];
        instances.push_back(InstanceData{row, 0, glyph.left, glyph.top, glyph.width, glyph.height,
                                         glyph.uv_left, glyph.uv_bot, glyph.uv_width,
                                         glyph.uv_height, glyph.uv_width, glyph.uv_height});
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // https://learnopengl.com/In-Practice/Debugging
    glPrintError();
}

void Renderer::loadGlyphs() {
    Rasterizer rasterizer = Rasterizer(mainFont);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &atlas);
    glBindTexture(GL_TEXTURE_2D, atlas);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_SIZE, ATLAS_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    int offset_x = 0;
    int offset_y = 0;
    for (char ch : std::string("Hello world!")) {
        CGGlyph glyph_index = CTFontGetGlyphIndex(mainFont, ch);
        RasterizedGlyph glyph = rasterizer.rasterizeGlyph(glyph_index);

        glBindTexture(GL_TEXTURE_2D, atlas);
        glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, glyph.width, glyph.height, GL_RGB,
                        GL_UNSIGNED_BYTE, &glyph.buffer[0]);

        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

        float uv_bot = offset_x;
        float uv_left = offset_y;
        float uv_width = static_cast<float>(glyph.width) / ATLAS_SIZE;
        float uv_height = static_cast<float>(glyph.height) / ATLAS_SIZE;

        AtlasGlyph atlas_glyph = {glm::ivec2(glyph.width, glyph.height),
                                  glm::ivec2(glyph.left, glyph.top),
                                  glyph.left,
                                  glyph.top,
                                  glyph.width,
                                  glyph.height,
                                  uv_bot,
                                  uv_left,
                                  uv_width,
                                  uv_height};
        glyph_cache.insert({ch, atlas_glyph});

        offset_x += glyph.width;
    }
}

void Renderer::clearAndResize() {
    glViewport(0, 0, width, height);
    glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::linkShaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = ReadFile(ResourcePath("text_vert.glsl"));
    const GLchar* frag_source = ReadFile(ResourcePath("text_frag.glsl"));
    glShaderSource(vertex_shader, 1, &vert_source, nullptr);
    glShaderSource(fragment_shader, 1, &frag_source, nullptr);
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader_program);
}

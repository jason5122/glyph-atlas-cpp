#import "Renderer.h"
#import "model/Rasterizer.h"
#import "util/CTFontUtil.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import "util/OpenGLErrorUtil.h"

Renderer::Renderer(float width, float height, CTFontRef mainFont)
    : width(width), height(height), mainFont(mainFont) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // DEBUG: Draw shapes as wireframes.

    link_shaders();

    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "resolution"), width, height);

    // Font experiments.
    CTFontRef appleEmojiFont = CTFontCreateWithName(CFSTR("Apple Color Emoji"), 16, nullptr);
    logDefault(@"Renderer", @"colored? %d", CTFontIsColored(appleEmojiFont));

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
        logDefault(@"Renderer", @"%@ %@", familyName, style);

        CTFontRef tempFont = CTFontCreateWithFontDescriptor(descriptor, 32, nullptr);
    }
    // End of font experiments.

    load_glyphs();

    Metrics metrics = CTFontGetMetrics(mainFont);
    float cell_width = CGFloat_floor(metrics.average_advance + 1);
    float cell_height = CGFloat_floor(metrics.line_height + 2);

    std::vector<glm::vec2> offsets(100);
    int i = 0;
    for (int row = 0; row < 10; row++) {
        for (int col = 0; col < 10; col++) {
            offsets[i++] = glm::vec2(row * cell_width, col * cell_height);
        }
    }

    glGenBuffers(1, &vbo_instance);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * offsets.size(), &offsets[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3,  // second triangle
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(1, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::render_text(std::string text, float x, float y) {
    // glViewport(0, 0, width, height);
    // glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    AtlasGlyph glyph = glyph_cache[text[0]];
    float xpos = x + glyph.bearing.x;
    float ypos = y - (glyph.size.y - glyph.bearing.y);
    float w = glyph.size.x;
    float h = glyph.size.y;

    // UV coordinates are normalized to [0, 1].
    float uv_width = w / ATLAS_SIZE;
    float uv_height = h / ATLAS_SIZE;

    float vertices[4][4] = {
        {xpos + w, ypos + h, uv_width, 0.0f},   // bottom right
        {xpos + w, ypos, uv_width, uv_height},  // top right
        {xpos, ypos, 0.0f, uv_height},          // top left
        {xpos, ypos + h, 0.0f, 0.0f},           // bottom left
    };
    glBindTexture(GL_TEXTURE_2D, atlas);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, 100);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.

    // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // https://learnopengl.com/In-Practice/Debugging
    glPrintError();
}

void Renderer::load_glyphs() {
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

    CGGlyph glyph_index = CTFontGetGlyphIndex(mainFont, 'E');
    RasterizedGlyph glyph = rasterizer.rasterize_glyph(glyph_index);

    glBindTexture(GL_TEXTURE_2D, atlas);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glyph.width, glyph.height, GL_RGB, GL_UNSIGNED_BYTE,
                    &glyph.buffer[0]);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    AtlasGlyph atlas_glyph = {glm::ivec2(glyph.width, glyph.height),
                              glm::ivec2(glyph.left, glyph.top)};
    glyph_cache.insert({'E', atlas_glyph});
}

void Renderer::link_shaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = readFile(resourcePath("text_vert.glsl"));
    const GLchar* frag_source = readFile(resourcePath("text_frag.glsl"));
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
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader_program);
}

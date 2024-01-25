#pragma once

#import <OpenGL/gl3.h>

class CursorRenderer {
public:
    CursorRenderer(float width, float height);
    void draw(float x, float y, float cell_width, float cell_height);
    void resize(int new_width, int new_height);
    ~CursorRenderer();

private:
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo, ebo;

    void linkShaders();
};

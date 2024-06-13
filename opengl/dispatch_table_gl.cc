#include "dispatch_table_gl.h"

namespace opengl {

#define ASSIGN(NAME, FP) FP = reinterpret_cast<decltype(FP)>(loadProcAddress(NAME))

void DispatchTableGL::initProcsGL() {
    // 1.0
    ASSIGN("glClear", clear);
    ASSIGN("glClearColor", clearColor);
    ASSIGN("glDepthMask", depthMask);
    ASSIGN("glEnable", enable);
    ASSIGN("glGetError", getError);
    ASSIGN("glViewport", viewport);

    // 1.1
    ASSIGN("glGenTextures", genTextures);

    // 1.4
    ASSIGN("glBlendFuncSeparate", blendFuncSeparate);

    // 1.5
    ASSIGN("glGenBuffers", genBuffers);
    ASSIGN("glBindBuffer", bindBuffer);
    ASSIGN("glBufferData", bufferData);
    ASSIGN("glBufferSubData", bufferSubData);

    // 2.0
    ASSIGN("glAttachShader", attachShader);
    ASSIGN("glCompileShader", compileShader);
    ASSIGN("glCreateProgram", createProgram);
    ASSIGN("glCreateShader", createShader);
    ASSIGN("glDeleteProgram", deleteProgram);
    ASSIGN("glDeleteShader", deleteShader);
    ASSIGN("glLinkProgram", linkProgram);
    ASSIGN("glShaderSource", shaderSource);
    ASSIGN("glGetShaderInfoLog", getShaderInfoLog);
    ASSIGN("glGetProgramInfoLog", getProgramInfoLog);
    ASSIGN("glGetShaderiv", getShaderiv);
    ASSIGN("glGetProgramiv", getProgramiv);
    ASSIGN("glEnableVertexAttribArray", enableVertexAttribArray);
    ASSIGN("glVertexAttribPointer", vertexAttribPointer);
    ASSIGN("glUseProgram", useProgram);
    ASSIGN("glGetUniformLocation", getUniformLocation);
    ASSIGN("glUniform2f", uniform2f);

    // 3.0
    ASSIGN("glBindVertexArray", bindVertexArray);
    ASSIGN("glGenVertexArrays", genVertexArrays);

    // 3.1
    ASSIGN("glDrawElementsInstanced", drawElementsInstanced);

    // 3.3
    ASSIGN("glVertexAttribDivisor", vertexAttribDivisor);
}

}

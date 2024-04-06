#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "ui/app/app.h"
#include "ui/app/app_window.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"
#include "util/profile_util.h"
#include <glad/glad.h>
#include <iostream>

class EditorWindow : public AppWindow {
public:
    EditorWindow(int id) : id(id) {}

    void onOpenGLActivate(int width, int height) {
        std::cerr << "id " << id << ": " << glGetString(GL_VERSION) << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

        // fs::path file_path = ResourcePath() / "sample_files/example.json";
        // fs::path file_path = ResourcePath() / "sample_files/worst_case.json";
        fs::path file_path = ResourcePath() / "sample_files/sort.scm";

        buffer.setContents(ReadFile(file_path));
        highlighter.setLanguage("source.json");

        // TODO: Implement scale factor support.
        main_font_rasterizer.setup(0, "Source Code Pro", 16 * 2);
        ui_font_rasterizer.setup(1, "SF Pro Text", 11 * 2);

        text_renderer.setup(width, height, main_font_rasterizer);
        rect_renderer.setup(width, height);
        image_renderer.setup(width, height);
    }

    void onDraw() {
        {
            int editor_offset_x = 200 * 2;
            int editor_offset_y = 30 * 2;
            int status_bar_height = ui_font_rasterizer.line_height;

            PROFILE_BLOCK("id " + std::to_string(id) + ": redraw");
            glClear(GL_COLOR_BUFFER_BIT);

            glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
            text_renderer.renderText(0, 0, buffer, highlighter, editor_offset_x, editor_offset_y,
                                     main_font_rasterizer, status_bar_height);

            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
            rect_renderer.draw(0, 0, text_renderer.cursor_end_x, text_renderer.cursor_end_line,
                               main_font_rasterizer.line_height, buffer.lineCount(),
                               text_renderer.longest_line_x, editor_offset_x, editor_offset_y,
                               status_bar_height);

            image_renderer.draw(0, 0, editor_offset_x, editor_offset_y);

            glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
            text_renderer.renderUiText(main_font_rasterizer, ui_font_rasterizer);
        }
    }

    void onResize(int width, int height) {
        glViewport(0, 0, width, height);
        text_renderer.resize(width, height);
        rect_renderer.resize(width, height);
        image_renderer.resize(width, height);
    }

private:
    int id;
    Buffer buffer;
    SyntaxHighlighter highlighter;

    FontRasterizer main_font_rasterizer;
    FontRasterizer ui_font_rasterizer;

    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    ImageRenderer image_renderer;
};

class SimpleText : public App {
public:
    SimpleText() : editor_window1(0), editor_window2(1) {}

    void onActivate() {
        createNewWindow(editor_window1);
        createNewWindow(editor_window2);
    }

private:
    EditorWindow editor_window1;
    EditorWindow editor_window2;
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText editor;
    editor.run();
    return 0;
}

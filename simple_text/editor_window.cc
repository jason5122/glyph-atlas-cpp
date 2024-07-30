#include "editor_window.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/horizontal_layout_widget.h"
#include "gui/widget/side_bar_widget.h"
#include "gui/widget/status_bar_widget.h"
#include "gui/widget/vertical_layout_widget.h"
#include "simple_text/editor_app.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"

using namespace gui;

namespace {
constexpr std::string Repeat(std::string_view sv, size_t times) {
    std::string result;
    for (size_t i = 0; i < times; ++i) {
        result += sv;
    }
    return result;
}

const std::string kSampleText =
    R"(Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non
proident, sunt in culpa qui officia deserunt mollit anim id est laborum.

)";

const std::string kLongLine =
    "This is a long long long long long long long long long long long long long long long long "
    "long long long long long long long long long long long long long long long long long long "
    "long long long long long long long long long long long long long long long long long long "
    "long long line.";

const std::string kUnicode =
    R"(˚¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£

🥲🥲🥲🥲🥲🥲)";
}

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window{parent, width, height},
      wid{wid},
      parent{parent},
      main_widget{new VerticalLayoutWidget{}} {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    main_widget->setWidth(width);
    main_widget->setHeight(height);

    editor_widget = std::make_shared<EditorWidget>();
    editor_widget->addTab("hello.txt", "Hello world!\nhi there");
    editor_widget->addTab("unicode.txt", kUnicode);
    editor_widget->addTab("long_line.txt", kSampleText + Repeat(kLongLine, 50));
    editor_widget->addTab("sample_text.txt", Repeat(kSampleText, 50) + kLongLine);

    // Main widgets.
    std::shared_ptr<ContainerWidget> horizontal_layout{new HorizontalLayoutWidget{}};
    std::shared_ptr<ContainerWidget> vertical_layout{new VerticalLayoutWidget{}};

    // These don't have default constructors since they are not intended to be main widgets.
    constexpr int kSideBarWidth = 250 * 2;
    constexpr int kStatusBarHeight = 22 * 2;
    std::shared_ptr<Widget> side_bar{new SideBarWidget({kSideBarWidth, height})};
    std::shared_ptr<Widget> status_bar{new StatusBarWidget({width, kStatusBarHeight})};

    horizontal_layout->addChildStart(side_bar);
    vertical_layout->setMainWidget(editor_widget);
    horizontal_layout->setMainWidget(vertical_layout);
    main_widget->setMainWidget(horizontal_layout);
    main_widget->addChildEnd(status_bar);
}

void EditorWindow::onDraw(int width, int height) {
    {
        // PROFILE_BLOCK("Total render time");
        main_widget->draw();
        Renderer::instance().flush({width, height});
    }
}

void EditorWindow::onResize(int width, int height) {
    main_widget->setWidth(width);
    main_widget->setHeight(height);
    main_widget->layout();
    redraw();
}

void EditorWindow::onScroll(int mouse_x, int mouse_y, int dx, int dy) {
    main_widget->scroll({mouse_x, mouse_y}, {dx, dy});
    main_widget->layout();
    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x,
                                   int mouse_y,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    drag_start_widget = main_widget->getWidgetAtPosition({mouse_x, mouse_y});
    if (drag_start_widget) {
        drag_start_widget->leftMouseDown({mouse_x, mouse_y});
        redraw();
    }
}

void EditorWindow::onLeftMouseUp() {
    drag_start_widget = nullptr;
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    if (drag_start_widget) {
        drag_start_widget->leftMouseDrag({mouse_x, mouse_y});
        redraw();
    }
}

bool EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    bool handled = false;
    if (key == app::Key::kJ && modifiers == app::ModifierKey::kSuper) {
        editor_widget->prevIndex();
        handled = true;
    }
    if (key == app::Key::kK && modifiers == app::ModifierKey::kSuper) {
        editor_widget->nextIndex();
        handled = true;
    }
    if (key == app::Key::k1 && modifiers == app::ModifierKey::kSuper) {
        editor_widget->setIndex(0);
        handled = true;
    }
    if (key == app::Key::k2 && modifiers == app::ModifierKey::kSuper) {
        editor_widget->setIndex(1);
        handled = true;
    }
    if (key == app::Key::k3 && modifiers == app::ModifierKey::kSuper) {
        editor_widget->setIndex(2);
        handled = true;
    }
    if (key == app::Key::k4 && modifiers == app::ModifierKey::kSuper) {
        editor_widget->setIndex(3);
        handled = true;
    }
    if (key == app::Key::kA && modifiers == app::ModifierKey::kSuper) {
        editor_widget->selectAll();
        handled = true;
    }
    if (key == app::Key::kW && modifiers == app::ModifierKey::kSuper) {
        editor_widget->removeTab(editor_widget->getCurrentIndex());
        handled = true;
    }

    if (handled) {
        redraw();
    }
    return handled;
}

void EditorWindow::onInsertText(std::string_view text) {
    {
        PROFILE_BLOCK("EditorWindow::onInsertText()");
        editor_widget->insertText(text);
    }
    redraw();
}

void EditorWindow::onAction(app::Action action, bool extend) {
    bool handled = false;
    {
        PROFILE_BLOCK("EditorWindow::onAction()");
        if (action == app::Action::kMoveForwardByCharacters) {
            editor_widget->move(gui::MoveBy::kCharacters, true, extend);
            handled = true;
        }
        if (action == app::Action::kMoveBackwardByCharacters) {
            editor_widget->move(gui::MoveBy::kCharacters, false, extend);
            handled = true;
        }
        if (action == app::Action::kMoveForwardByLines) {
            editor_widget->move(gui::MoveBy::kLines, true, extend);
            handled = true;
        }
        if (action == app::Action::kMoveBackwardByLines) {
            editor_widget->move(gui::MoveBy::kLines, false, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToHardBOL) {
            editor_widget->moveTo(gui::MoveTo::kHardBOL, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToHardEOL) {
            editor_widget->moveTo(gui::MoveTo::kHardEOL, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToBOF) {
            editor_widget->moveTo(gui::MoveTo::kBOF, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToEOF) {
            editor_widget->moveTo(gui::MoveTo::kEOF, extend);
            handled = true;
        }
        if (action == app::Action::kInsertNewline) {
            editor_widget->insertText("\n");
            handled = true;
        }
        if (action == app::Action::kInsertTab) {
            editor_widget->insertText("    ");
            handled = true;
        }
        if (action == app::Action::kLeftDelete) {
            editor_widget->leftDelete();
            handled = true;
        }
    }

    if (handled) {
        redraw();
    }
}

void EditorWindow::onAppAction(app::AppAction action) {
    if (action == app::AppAction::kNewFile) {
        editor_widget->addTab("sample_text.txt", Repeat(kSampleText, 50) + kLongLine);
        redraw();
    }
    if (action == app::AppAction::kNewWindow) {
        parent.createWindow();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}

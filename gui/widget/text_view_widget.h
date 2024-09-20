#pragma once

#include "base/buffer/piece_table.h"
#include "base/syntax_highlighter/syntax_highlighter.h"
#include "gui/text_system/line_layout_cache.h"
#include "gui/text_system/selection.h"
#include "gui/widget/scrollable_widget.h"
#include "gui/widget/types.h"

namespace gui {

class TextViewWidget : public ScrollableWidget {
public:
    TextViewWidget(std::string_view text);

    void selectAll();
    void move(MoveBy by, bool forward, bool extend);
    void moveTo(MoveTo to, bool extend);
    void insertText(std::string_view text);
    void leftDelete();
    void rightDelete();
    void deleteWord(bool forward);
    std::string getSelectionText();

    void draw() override;
    void leftMouseDown(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;

    void updateMaxScroll() override;

private:
    static constexpr int kMinScrollbarWidth = 100;
    static constexpr Rgb kTextColor{51, 51, 51};
    static constexpr Rgba kScrollBarColor{190, 190, 190, 255};
    static constexpr Rgba kCaretColor{95, 180, 180, 255};

    base::PieceTable table;
    LineLayoutCache line_layout_cache;

    Selection selection{};
    base::SyntaxHighlighter highlighter;

    size_t lineAtY(int y);
    inline const font::LineLayout& layoutAt(size_t line);
    inline const font::LineLayout& layoutAt(size_t line, bool& exclude_end);

    // Draw helpers.
    void renderText(size_t start_line, size_t end_line, int main_line_height);
    void renderSelections(size_t start_line, size_t end_line);
    void renderScrollBars(int main_line_height, size_t visible_lines);
    void renderCaret(int main_line_height);
};

}

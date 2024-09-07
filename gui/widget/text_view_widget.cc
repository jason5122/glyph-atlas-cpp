#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "text_view_widget.h"
#include <cmath>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <cassert>
#include <format>
#include <iostream>

namespace gui {

TextViewWidget::TextViewWidget(std::string_view text) : table{text} {
    updateMaxScroll();
}

void TextViewWidget::selectAll() {
    start_caret.index = 0;
    end_caret.index = table.length();

    // updateCaretX();
}

void TextViewWidget::move(MoveBy by, bool forward, bool extend) {
    PROFILE_BLOCK("TextViewWidget::move()");

    auto [line, col] = table.lineColumnAt(end_caret.index);
    const auto& layout = layoutAt(line);

    if (by == MoveBy::kCharacters && !forward) {
        size_t delta = end_caret.moveToPrevGlyph(layout, col);
        selection.end().moveToPrevGlyph(layout, col);

        // Move to previous line if at beginning of line.
        if (delta == 0 && line > 0) {
            const auto& prev_layout = layoutAt(line - 1);
            end_caret.index = table.indexAt(line - 1, base::sub_sat(prev_layout.length, 1_Z));

            size_t index = table.indexAt(line - 1, base::sub_sat(prev_layout.length, 1_Z));
            selection.setIndex(index, extend);
        }
        // updateCaretX();
    }
    if (by == MoveBy::kCharacters && forward) {
        end_caret.moveToNextGlyph(layout, col);
        selection.end().moveToNextGlyph(layout, col);
        // updateCaretX();
    }
    // TODO: Find a clean way to combine vertical caret movement logic.
    if (by == MoveBy::kLines && !forward) {
        if (line > 0) {
            bool exclude_end;
            const auto& prev_layout = layoutAt(line - 1, exclude_end);
            int x = Caret::xAtColumn(layout, col, false);
            size_t new_col = Caret::columnAtX(prev_layout, x, exclude_end);
            end_caret.index = table.indexAt(line - 1, new_col);

            size_t index = table.indexAt(line - 1, new_col);
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kLines && forward) {
        if (line < base::sub_sat(table.lineCount(), 1_Z)) {
            bool exclude_end;
            const auto& prev_layout = layoutAt(line + 1, exclude_end);
            int x = Caret::xAtColumn(layout, col, false);
            size_t new_col = Caret::columnAtX(prev_layout, x, exclude_end);
            end_caret.index = table.indexAt(line + 1, new_col);

            size_t index = table.indexAt(line + 1, new_col);
            selection.setIndex(index, extend);
        }
    }

    if (!extend) {
        start_caret = end_caret;
    }
}

void TextViewWidget::moveTo(MoveTo to, bool extend) {
    PROFILE_BLOCK("TextViewWidget::moveTo()");

    if (to == MoveTo::kHardBOL) {
        auto [line, _] = table.lineColumnAt(end_caret.index);

        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        size_t new_col = Caret::columnAtX(layout, 0, exclude_end);

        end_caret.index = table.indexAt(line, new_col);
        selection.setIndex(table.indexAt(line, new_col), extend);
        // updateCaretX();
    }
    if (to == MoveTo::kHardEOL) {
        auto [line, _] = table.lineColumnAt(end_caret.index);

        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        size_t new_col = Caret::columnAtX(layout, layout.width, exclude_end);

        end_caret.index = table.indexAt(line, new_col);
        selection.setIndex(table.indexAt(line, new_col), extend);
        // updateCaretX();
    }
    if (to == MoveTo::kBOF) {
        end_caret.index = 0;
        // updateCaretX();
    }
    if (to == MoveTo::kEOF) {
        end_caret.index = table.length();
        selection.setIndex(table.length(), extend);
        // updateCaretX();
    }

    if (!extend) {
        start_caret = end_caret;
    }
}

void TextViewWidget::insertText(std::string_view text) {
    PROFILE_BLOCK("TextViewWidget::insertText()");

    table.insert(end_caret.index, text);
    end_caret.index += text.length();

    start_caret = end_caret;
    // TODO: Do we update caret `max_x` too?

    updateMaxScroll();
}

void TextViewWidget::leftDelete() {
    PROFILE_BLOCK("TextViewWidget::leftDelete()");

    // Selection is empty.
    if (start_caret == end_caret) {
        auto [line, col] = table.lineColumnAt(end_caret.index);
        const auto& layout = layoutAt(line);

        size_t delta = end_caret.moveToPrevGlyph(layout, col);

        // Delete newline if at beginning of line.
        if (delta == 0 && line > 0) {
            --end_caret.index;
            delta = 1;
        }

        table.erase(end_caret.index, delta);

        start_caret = end_caret;
    } else {
        const auto& c1 = std::min(start_caret, end_caret);
        const auto& c2 = std::max(start_caret, end_caret);
        table.erase(c1.index, c2.index - c1.index);
        start_caret = c1;
        end_caret = c1;
    }
}

std::string TextViewWidget::getSelectionText() {
    const auto& c1 = std::min(start_caret, end_caret);
    const auto& c2 = std::max(start_caret, end_caret);
    return table.substr(c1.index, c2.index - c1.index);
}

void TextViewWidget::draw() {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    // Calculate start and end lines.
    int main_line_height = metrics.line_height;
    size_t visible_lines = std::ceil(static_cast<float>(size.height) / main_line_height);

    size_t start_line = scroll_offset.y / main_line_height;
    size_t end_line = start_line + visible_lines;

    renderText(start_line, end_line, main_line_height);
    renderSelections(start_line, end_line);
    renderScrollBars(main_line_height, visible_lines);
    renderCaret(main_line_height);
}

void TextViewWidget::leftMouseDown(const Point& mouse_pos) {
    {
        // TODO: Outdated; remove this.
        leftMouseDrag(mouse_pos);
        start_caret = end_caret;
        // updateCaretX();  // Update for start as well.
    }

    Point new_coords = mouse_pos - position + scroll_offset;
    size_t new_line = lineAtY(new_coords.y);

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    size_t new_col = Caret::columnAtX(layout, new_coords.x, exclude_end);

    selection.setIndex(table.indexAt(new_line, new_col), false);
}

void TextViewWidget::leftMouseDrag(const Point& mouse_pos) {
    Point new_coords = mouse_pos - position + scroll_offset;
    size_t new_line = lineAtY(new_coords.y);

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    size_t new_col = Caret::columnAtX(layout, new_coords.x, exclude_end);

    end_caret.index = table.indexAt(new_line, new_col);
    selection.setIndex(table.indexAt(new_line, new_col), true);

    // updateCaretX();
}

void TextViewWidget::updateMaxScroll() {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    max_scroll_offset.x = line_layout_cache.maxWidth();
    max_scroll_offset.y = table.lineCount() * metrics.line_height;
}

size_t TextViewWidget::lineAtY(int y) {
    if (y < 0) {
        y = 0;
    }

    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    size_t line = y / metrics.line_height;
    return std::clamp(line, 0_Z, base::sub_sat(table.lineCount(), 1_Z));
}

inline const font::LineLayout& TextViewWidget::layoutAt(size_t line) {
    bool unused;
    return layoutAt(line, unused);
}

inline const font::LineLayout& TextViewWidget::layoutAt(size_t line, bool& exclude_end) {
    std::string line_str = table.line(line);
    exclude_end = !line_str.empty() && line_str.back() == '\n';

    if (exclude_end) {
        line_str.back() = ' ';
    }

    return line_layout_cache.getLineLayout(line_str);
}

void TextViewWidget::renderText(size_t start_line, size_t end_line, int main_line_height) {
    // Render two lines before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 2_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, table.lineCount());
    end_line = std::clamp(end_line, 0_Z, table.lineCount());

    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    {
        PROFILE_BLOCK("TextViewWidget::renderText()");
        for (size_t line = start_line; line < end_line; ++line) {
            const auto& layout = layoutAt(line);

            Point coords = position - scroll_offset;
            // TODO: Using `metrics.line_height` causes a use-after-free error??
            //       The line with `line_layout_cache.getLineLayout()` is problematic.
            coords.y += line * main_line_height;

            // TODO: These changes are optimal to match Sublime Text's layout. Formalize this.
            coords.y -= main_line_height;
            // TODO: Seems like Sublime Text adds something to the left edge no matter what.
            // We could formalize this as padding, but we need selection to be flush...
            // coords.x += 3;  // Source Code Pro et al.
            // coords.x += 2;  // Chinese

            int min_x = scroll_offset.x;
            int max_x = scroll_offset.x + size.width;

            text_renderer.renderLineLayout(layout, coords, min_x, max_x, kTextColor,
                                           TextRenderer::FontType::kMain);
        }
    }
    constexpr bool kDebugAtlas = false;
    if constexpr (kDebugAtlas) {
        text_renderer.renderAtlasPages(position);
    }
}

void TextViewWidget::renderSelections(size_t start_line, size_t end_line) {
    SelectionRenderer& selection_renderer = Renderer::instance().getSelectionRenderer();
    const auto& c1 = std::min(start_caret, end_caret);
    const auto& c2 = std::max(start_caret, end_caret);
    auto [c1_line, c1_col] = table.lineColumnAt(c1.index);
    auto [c2_line, c2_col] = table.lineColumnAt(c2.index);

    const auto& c1_layout = layoutAt(c1_line);
    const auto& c2_layout = layoutAt(c2_line);
    int c1_x = Caret::xAtColumn(c1_layout, c1_col);
    int c2_x = Caret::xAtColumn(c2_layout, c2_col);

    // Don't render off-screen selections.
    if (c1_line < start_line) c1_line = start_line;
    if (c2_line > end_line) c2_line = end_line;

    std::vector<SelectionRenderer::Selection> selections;
    for (size_t line = c1_line; line <= c2_line; ++line) {
        const auto& layout = layoutAt(line);
        int start = line == c1_line ? c1_x : 0;
        int end = line == c2_line ? c2_x : layout.width;

        if (end - start > 0) {
            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>(line),
                .start = start,
                .end = end,
            });
        }
    }
    selection_renderer.renderSelections(selections, position - scroll_offset);
}

void TextViewWidget::renderScrollBars(int main_line_height, size_t visible_lines) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int line_count = table.lineCount();
    int line_height = main_line_height;
    int vbar_width = 15;
    int max_scrollbar_y = (line_count + visible_lines) * line_height;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scrollbar_y);
    vbar_height = std::max(30, vbar_height);
    float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;
    Point vbar_coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, kScrollBarColor, 5);

    // Add horizontal scroll bar.
    // int hbar_height = 15;
    // int hbar_width = size.width * (static_cast<float>(size.width) / max_scroll_offset.x);
    // hbar_width = std::max(hbar_width, kMinScrollbarWidth);
    // float hbar_percent = static_cast<float>(scroll_offset.x) / max_scroll_offset.x;
    // Point hbar_coords{
    //     .x = static_cast<int>(std::round((size.width - hbar_width) * hbar_percent)),
    //     .y = size.height - hbar_height,
    // };
    // rect_renderer.addRect(hbar_coords + position, {hbar_width, hbar_height}, kScrollBarColor,
    // 5);
}

void TextViewWidget::renderCaret(int main_line_height) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    int caret_width = 4;
    int extra_padding = 8;
    int caret_height = main_line_height + extra_padding * 2;

    auto [line, col] = table.lineColumnAt(end_caret.index);
    bool exclude_end;
    const auto& layout = layoutAt(line, exclude_end);
    int end_caret_x = Caret::xAtColumn(layout, col, exclude_end);

    Point caret_pos{
        .x = end_caret_x,
        .y = static_cast<int>(line) * main_line_height,
    };
    caret_pos += position;
    caret_pos -= scroll_offset;
    caret_pos.x -= caret_width / 2;
    caret_pos.y -= extra_padding;
    rect_renderer.addRect(caret_pos, {caret_width, caret_height}, kCaretColor);

    {
        auto [line, col] = table.lineColumnAt(selection.end().index);
        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        int caret_x = Caret::xAtColumn(layout, col, exclude_end);

        Point caret_pos{
            .x = caret_x,
            .y = static_cast<int>(line) * main_line_height,
        };
        caret_pos += position;
        caret_pos -= scroll_offset;
        caret_pos.x -= caret_width / 2;
        caret_pos.y -= extra_padding;
        rect_renderer.addRect(caret_pos, {caret_width, caret_height}, {255, 0, 0, 255});
    }
}

}

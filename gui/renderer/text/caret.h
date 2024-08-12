#pragma once

#include "font/types.h"
#include "gui/renderer/types.h"
#include <cstddef>

namespace gui {

struct Caret {
    size_t line;
    size_t index;  // UTF-8 index in line.
    int x;
    // We use this value to position the caret during vertical movement.
    // This is updated whenever the caret moves horizontally.
    int max_x;

    // TODO: Is this the best implementation?
    size_t run_index;
    size_t run_glyph_index;

    void moveToPoint(const font::LineLayout& layout, size_t line, const Point& point);
    void moveByCharacters(const font::LineLayout& layout, bool forward);

    friend constexpr bool operator==(const Caret& c1, const Caret& c2) {
        return c1.line == c2.line && c1.index == c2.index;
    }

    friend constexpr bool operator!=(const Caret& c1, const Caret& c2) {
        return !(operator==(c1, c2));
    }

    friend constexpr bool operator<(const Caret& c1, const Caret& c2) {
        if (c1.line == c2.line) {
            return c1.index < c2.index;
        } else {
            return c1.line < c2.line;
        }
    }

    friend constexpr bool operator>(const Caret& c1, const Caret& c2) {
        return operator<(c2, c1);
    }

    friend constexpr bool operator<=(const Caret& c1, const Caret& c2) {
        return !(operator>(c1, c2));
    }

    friend constexpr bool operator>=(const Caret& c1, const Caret& c2) {
        return !(operator<(c1, c2));
    }
};

}

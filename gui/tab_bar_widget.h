#pragma once

#include "gui/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(const renderer::Size& size);

    void draw(const renderer::Size& screen_size, const renderer::Point& offset) override;
};

}

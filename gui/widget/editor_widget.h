#pragma once

#include "gui/widget/multi_view_widget.h"
#include "gui/widget/tab_bar_widget.h"
#include "gui/widget/vertical_layout_widget.h"

namespace gui {

class EditorWidget : public VerticalLayoutWidget {
public:
    EditorWidget();

    void setIndex(size_t index);
    void nextIndex();
    void addTab(const std::string& text);

private:
    static constexpr int kTabBarHeight = 29 * 2;

    std::shared_ptr<MultiViewWidget> multi_view;
    std::shared_ptr<TabBarWidget> tab_bar;
};

}

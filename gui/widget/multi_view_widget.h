#pragma once

#include "gui/widget/text_view_widget.h"
#include "gui/widget/types.h"
#include "gui/widget/widget.h"
#include <memory>
#include <vector>

namespace gui {

class MultiViewWidget : public Widget {
public:
    MultiViewWidget();

    TextViewWidget* currentTextViewWidget() const;
    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    void lastIndex();
    size_t getCurrentIndex();
    void addTab(std::string_view text);
    void removeTab(size_t index);

    void draw() override;
    void scroll(const Point& mouse_pos, const Point& delta) override;
    void leftMouseDown(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void layout() override;

private:
    size_t index = 0;
    std::vector<std::shared_ptr<TextViewWidget>> text_views;
};

}

#include "horizontal_layout_widget.h"

#include <iostream>

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(const renderer::Size& size)
    : ContainerWidget{size} {}

// TODO: Debug use; remove this.
void HorizontalLayoutWidget::draw() {
    std::cerr << "HorizontalLayout: position = " << position << ", size = " << size << '\n';
    ContainerWidget::draw();
}

void HorizontalLayoutWidget::layout() {
    int left_offset = position.x;
    for (auto& child : children) {
        child->setPosition({left_offset, position.y});
        child->setHeight(size.height);

        // Recursively layout children.
        child->layout();

        left_offset += child->getSize().width;
    }

    if (main_widget) {
        main_widget->setPosition({left_offset, position.y});
        main_widget->setWidth(size.width - (left_offset - position.x));
        main_widget->setHeight(size.height);

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}

#include "app/cocoa/impl_cocoa.h"
#include "menu.h"

namespace app {

Menu::Menu() : pimpl{new impl{}} {
    pimpl->ns_menu = [[NSMenu alloc] initWithTitle:@""];
}

Menu::~Menu() {
    [pimpl->ns_menu release];
}

void Menu::addItem(ItemType type) {
    [pimpl->ns_menu addItemWithTitle:@"Exit" action:@selector(terminate:) keyEquivalent:@""];
}

void Menu::show(const Point& mouse_pos) const {
    NSPoint point = {static_cast<CGFloat>(mouse_pos.x), static_cast<CGFloat>(mouse_pos.y)};
    [pimpl->ns_menu popUpMenuPositioningItem:nil atLocation:point inView:nullptr];
}

}  // namespace app

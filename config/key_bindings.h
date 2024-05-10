#pragma once

#include "base/filesystem/file_reader.h"
#include "glaze/glaze.hpp"
#include "gui/key.h"
#include "gui/modifier_key.h"
#include <unordered_map>
#include <vector>

namespace config {
enum class Action {
    kNone,
    kNewWindow,
    kCloseWindow,
    kNewTab,
    kCloseTab,
    kPreviousTab,
    kNextTab,
    kSelectTab1,
    kSelectTab2,
    kSelectTab3,
    kSelectTab4,
    kSelectTab5,
    kSelectTab6,
    kSelectTab7,
    kSelectTab8,
    kSelectLastTab,
    kToggleSideBar,
};

class KeyBindings {
public:
    KeyBindings();
    void reload();
    Action parseKeyPress(app::Key key, app::ModifierKey modifiers);

private:
    static inline const fs::path kKeyBindingsPath = DataDir() / "key_bindings.json";
    static constexpr glz::opts kDefaultOptions{.prettify = true, .indentation_width = 2};
    static constexpr char kDelimiter = '+';

    struct JsonSchema {
        std::string keys;
        std::string command;
    };

    struct Binding {
        app::Key key;
        app::ModifierKey modifiers;
        Action action;
    };

    std::vector<Binding> bindings;
    void addBinding(const std::string& keys, const std::string& command);

    static inline const std::unordered_map<std::string, app::ModifierKey> modifier_map{
        {"shift", app::ModifierKey::kShift}, {"ctrl", app::ModifierKey::kControl},
        {"alt", app::ModifierKey::kAlt},     {"super", app::ModifierKey::kSuper},
        {"primary", app::kPrimaryModifier},
    };
    static inline const std::unordered_map<std::string, Action> action_map{
        {"new_window", Action::kNewWindow},     {"close_window", Action::kCloseWindow},
        {"new_tab", Action::kNewTab},           {"close_tab", Action::kCloseTab},
        {"previous_tab", Action::kPreviousTab}, {"next_tab", Action::kNextTab},
    };
};
}

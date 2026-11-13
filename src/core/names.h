#pragma once

#include "spriteloop/spla_baked.hpp"
#include "spriteloop/spla_package.hpp"

#include <godot_cpp/variant/string.hpp>

#include <string>

namespace spriteloop_godot {

inline std::string to_std(const godot::String& value)
{
    const godot::CharString utf8 = value.utf8();
    return std::string(utf8.get_data(), static_cast<std::size_t>(utf8.length()));
}

inline godot::String to_godot(const std::string& value)
{
    return godot::String::utf8(value.c_str(), static_cast<int64_t>(value.size()));
}

// Name shown in the inspector and returned by getters: the display name, or
// the exported ID when the name is empty.
template <typename T>
godot::String display_name(const T& entry)
{
    return to_godot(entry.name.empty() ? entry.id : entry.name);
}

// Same lookup order as SplaPlayer::play: exact ID first, then display name.
inline int find_animation_index(const spriteloop::SplaPackage& package, const std::string& value)
{
    for (std::size_t i = 0; i < package.animations.size(); ++i) {
        if (package.animations[i].id == value) {
            return static_cast<int>(i);
        }
    }
    for (std::size_t i = 0; i < package.animations.size(); ++i) {
        if (package.animations[i].name == value) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace spriteloop_godot

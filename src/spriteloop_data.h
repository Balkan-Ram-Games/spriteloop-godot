#pragma once

#include "core/package_asset.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <memory>

namespace godot {

// An imported SpriteLoop .spla package.
//
// Holds the original package bytes. The parsed package and atlas texture are
// built on first use and shared by every node that uses this resource.
class SpriteLoopData : public Resource {
    GDCLASS(SpriteLoopData, Resource)

public:
    SpriteLoopData() = default;
    ~SpriteLoopData() override = default;

    void set_package_bytes(const PackedByteArray& bytes);
    PackedByteArray get_package_bytes() const;
    // Generate atlas mipmaps (the "Mipmaps" import option). Needed for
    // characters drawn much smaller than the art, e.g. distant 3D characters.
    void set_mipmaps(bool enabled);
    bool has_mipmaps() const;

    // Loads a .spla file directly (for runtime-downloaded or modded content).
    static Ref<SpriteLoopData> load_from_file(const String& path, bool mipmaps = false);

    bool is_valid() const;
    String get_load_error() const;

    String get_package_name() const;
    Vector2i get_canvas_size() const;

    PackedStringArray get_animation_names() const;
    bool has_animation(const StringName& animation) const;
    int get_animation_frame_count(const StringName& animation) const;
    double get_animation_fps(const StringName& animation) const;
    double get_animation_length(const StringName& animation) const;
    bool is_animation_looping(const StringName& animation) const;

    PackedStringArray get_skin_names() const;
    PackedStringArray get_part_names() const;
    PackedStringArray get_variant_names(const StringName& part) const;

    // Internal: shared parsed package, or nullptr (see get_load_error()).
    const std::shared_ptr<spriteloop_godot::PackageAsset>& asset() const;

    // Resolves an animation by ID, then display name. Returns -1 if missing.
    int find_animation_index(const String& animation) const;

protected:
    static void _bind_methods();

private:
    PackedByteArray package_bytes_;
    bool mipmaps_ = false;
    mutable std::shared_ptr<spriteloop_godot::PackageAsset> asset_;
    mutable String load_error_;
    mutable bool load_attempted_ = false;
};

} // namespace godot

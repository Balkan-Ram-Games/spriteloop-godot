#include "spriteloop_data.h"

#include "core/names.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace spriteloop_godot;

namespace godot {

void SpriteLoopData::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_package_bytes", "bytes"), &SpriteLoopData::set_package_bytes);
    ClassDB::bind_method(D_METHOD("get_package_bytes"), &SpriteLoopData::get_package_bytes);
    ClassDB::bind_method(D_METHOD("set_mipmaps", "enabled"), &SpriteLoopData::set_mipmaps);
    ClassDB::bind_method(D_METHOD("has_mipmaps"), &SpriteLoopData::has_mipmaps);
    ClassDB::bind_static_method("SpriteLoopData", D_METHOD("load_from_file", "path", "mipmaps"),
                                &SpriteLoopData::load_from_file, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("is_valid"), &SpriteLoopData::is_valid);
    ClassDB::bind_method(D_METHOD("get_load_error"), &SpriteLoopData::get_load_error);
    ClassDB::bind_method(D_METHOD("get_package_name"), &SpriteLoopData::get_package_name);
    ClassDB::bind_method(D_METHOD("get_canvas_size"), &SpriteLoopData::get_canvas_size);
    ClassDB::bind_method(D_METHOD("get_animation_names"), &SpriteLoopData::get_animation_names);
    ClassDB::bind_method(D_METHOD("has_animation", "animation"), &SpriteLoopData::has_animation);
    ClassDB::bind_method(D_METHOD("get_animation_frame_count", "animation"),
                         &SpriteLoopData::get_animation_frame_count);
    ClassDB::bind_method(D_METHOD("get_animation_fps", "animation"), &SpriteLoopData::get_animation_fps);
    ClassDB::bind_method(D_METHOD("get_animation_length", "animation"),
                         &SpriteLoopData::get_animation_length);
    ClassDB::bind_method(D_METHOD("is_animation_looping", "animation"),
                         &SpriteLoopData::is_animation_looping);
    ClassDB::bind_method(D_METHOD("get_skin_names"), &SpriteLoopData::get_skin_names);
    ClassDB::bind_method(D_METHOD("get_part_names"), &SpriteLoopData::get_part_names);
    ClassDB::bind_method(D_METHOD("get_variant_names", "part"), &SpriteLoopData::get_variant_names);

    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "package_bytes", PROPERTY_HINT_NONE, "",
                              PROPERTY_USAGE_STORAGE),
                 "set_package_bytes", "get_package_bytes");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mipmaps"), "set_mipmaps", "has_mipmaps");
}

void SpriteLoopData::set_package_bytes(const PackedByteArray& bytes)
{
    package_bytes_ = bytes;
    asset_.reset();
    load_error_ = String();
    load_attempted_ = false;
    emit_changed();
}

PackedByteArray SpriteLoopData::get_package_bytes() const
{
    return package_bytes_;
}

void SpriteLoopData::set_mipmaps(bool enabled)
{
    if (enabled == mipmaps_) {
        return;
    }
    mipmaps_ = enabled;
    // The atlas is rebuilt on next use; nodes reload through `changed`.
    asset_.reset();
    load_error_ = String();
    load_attempted_ = false;
    emit_changed();
}

bool SpriteLoopData::has_mipmaps() const
{
    return mipmaps_;
}

Ref<SpriteLoopData> SpriteLoopData::load_from_file(const String& path, bool mipmaps)
{
    const PackedByteArray bytes = FileAccess::get_file_as_bytes(path);
    if (bytes.is_empty()) {
        UtilityFunctions::push_error("SpriteLoop: cannot read package '", path, "'.");
        return Ref<SpriteLoopData>();
    }
    Ref<SpriteLoopData> data;
    data.instantiate();
    data->set_mipmaps(mipmaps);
    data->set_package_bytes(bytes);
    if (!data->is_valid()) {
        UtilityFunctions::push_error("SpriteLoop: invalid package '", path, "': ", data->get_load_error());
        return Ref<SpriteLoopData>();
    }
    return data;
}

const std::shared_ptr<PackageAsset>& SpriteLoopData::asset() const
{
    if (!load_attempted_) {
        load_attempted_ = true;
        if (package_bytes_.is_empty()) {
            load_error_ = "the resource contains no package data";
        } else {
            std::string error;
            const String path = get_path();
            asset_ = PackageAsset::create(package_bytes_.ptr(),
                                          static_cast<std::size_t>(package_bytes_.size()),
                                          to_std(path.is_empty() ? String("<memory>") : path), mipmaps_,
                                          error);
            if (!asset_) {
                load_error_ = to_godot(error);
                UtilityFunctions::push_error("SpriteLoop: failed to load '", path, "': ", load_error_);
            }
        }
    }
    return asset_;
}

bool SpriteLoopData::is_valid() const
{
    return asset() != nullptr;
}

String SpriteLoopData::get_load_error() const
{
    asset();
    return load_error_;
}

String SpriteLoopData::get_package_name() const
{
    return asset() ? to_godot(asset()->package().name) : String();
}

Vector2i SpriteLoopData::get_canvas_size() const
{
    if (!asset()) {
        return Vector2i();
    }
    return Vector2i(asset()->package().canvas_width, asset()->package().canvas_height);
}

int SpriteLoopData::find_animation_index(const String& animation) const
{
    if (!asset()) {
        return -1;
    }
    return spriteloop_godot::find_animation_index(asset()->package(), to_std(animation));
}

PackedStringArray SpriteLoopData::get_animation_names() const
{
    PackedStringArray names;
    if (asset()) {
        for (const auto& animation : asset()->package().animations) {
            names.push_back(display_name(animation));
        }
    }
    return names;
}

bool SpriteLoopData::has_animation(const StringName& animation) const
{
    return find_animation_index(animation) >= 0;
}

int SpriteLoopData::get_animation_frame_count(const StringName& animation) const
{
    const int index = find_animation_index(animation);
    return index < 0 ? 0 : static_cast<int>(asset()->package().animations[index].frames.size());
}

double SpriteLoopData::get_animation_fps(const StringName& animation) const
{
    const int index = find_animation_index(animation);
    return index < 0 ? 0.0 : asset()->package().animations[index].fps;
}

double SpriteLoopData::get_animation_length(const StringName& animation) const
{
    const int index = find_animation_index(animation);
    if (index < 0) {
        return 0.0;
    }
    const auto& entry = asset()->package().animations[index];
    return entry.fps > 0.0f ? static_cast<double>(entry.frames.size()) / entry.fps : 0.0;
}

bool SpriteLoopData::is_animation_looping(const StringName& animation) const
{
    const int index = find_animation_index(animation);
    return index >= 0 && asset()->package().animations[index].loop;
}

PackedStringArray SpriteLoopData::get_skin_names() const
{
    PackedStringArray names;
    if (asset()) {
        for (const auto& skin : asset()->package().skins) {
            names.push_back(display_name(skin));
        }
    }
    return names;
}

PackedStringArray SpriteLoopData::get_part_names() const
{
    PackedStringArray names;
    if (asset()) {
        for (const auto& part : asset()->package().parts) {
            names.push_back(display_name(part));
        }
    }
    return names;
}

PackedStringArray SpriteLoopData::get_variant_names(const StringName& part) const
{
    PackedStringArray names;
    if (!asset()) {
        return names;
    }
    const auto& package = asset()->package();
    const int part_index = spriteloop::find_part_index_by_id_key_or_name(package, to_std(part));
    if (part_index < 0) {
        return names;
    }
    for (const auto& variant : package.variants) {
        if (variant.part_index == part_index) {
            names.push_back(display_name(variant));
        }
    }
    return names;
}

} // namespace godot

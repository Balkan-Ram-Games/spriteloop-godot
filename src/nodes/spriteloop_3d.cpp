#include "nodes/spriteloop_3d.h"

#include "core/stats.h"
#include "nodes/spriteloop_attachment.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <cmath>

namespace godot {

#define SPRITELOOP_NODE SpriteLoop3D
#include "nodes/playback_api_impl.inc"
#undef SPRITELOOP_NODE

SpriteLoop3D::SpriteLoop3D()
{
    ++spriteloop_godot::stats().nodes;
}

SpriteLoop3D::~SpriteLoop3D()
{
    --spriteloop_godot::stats().nodes;
}

void SpriteLoop3D::_bind_methods()
{
    _bind_playback_api();

    ClassDB::bind_method(D_METHOD("set_pixel_size", "pixel_size"), &SpriteLoop3D::set_pixel_size);
    ClassDB::bind_method(D_METHOD("get_pixel_size"), &SpriteLoop3D::get_pixel_size);
    ClassDB::bind_method(D_METHOD("set_offset", "offset"), &SpriteLoop3D::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset"), &SpriteLoop3D::get_offset);
    ClassDB::bind_method(D_METHOD("set_modulate", "modulate"), &SpriteLoop3D::set_modulate);
    ClassDB::bind_method(D_METHOD("get_modulate"), &SpriteLoop3D::get_modulate);
    ClassDB::bind_method(D_METHOD("set_flip_h", "flip"), &SpriteLoop3D::set_flip_h);
    ClassDB::bind_method(D_METHOD("is_flipped_h"), &SpriteLoop3D::is_flipped_h);
    ClassDB::bind_method(D_METHOD("set_flip_v", "flip"), &SpriteLoop3D::set_flip_v);
    ClassDB::bind_method(D_METHOD("is_flipped_v"), &SpriteLoop3D::is_flipped_v);
    ClassDB::bind_method(D_METHOD("set_billboard_mode", "mode"), &SpriteLoop3D::set_billboard_mode);
    ClassDB::bind_method(D_METHOD("get_billboard_mode"), &SpriteLoop3D::get_billboard_mode);
    ClassDB::bind_method(D_METHOD("set_shaded", "shaded"), &SpriteLoop3D::set_shaded);
    ClassDB::bind_method(D_METHOD("is_shaded"), &SpriteLoop3D::is_shaded);
    ClassDB::bind_method(D_METHOD("set_double_sided", "double_sided"), &SpriteLoop3D::set_double_sided);
    ClassDB::bind_method(D_METHOD("is_double_sided"), &SpriteLoop3D::is_double_sided);
    ClassDB::bind_method(D_METHOD("set_no_depth_test", "enabled"), &SpriteLoop3D::set_no_depth_test);
    ClassDB::bind_method(D_METHOD("is_no_depth_test"), &SpriteLoop3D::is_no_depth_test);
    ClassDB::bind_method(D_METHOD("set_alpha_cut_mode", "mode"), &SpriteLoop3D::set_alpha_cut_mode);
    ClassDB::bind_method(D_METHOD("get_alpha_cut_mode"), &SpriteLoop3D::get_alpha_cut_mode);
    ClassDB::bind_method(D_METHOD("set_alpha_scissor_threshold", "threshold"),
                         &SpriteLoop3D::set_alpha_scissor_threshold);
    ClassDB::bind_method(D_METHOD("get_alpha_scissor_threshold"), &SpriteLoop3D::get_alpha_scissor_threshold);
    ClassDB::bind_method(D_METHOD("set_texture_filter", "filter"), &SpriteLoop3D::set_texture_filter);
    ClassDB::bind_method(D_METHOD("get_texture_filter"), &SpriteLoop3D::get_texture_filter);
    ClassDB::bind_method(D_METHOD("set_render_priority", "priority"), &SpriteLoop3D::set_render_priority);
    ClassDB::bind_method(D_METHOD("get_render_priority"), &SpriteLoop3D::get_render_priority);
    ClassDB::bind_method(D_METHOD("get_part_transform", "part", "origin"), &SpriteLoop3D::get_part_transform,
                         DEFVAL(PART_ORIGIN_PIVOT));
    ClassDB::bind_method(D_METHOD("set_material", "material"), &SpriteLoop3D::set_material);
    ClassDB::bind_method(D_METHOD("get_material"), &SpriteLoop3D::get_material);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixel_size", PROPERTY_HINT_RANGE, "0.0001,128,0.0001,suffix:m"),
                 "set_pixel_size", "get_pixel_size");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset", PROPERTY_HINT_NONE, "suffix:px"), "set_offset",
                 "get_offset");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "modulate"), "set_modulate", "get_modulate");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_h"), "set_flip_h", "is_flipped_h");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_v"), "set_flip_v", "is_flipped_v");
    ADD_GROUP("Flags", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "billboard", PROPERTY_HINT_ENUM, "Disabled,Enabled,Y-Billboard"),
                 "set_billboard_mode", "get_billboard_mode");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shaded"), "set_shaded", "is_shaded");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "double_sided"), "set_double_sided", "is_double_sided");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "no_depth_test"), "set_no_depth_test", "is_no_depth_test");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alpha_cut", PROPERTY_HINT_ENUM,
                              "Disabled,Discard,Opaque Pre-Pass,Alpha Hash"),
                 "set_alpha_cut_mode", "get_alpha_cut_mode");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "alpha_scissor_threshold", PROPERTY_HINT_RANGE, "0,1,0.01"),
                 "set_alpha_scissor_threshold", "get_alpha_scissor_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "texture_filter", PROPERTY_HINT_ENUM, "Nearest,Linear,Linear Mipmap"),
                 "set_texture_filter", "get_texture_filter");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "render_priority", PROPERTY_HINT_RANGE, "-128,127,1"),
                 "set_render_priority", "get_render_priority");
    ADD_GROUP("Material", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"),
                 "set_material", "get_material");

    BIND_ENUM_CONSTANT(BILLBOARD_DISABLED);
    BIND_ENUM_CONSTANT(BILLBOARD_ENABLED);
    BIND_ENUM_CONSTANT(BILLBOARD_FIXED_Y);
    BIND_ENUM_CONSTANT(ALPHA_CUT_DISABLED);
    BIND_ENUM_CONSTANT(ALPHA_CUT_DISCARD);
    BIND_ENUM_CONSTANT(ALPHA_CUT_OPAQUE_PREPASS);
    BIND_ENUM_CONSTANT(ALPHA_CUT_HASH);
    BIND_ENUM_CONSTANT(TEXTURE_FILTER_NEAREST);
    BIND_ENUM_CONSTANT(TEXTURE_FILTER_LINEAR);
    BIND_ENUM_CONSTANT(TEXTURE_FILTER_LINEAR_WITH_MIPMAPS);
}

void SpriteLoop3D::_notification(int what)
{
    _playback_notification(what);
}

void SpriteLoop3D::_validate_property(PropertyInfo& property) const
{
    _validate_playback_property(property);
    if (property.name == StringName("alpha_scissor_threshold") &&
        options_.alpha_cut != spriteloop_godot::Material3DOptions::alpha_discard) {
        property.usage = PROPERTY_USAGE_NO_EDITOR;
    }
    // A custom material's shader declares its own render modes.
    if (options_.custom.is_valid()) {
        const String name = property.name;
        if (name == "shaded" || name == "double_sided" || name == "no_depth_test" || name == "alpha_cut") {
            property.usage |= PROPERTY_USAGE_READ_ONLY;
        }
    }
}

// ---------------------------------------------------------------------------
// Rendering

void SpriteLoop3D::update_mesh()
{
    // A frame with masks is composed once into a shared texture (texture
    // mode) and drawn on a quad; other frames draw their mesh with the atlas.
    // Packages without masks skip the lookup.
    const auto& asset = playback_.asset();
    const bool masks = asset && asset->has_masks();
    // texture_filter 0 is nearest.
    mask_frame_ = masks ? playback_.current_mask_frame(options_.texture_filter == 0) : nullptr;
    RID mesh;
    Ref<Texture2D> texture;
    if (mask_frame_) {
        mesh = mask_frame_->quad_mesh_3d(pixel_size_);
        texture = mask_frame_->texture_resource();
    } else {
        mesh = playback_.current_mesh_3d(pixel_size_);
        if (asset) {
            texture = asset->texture();
        }
    }
    const bool texture_changed = texture != current_texture_;
    if (mesh == current_mesh_ && !texture_changed) {
        return;
    }
    current_mesh_ = mesh;
    set_base(mesh);
    if (texture_changed) {
        current_texture_ = texture;
        update_material();
    } else {
        // The instance has a material slot only while it has a mesh base.
        apply_material();
    }
    ++spriteloop_godot::stats().redraws;
}

void SpriteLoop3D::update_material()
{
    material_ = current_texture_.is_valid() ? spriteloop_godot::material_3d(current_texture_, options_)
                                            : Ref<ShaderMaterial>();
    apply_material();
}

void SpriteLoop3D::apply_material()
{
    if (!current_mesh_.is_valid()) {
        return;
    }
    RenderingServer::get_singleton()->instance_set_surface_override_material(
        get_instance(), 0, material_.is_valid() ? material_->get_rid() : RID());
}

AABB SpriteLoop3D::bounds_aabb() const
{
    // Bounds of every frame of the active skin, so the box does not jitter.
    const Rect2 rect = get_local_rect();
    if (rect.size.x <= 0.0f || rect.size.y <= 0.0f) {
        return AABB();
    }
    const Vector2 begin = rect.position;
    const Vector2 end = rect.get_end();
    switch (options_.billboard) {
    case spriteloop_godot::Material3DOptions::billboard_enabled: {
        // The character turns to face the camera around the node origin, so a
        // point at distance d from the origin can reach d along any axis. The
        // tightest box covering every orientation uses the farthest corner.
        float reach = 0.0f;
        for (const Vector2& corner : {begin, end, Vector2(begin.x, end.y), Vector2(end.x, begin.y)}) {
            reach = std::max(reach, corner.length());
        }
        return AABB(Vector3(-reach, -reach, -reach), Vector3(reach, reach, reach) * 2.0f);
    }
    case spriteloop_godot::Material3DOptions::billboard_fixed_y: {
        // Turns around the Y axis only: keep the height, widen X and Z by the
        // largest horizontal extent (as Sprite3D does).
        const float reach = std::max(std::abs(begin.x), std::abs(end.x));
        return AABB(Vector3(-reach, begin.y, -reach), Vector3(reach * 2.0f, rect.size.y, reach * 2.0f));
    }
    default:
        return AABB(Vector3(begin.x, begin.y, 0.0f), Vector3(rect.size.x, rect.size.y, 0.0f));
    }
}

Rect2 SpriteLoop3D::get_local_rect() const
{
    const spriteloop::SplaBounds* bounds = playback_.bounds();
    if (bounds == nullptr) {
        return Rect2();
    }
    // Offset first, then the flips mirror around the node origin (as in the shader).
    float x0 = bounds->min_x + offset_.x;
    float x1 = bounds->max_x + offset_.x;
    float y0 = bounds->min_y + offset_.y;
    float y1 = bounds->max_y + offset_.y;
    if (options_.flip_h) {
        const float left = -x1;
        x1 = -x0;
        x0 = left;
    }
    if (options_.flip_v) {
        const float bottom = -y1;
        y1 = -y0;
        y0 = bottom;
    }
    return Rect2(x0 * pixel_size_, y0 * pixel_size_, (x1 - x0) * pixel_size_, (y1 - y0) * pixel_size_);
}

AABB SpriteLoop3D::_get_aabb() const
{
    return bounds_aabb();
}

void SpriteLoop3D::playback_visual_changed()
{
    update_mesh();
    // The pose changed: attachments follow now, never a frame late.
    update_attachments();
}

void SpriteLoop3D::update_attachments()
{
    for (int i = 0; i < get_child_count(); ++i) {
        if (SpriteLoopAttachment3D* attachment = Object::cast_to<SpriteLoopAttachment3D>(get_child(i))) {
            attachment->update_from_parent();
        }
    }
}

void SpriteLoop3D::playback_bounds_changed()
{
    // Cull by the bounds of every frame (and every billboard orientation), not
    // by the current frame's mesh, so the culling box does not jitter.
    RenderingServer::get_singleton()->instance_set_custom_aabb(get_instance(), bounds_aabb());
    update_material();
    update_gizmos();
}

// ---------------------------------------------------------------------------
// Properties

void SpriteLoop3D::set_pixel_size(float pixel_size)
{
    pixel_size = std::max(0.0001f, pixel_size);
    if (pixel_size == pixel_size_) {
        return;
    }
    pixel_size_ = pixel_size;
    options_.offset = offset_ * pixel_size_;
    update_mesh();
    playback_bounds_changed();
    update_attachments();
}

void SpriteLoop3D::set_offset(const Vector2& offset)
{
    if (offset == offset_) {
        return;
    }
    offset_ = offset;
    options_.offset = offset_ * pixel_size_;
    playback_bounds_changed();
    update_attachments();
}

Vector2 SpriteLoop3D::get_offset() const
{
    return offset_;
}

float SpriteLoop3D::get_pixel_size() const
{
    return pixel_size_;
}

void SpriteLoop3D::set_modulate(const Color& modulate)
{
    options_.modulate = modulate;
    update_material();
}

Color SpriteLoop3D::get_modulate() const
{
    return options_.modulate;
}

void SpriteLoop3D::set_flip_h(bool flip)
{
    options_.flip_h = flip;
    playback_bounds_changed();
    update_attachments();
}

bool SpriteLoop3D::is_flipped_h() const
{
    return options_.flip_h;
}

void SpriteLoop3D::set_flip_v(bool flip)
{
    options_.flip_v = flip;
    playback_bounds_changed();
    update_attachments();
}

bool SpriteLoop3D::is_flipped_v() const
{
    return options_.flip_v;
}

void SpriteLoop3D::set_billboard_mode(BillboardMode mode)
{
    options_.billboard = static_cast<spriteloop_godot::Material3DOptions::Billboard>(mode);
    playback_bounds_changed();
    update_attachments();
}

SpriteLoop3D::BillboardMode SpriteLoop3D::get_billboard_mode() const
{
    return static_cast<BillboardMode>(options_.billboard);
}

void SpriteLoop3D::set_shaded(bool shaded)
{
    options_.shaded = shaded;
    update_material();
}

bool SpriteLoop3D::is_shaded() const
{
    return options_.shaded;
}

void SpriteLoop3D::set_double_sided(bool double_sided)
{
    options_.double_sided = double_sided;
    update_material();
}

bool SpriteLoop3D::is_double_sided() const
{
    return options_.double_sided;
}

void SpriteLoop3D::set_no_depth_test(bool enabled)
{
    options_.no_depth_test = enabled;
    update_material();
}

bool SpriteLoop3D::is_no_depth_test() const
{
    return options_.no_depth_test;
}

void SpriteLoop3D::set_alpha_cut_mode(AlphaCutMode mode)
{
    options_.alpha_cut = static_cast<spriteloop_godot::Material3DOptions::AlphaCut>(mode);
    update_material();
    notify_property_list_changed();
}

SpriteLoop3D::AlphaCutMode SpriteLoop3D::get_alpha_cut_mode() const
{
    return static_cast<AlphaCutMode>(options_.alpha_cut);
}

void SpriteLoop3D::set_alpha_scissor_threshold(float threshold)
{
    options_.alpha_scissor_threshold = std::clamp(threshold, 0.0f, 1.0f);
    update_material();
}

float SpriteLoop3D::get_alpha_scissor_threshold() const
{
    return options_.alpha_scissor_threshold;
}

void SpriteLoop3D::set_texture_filter(TextureFilter filter)
{
    options_.texture_filter = static_cast<std::uint8_t>(std::clamp(static_cast<int>(filter), 0, 2));
    update_material();
    // Masked frames are composed with nearest or linear filtering.
    update_mesh();
}

SpriteLoop3D::TextureFilter SpriteLoop3D::get_texture_filter() const
{
    return static_cast<TextureFilter>(options_.texture_filter);
}

void SpriteLoop3D::set_render_priority(int priority)
{
    options_.render_priority = std::clamp(priority, -128, 127);
    update_material();
}

int SpriteLoop3D::get_render_priority() const
{
    return options_.render_priority;
}

void SpriteLoop3D::set_material(const Ref<ShaderMaterial>& material)
{
    if (options_.custom == material) {
        return;
    }
    const Callable on_changed = callable_mp(this, &SpriteLoop3D::on_custom_material_changed);
    if (options_.custom.is_valid() && options_.custom->is_connected("changed", on_changed)) {
        options_.custom->disconnect("changed", on_changed);
    }
    options_.custom = material;
    if (options_.custom.is_valid()) {
        // Deferred: runs after the material cache has registered the change.
        options_.custom->connect("changed", on_changed, CONNECT_DEFERRED);
    }
    update_material();
    update_configuration_warnings();
    notify_property_list_changed();
}

Ref<ShaderMaterial> SpriteLoop3D::get_material() const
{
    return options_.custom;
}

void SpriteLoop3D::on_custom_material_changed()
{
    update_material();
    update_configuration_warnings();
}

PackedStringArray SpriteLoop3D::_get_configuration_warnings() const
{
    PackedStringArray warnings;
    if (options_.custom.is_valid()) {
        if (options_.custom->get_shader().is_null()) {
            warnings.push_back("Material has no shader, so the built-in look is used. Give it a spatial shader "
                               "that includes res://addons/spriteloop/shaders/spriteloop_3d.gdshaderinc.");
        }
        if (get_material_override().is_valid()) {
            warnings.push_back("Material Override is set, so Material is not used. Material Override replaces "
                               "every material, including SpriteLoop layering, flips and billboarding.");
        }
    }
    return warnings;
}

// ---------------------------------------------------------------------------
// Parts

bool SpriteLoop3D::find_part_transform(const String& part, bool center, Transform3D& transform,
                                       bool warn) const
{
    Transform2D y_up;
    if (!playback_.get_part_transform_y_up(part, center, y_up, warn)) {
        return false;
    }
    const float fx = options_.flip_h ? -1.0f : 1.0f;
    const float fy = options_.flip_v ? -1.0f : 1.0f;
    // Offset first, then conjugate with the flip S = diag(fx, fy, 1):
    // basis S*M*S, origin S*(o + offset).
    const Basis basis(Vector3(y_up.columns[0].x, fx * fy * y_up.columns[0].y, 0.0f),
                      Vector3(fx * fy * y_up.columns[1].x, y_up.columns[1].y, 0.0f),
                      Vector3(0.0f, 0.0f, 1.0f));
    const Vector3 position(fx * (y_up.columns[2].x + offset_.x) * pixel_size_,
                           fy * (y_up.columns[2].y + offset_.y) * pixel_size_, 0.0f);
    transform = Transform3D(basis, position);
    return true;
}

Transform3D SpriteLoop3D::get_part_transform(const String& part, PartOrigin origin) const
{
    Transform3D transform;
    if (!find_part_transform(part, origin == PART_ORIGIN_CENTER, transform, true)) {
        return Transform3D();
    }
    return transform;
}

} // namespace godot

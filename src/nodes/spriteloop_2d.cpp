#include "nodes/spriteloop_2d.h"

#include "core/stats.h"
#include "nodes/spriteloop_attachment.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>

namespace godot {

#define SPRITELOOP_NODE SpriteLoop2D
#include "nodes/playback_api_impl.inc"
#undef SPRITELOOP_NODE

SpriteLoop2D::SpriteLoop2D()
{
    ++spriteloop_godot::stats().nodes;
}

SpriteLoop2D::~SpriteLoop2D()
{
    --spriteloop_godot::stats().nodes;
}

void SpriteLoop2D::_bind_methods()
{
    _bind_playback_api();
    ClassDB::bind_method(D_METHOD("set_offset", "offset"), &SpriteLoop2D::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset"), &SpriteLoop2D::get_offset);
    ClassDB::bind_method(D_METHOD("get_part_transform", "part", "origin"), &SpriteLoop2D::get_part_transform,
                         DEFVAL(PART_ORIGIN_PIVOT));
    ClassDB::bind_method(D_METHOD("get_rect"), &SpriteLoop2D::get_rect);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset", PROPERTY_HINT_NONE, "suffix:px"), "set_offset",
                 "get_offset");
}

void SpriteLoop2D::_notification(int what)
{
    _playback_notification(what);
    if (what == NOTIFICATION_DRAW) {
        draw_current_frame();
    }
}

void SpriteLoop2D::_validate_property(PropertyInfo& property) const
{
    _validate_playback_property(property);
}

bool SpriteLoop2D::uses_nearest_filter() const
{
    for (const CanvasItem* item = this; item != nullptr; item = Object::cast_to<CanvasItem>(item->get_parent())) {
        switch (item->get_texture_filter()) {
        case TEXTURE_FILTER_PARENT_NODE:
            continue;
        case TEXTURE_FILTER_NEAREST:
        case TEXTURE_FILTER_NEAREST_WITH_MIPMAPS:
        case TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC:
            return true;
        default:
            return false;
        }
    }
    // rendering/textures/canvas_textures/default_texture_filter: 0 is Nearest,
    // 3 is Nearest Mipmap.
    const int filter = ProjectSettings::get_singleton()->get_setting(
        "rendering/textures/canvas_textures/default_texture_filter", 1);
    return filter == 0 || filter == 3;
}

void SpriteLoop2D::draw_current_frame()
{
    RenderingServer* rs = RenderingServer::get_singleton();
    // Packages without masks skip the filter lookup: no cost for unmasked crowds.
    const bool masks = playback_.asset() && playback_.asset()->has_masks();
    mask_frame_ = masks ? playback_.current_mask_frame(uses_nearest_filter()) : nullptr;
    if (mask_frame_) {
        Rect2 rect = mask_frame_->rect();
        rect.position += offset_;
        rs->canvas_item_add_texture_rect(get_canvas_item(), rect, mask_frame_->texture());
        ++spriteloop_godot::stats().redraws;
        return;
    }
    const RID mesh = playback_.current_mesh_2d();
    if (!mesh.is_valid()) {
        return;
    }
    rs->canvas_item_add_mesh(get_canvas_item(), mesh, Transform2D(0.0f, offset_), Color(1, 1, 1, 1),
                             playback_.texture_rid());
    ++spriteloop_godot::stats().redraws;
}

void SpriteLoop2D::update_attachments()
{
    for (int i = 0; i < get_child_count(); ++i) {
        if (SpriteLoopAttachment2D* attachment = Object::cast_to<SpriteLoopAttachment2D>(get_child(i))) {
            attachment->update_from_parent();
        }
    }
}

void SpriteLoop2D::playback_visual_changed()
{
    queue_redraw();
    // The pose changed (frame, animation, seek or package): attachments follow
    // now, so they never lag a frame behind.
    update_attachments();
}

void SpriteLoop2D::playback_bounds_changed()
{
    // Godot culls canvas items by this rect; it covers every frame of the skin.
    const bool has_bounds = playback_.bounds() != nullptr;
    RenderingServer::get_singleton()->canvas_item_set_custom_rect(get_canvas_item(), has_bounds, get_rect());
    // The same signal Sprite2D emits when its rect changes; the 2D editor
    // plugin uses it to keep picking and handles on the bounds.
    emit_signal("item_rect_changed");
}

void SpriteLoop2D::set_offset(const Vector2& offset)
{
    if (offset == offset_) {
        return;
    }
    offset_ = offset;
    queue_redraw();
    playback_bounds_changed();
    update_attachments();
}

Vector2 SpriteLoop2D::get_offset() const
{
    return offset_;
}

bool SpriteLoop2D::find_part_transform(const String& part, bool center, Transform2D& transform,
                                       bool warn) const
{
    if (!playback_.get_part_transform_2d(part, center, transform, warn)) {
        return false;
    }
    transform.columns[2] += offset_;
    return true;
}

Transform2D SpriteLoop2D::get_part_transform(const String& part, PartOrigin origin) const
{
    Transform2D transform;
    if (!find_part_transform(part, origin == PART_ORIGIN_CENTER, transform, true)) {
        return Transform2D();
    }
    return transform;
}

Rect2 SpriteLoop2D::get_rect() const
{
    Rect2 rect = playback_.bounds_2d();
    rect.position += offset_;
    return rect;
}

} // namespace godot

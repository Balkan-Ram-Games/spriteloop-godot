#include "nodes/spriteloop_2d.h"

#include "core/stats.h"

#include <godot_cpp/classes/engine.hpp>
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
    ClassDB::bind_method(D_METHOD("get_part_transform", "part", "origin"), &SpriteLoop2D::get_part_transform,
                         DEFVAL(PART_ORIGIN_PIVOT));
    ClassDB::bind_method(D_METHOD("get_rect"), &SpriteLoop2D::get_rect);
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

void SpriteLoop2D::draw_current_frame()
{
    const RID mesh = playback_.current_mesh_2d();
    if (!mesh.is_valid()) {
        return;
    }
    RenderingServer::get_singleton()->canvas_item_add_mesh(get_canvas_item(), mesh, Transform2D(),
                                                           Color(1, 1, 1, 1), playback_.texture_rid());
    ++spriteloop_godot::stats().redraws;
}

void SpriteLoop2D::playback_visual_changed()
{
    queue_redraw();
}

void SpriteLoop2D::playback_bounds_changed()
{
    // Godot culls canvas items by this rect; it covers every frame of the skin.
    const bool has_bounds = playback_.bounds() != nullptr;
    RenderingServer::get_singleton()->canvas_item_set_custom_rect(get_canvas_item(), has_bounds,
                                                                  playback_.bounds_2d());
    // The same signal Sprite2D emits when its rect changes; the 2D editor
    // plugin uses it to keep picking and handles on the bounds.
    emit_signal("item_rect_changed");
}

Transform2D SpriteLoop2D::get_part_transform(const String& part, PartOrigin origin) const
{
    Transform2D transform;
    playback_.get_part_transform_2d(part, origin == PART_ORIGIN_CENTER, transform, true);
    return transform;
}

Rect2 SpriteLoop2D::get_rect() const
{
    return playback_.bounds_2d();
}

} // namespace godot

#pragma once

#include "core/playback.h"
#include "spriteloop_data.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/transform2d.hpp>

namespace godot {

// Plays a SpriteLoop package in 2D.
//
// Draws the current baked frame as one shared mesh on the node's own canvas
// item. A frame with masks is composed once into a shared texture (see
// MaskFrame) and drawn as one textured rect instead. Commands are re-recorded only when the displayed frame, skin or
// package changes. Tint with modulate/self_modulate, filter with
// texture_filter, and use material for custom shaders as with any CanvasItem.
// Emits item_rect_changed when its bounds change. Editor picking and handles
// are provided by SpriteLoop2DEditorPlugin.
class SpriteLoop2D : public Node2D, public spriteloop_godot::PlaybackHost {
    GDCLASS(SpriteLoop2D, Node2D)

#define SPRITELOOP_NODE SpriteLoop2D
#include "nodes/playback_api_decl.inc"
#undef SPRITELOOP_NODE

public:
    SpriteLoop2D();
    ~SpriteLoop2D() override;

    // Moves the drawn character relative to the node origin (pixels, Y down),
    // e.g. to put the origin at the character's feet.
    void set_offset(const Vector2& offset);
    Vector2 get_offset() const;

    // Current transform of a part in this node's local space (pixels, Y down,
    // offset included). Returns the identity transform and warns if the part is
    // not shown.
    Transform2D get_part_transform(const String& part, PartOrigin origin = PART_ORIGIN_PIVOT) const;
    // C++ form for attachments: false if the part is not in the current frame.
    bool find_part_transform(const String& part, bool center, Transform2D& transform, bool warn) const;
    // Bounds of every frame of the active skin, in local space.
    Rect2 get_rect() const;

    // PlaybackHost
    void playback_visual_changed() override;
    void playback_bounds_changed() override;
    Object* playback_owner() override { return this; }

protected:
    static void _bind_methods();
    void _notification(int what);
    void _validate_property(PropertyInfo& property) const;

private:
    void draw_current_frame();
    void update_attachments();
    // Nearest filtering for this node's texture (texture_filter, inherited
    // from parents, or the project default).
    bool uses_nearest_filter() const;

    Vector2 offset_;
    // The composed current frame when it has masks (kept alive while shown).
    std::shared_ptr<spriteloop_godot::MaskFrame> mask_frame_;
};

} // namespace godot

VARIANT_ENUM_CAST(SpriteLoop2D::LoopMode);
VARIANT_ENUM_CAST(SpriteLoop2D::ProcessCallback);
VARIANT_ENUM_CAST(SpriteLoop2D::PartOrigin);

#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace godot {

// Follows a part of its parent SpriteLoop2D, like Godot's BoneAttachment2D:
// children (weapons, hitboxes, particles) move with the part. The parent
// updates it whenever the displayed pose changes, so it never lags a frame.
// If the part is not in the current frame, the last transform is kept.
class SpriteLoopAttachment2D : public Node2D {
    GDCLASS(SpriteLoopAttachment2D, Node2D)

public:
    enum Origin {
        ORIGIN_PIVOT,
        ORIGIN_CENTER,
    };

    void set_part(const StringName& part);
    StringName get_part() const;
    void set_origin(Origin origin);
    Origin get_origin() const;
    void set_follow_rotation(bool enabled);
    bool is_following_rotation() const;
    void set_follow_scale(bool enabled);
    bool is_following_scale() const;

    // Moves to the part's current transform. Called by the parent SpriteLoop2D.
    void update_from_parent();

    PackedStringArray _get_configuration_warnings() const override;

protected:
    static void _bind_methods();
    void _notification(int what);
    void _validate_property(PropertyInfo& property) const;

private:
    StringName part_;
    Origin origin_ = ORIGIN_PIVOT;
    bool follow_rotation_ = true;
    bool follow_scale_ = true;
};

class SpriteLoop3D;

// 3D counterpart for SpriteLoop3D. Follows the part in the parent's local XY
// plane (pixel_size, offset and flips applied). For a billboarded parent it
// also follows the turn towards the camera, updated just before each frame is
// drawn (in the editor, towards the first 3D viewport's camera).
class SpriteLoopAttachment3D : public Node3D {
    GDCLASS(SpriteLoopAttachment3D, Node3D)

public:
    enum Origin {
        ORIGIN_PIVOT,
        ORIGIN_CENTER,
    };

    void set_part(const StringName& part);
    StringName get_part() const;
    void set_origin(Origin origin);
    Origin get_origin() const;
    void set_follow_rotation(bool enabled);
    bool is_following_rotation() const;
    void set_follow_scale(bool enabled);
    bool is_following_scale() const;

    // Moves to the part's current transform. Called by the parent SpriteLoop3D.
    void update_from_parent();

    PackedStringArray _get_configuration_warnings() const override;

protected:
    static void _bind_methods();
    void _notification(int what);
    void _validate_property(PropertyInfo& property) const;

private:
    // Rotation (with the node's scale) the shader gives a billboarded parent,
    // or false if it is not billboarded or there is no camera.
    bool billboard_basis(const SpriteLoop3D* parent, Basis& basis) const;
    void update_billboard();

    StringName part_;
    Origin origin_ = ORIGIN_PIVOT;
    bool follow_rotation_ = true;
    bool follow_scale_ = true;
};

} // namespace godot

VARIANT_ENUM_CAST(SpriteLoopAttachment2D::Origin);
VARIANT_ENUM_CAST(SpriteLoopAttachment3D::Origin);

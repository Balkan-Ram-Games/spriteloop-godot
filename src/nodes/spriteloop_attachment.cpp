#include "nodes/spriteloop_attachment.h"

#include "nodes/spriteloop_2d.h"
#include "nodes/spriteloop_3d.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#ifdef SPRITELOOP_EDITOR
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#endif

#include <cmath>

namespace godot {

namespace {

// Shared by both attachment classes: bindings, inspector hint and warnings.
template <typename Attachment>
void bind_attachment_api()
{
    ClassDB::bind_method(D_METHOD("set_part", "part"), &Attachment::set_part);
    ClassDB::bind_method(D_METHOD("get_part"), &Attachment::get_part);
    ClassDB::bind_method(D_METHOD("set_origin", "origin"), &Attachment::set_origin);
    ClassDB::bind_method(D_METHOD("get_origin"), &Attachment::get_origin);
    ClassDB::bind_method(D_METHOD("set_follow_rotation", "enabled"), &Attachment::set_follow_rotation);
    ClassDB::bind_method(D_METHOD("is_following_rotation"), &Attachment::is_following_rotation);
    ClassDB::bind_method(D_METHOD("set_follow_scale", "enabled"), &Attachment::set_follow_scale);
    ClassDB::bind_method(D_METHOD("is_following_scale"), &Attachment::is_following_scale);

    const StringName name = Attachment::get_class_static();
    ClassDB::add_property(name, PropertyInfo(Variant::STRING_NAME, "part", PROPERTY_HINT_ENUM_SUGGESTION, ""),
                          "set_part", "get_part");
    ClassDB::add_property(name, PropertyInfo(Variant::INT, "origin", PROPERTY_HINT_ENUM, "Pivot,Center"),
                          "set_origin", "get_origin");
    ClassDB::add_property(name, PropertyInfo(Variant::BOOL, "follow_rotation"), "set_follow_rotation",
                          "is_following_rotation");
    ClassDB::add_property(name, PropertyInfo(Variant::BOOL, "follow_scale"), "set_follow_scale",
                          "is_following_scale");
}

template <typename Parent>
void part_hint(const Node* attachment, PropertyInfo& property)
{
    if (property.name != StringName("part")) {
        return;
    }
    const Parent* parent = Object::cast_to<Parent>(attachment->get_parent());
    if (parent == nullptr || parent->get_data().is_null() || !parent->get_data()->is_valid()) {
        return;
    }
    property.hint_string = String(",").join(parent->get_data()->get_part_names());
}

template <typename Parent>
PackedStringArray attachment_warnings(const Node* attachment, const StringName& part, const char* parent_name)
{
    PackedStringArray warnings;
    if (Object::cast_to<Parent>(attachment->get_parent()) == nullptr) {
        warnings.push_back(String("Must be a direct child of a ") + parent_name + ".");
    }
    if (part.is_empty()) {
        warnings.push_back("Choose the part to follow.");
    }
    return warnings;
}

} // namespace

// ---------------------------------------------------------------------------
// SpriteLoopAttachment2D

void SpriteLoopAttachment2D::_bind_methods()
{
    bind_attachment_api<SpriteLoopAttachment2D>();
    BIND_ENUM_CONSTANT(ORIGIN_PIVOT);
    BIND_ENUM_CONSTANT(ORIGIN_CENTER);
}

void SpriteLoopAttachment2D::_notification(int what)
{
    if (what == NOTIFICATION_ENTER_TREE) {
        update_from_parent();
        update_configuration_warnings();
    }
}

void SpriteLoopAttachment2D::_validate_property(PropertyInfo& property) const
{
    part_hint<SpriteLoop2D>(this, property);
}

PackedStringArray SpriteLoopAttachment2D::_get_configuration_warnings() const
{
    return attachment_warnings<SpriteLoop2D>(this, part_, "SpriteLoop2D");
}

void SpriteLoopAttachment2D::update_from_parent()
{
    const SpriteLoop2D* parent = Object::cast_to<SpriteLoop2D>(get_parent());
    Transform2D transform;
    if (parent == nullptr || part_.is_empty() ||
        !parent->find_part_transform(part_, origin_ == ORIGIN_CENTER, transform, false)) {
        return;
    }
    if (!follow_rotation_ || !follow_scale_) {
        transform = Transform2D(follow_rotation_ ? transform.get_rotation() : 0.0f,
                                follow_scale_ ? transform.get_scale() : Vector2(1, 1),
                                follow_scale_ ? transform.get_skew() : 0.0f, transform.get_origin());
    }
    set_transform(transform);
}

void SpriteLoopAttachment2D::set_part(const StringName& part)
{
    part_ = part;
    update_from_parent();
    update_configuration_warnings();
}

StringName SpriteLoopAttachment2D::get_part() const
{
    return part_;
}

void SpriteLoopAttachment2D::set_origin(Origin origin)
{
    origin_ = origin;
    update_from_parent();
}

SpriteLoopAttachment2D::Origin SpriteLoopAttachment2D::get_origin() const
{
    return origin_;
}

void SpriteLoopAttachment2D::set_follow_rotation(bool enabled)
{
    follow_rotation_ = enabled;
    update_from_parent();
}

bool SpriteLoopAttachment2D::is_following_rotation() const
{
    return follow_rotation_;
}

void SpriteLoopAttachment2D::set_follow_scale(bool enabled)
{
    follow_scale_ = enabled;
    update_from_parent();
}

bool SpriteLoopAttachment2D::is_following_scale() const
{
    return follow_scale_;
}

// ---------------------------------------------------------------------------
// SpriteLoopAttachment3D

void SpriteLoopAttachment3D::_bind_methods()
{
    bind_attachment_api<SpriteLoopAttachment3D>();
    BIND_ENUM_CONSTANT(ORIGIN_PIVOT);
    BIND_ENUM_CONSTANT(ORIGIN_CENTER);
}

void SpriteLoopAttachment3D::_notification(int what)
{
    const Callable on_pre_draw = callable_mp(this, &SpriteLoopAttachment3D::update_billboard);
    RenderingServer* rs = RenderingServer::get_singleton();
    if (what == NOTIFICATION_ENTER_TREE) {
        // Billboards turn towards the camera every frame; follow them just
        // before drawing, after game logic has moved the camera.
        if (!rs->is_connected("frame_pre_draw", on_pre_draw)) {
            rs->connect("frame_pre_draw", on_pre_draw);
        }
        update_from_parent();
        update_configuration_warnings();
    } else if (what == NOTIFICATION_EXIT_TREE) {
        if (rs->is_connected("frame_pre_draw", on_pre_draw)) {
            rs->disconnect("frame_pre_draw", on_pre_draw);
        }
    }
}

namespace {

// The camera whose view billboards face (MAIN_CAM_INV_VIEW_MATRIX in the
// shader): the viewport's camera in games, the first 3D editor viewport's
// camera in the editor.
Camera3D* billboard_camera(const Node* node)
{
#ifdef SPRITELOOP_EDITOR
    if (Engine::get_singleton()->is_editor_hint()) {
        if (SubViewport* viewport = EditorInterface::get_singleton()->get_editor_viewport_3d(0)) {
            return viewport->get_camera_3d();
        }
        return nullptr;
    }
#endif
    Viewport* viewport = node->get_viewport();
    return viewport != nullptr ? viewport->get_camera_3d() : nullptr;
}

} // namespace

bool SpriteLoopAttachment3D::billboard_basis(const SpriteLoop3D* parent, Basis& basis) const
{
    if (parent->get_billboard_mode() == SpriteLoop3D::BILLBOARD_DISABLED) {
        return false;
    }
    const Camera3D* camera = billboard_camera(this);
    if (camera == nullptr || !camera->is_inside_tree()) {
        return false;
    }
    // Same as SPRITELOOP_VERTEX: camera axes, keeping the node's scale.
    const Basis node_basis = parent->get_global_transform().basis;
    const Basis camera_basis = camera->get_global_transform().basis.orthonormalized();
    const Vector3 scale(node_basis.get_column(0).length(), node_basis.get_column(1).length(),
                        node_basis.get_column(2).length());
    Basis facing = camera_basis;
    if (parent->get_billboard_mode() == SpriteLoop3D::BILLBOARD_FIXED_Y) {
        const Vector3 up(0, 1, 0);
        facing = Basis(up.cross(camera_basis.get_column(2)).normalized(), up,
                       camera_basis.get_column(0).cross(up).normalized());
    }
    basis = facing * Basis::from_scale(scale);
    return true;
}

void SpriteLoopAttachment3D::update_billboard()
{
    const SpriteLoop3D* parent = Object::cast_to<SpriteLoop3D>(get_parent());
    if (parent != nullptr && parent->get_billboard_mode() != SpriteLoop3D::BILLBOARD_DISABLED) {
        update_from_parent();
    }
}

void SpriteLoopAttachment3D::_validate_property(PropertyInfo& property) const
{
    part_hint<SpriteLoop3D>(this, property);
}

PackedStringArray SpriteLoopAttachment3D::_get_configuration_warnings() const
{
    return attachment_warnings<SpriteLoop3D>(this, part_, "SpriteLoop3D");
}

void SpriteLoopAttachment3D::update_from_parent()
{
    const SpriteLoop3D* parent = Object::cast_to<SpriteLoop3D>(get_parent());
    Transform3D transform;
    if (parent == nullptr || part_.is_empty() ||
        !parent->find_part_transform(part_, origin_ == ORIGIN_CENTER, transform, false)) {
        return;
    }
    if (!follow_rotation_ || !follow_scale_) {
        // Parts move in the XY plane, so the rotation is around Z.
        const Vector3 x_axis = transform.basis.get_column(0);
        const real_t angle = follow_rotation_ ? std::atan2(x_axis.y, x_axis.x) : 0.0f;
        const Vector3 scale = follow_scale_ ? transform.basis.get_scale() : Vector3(1, 1, 1);
        transform.basis = Basis(Vector3(0, 0, 1), angle) * Basis::from_scale(scale);
    }
    Basis billboard;
    if (parent->is_inside_tree() && billboard_basis(parent, billboard)) {
        // The character is drawn turned towards the camera around the node
        // origin; place the part in that turned frame.
        set_global_transform(Transform3D(billboard, parent->get_global_transform().origin) * transform);
        return;
    }
    set_transform(transform);
}

void SpriteLoopAttachment3D::set_part(const StringName& part)
{
    part_ = part;
    update_from_parent();
    update_configuration_warnings();
}

StringName SpriteLoopAttachment3D::get_part() const
{
    return part_;
}

void SpriteLoopAttachment3D::set_origin(Origin origin)
{
    origin_ = origin;
    update_from_parent();
}

SpriteLoopAttachment3D::Origin SpriteLoopAttachment3D::get_origin() const
{
    return origin_;
}

void SpriteLoopAttachment3D::set_follow_rotation(bool enabled)
{
    follow_rotation_ = enabled;
    update_from_parent();
}

bool SpriteLoopAttachment3D::is_following_rotation() const
{
    return follow_rotation_;
}

void SpriteLoopAttachment3D::set_follow_scale(bool enabled)
{
    follow_scale_ = enabled;
    update_from_parent();
}

bool SpriteLoopAttachment3D::is_following_scale() const
{
    return follow_scale_;
}

} // namespace godot

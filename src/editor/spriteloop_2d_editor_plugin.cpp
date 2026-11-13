#include "editor/spriteloop_2d_editor_plugin.h"

#include "nodes/spriteloop_2d.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <cmath>

namespace godot {

namespace {

constexpr const char* pick_rect_name = "_SpriteLoopEditorPick";
constexpr float handle_grab_radius = 8.0f;
constexpr float minimum_scale = 0.001f;

// Handle order: top-left, top, top-right, right, bottom-right, bottom,
// bottom-left, left. The opposite handle is (index + 4) % 8.
bool handle_moves_x(int handle)
{
    return handle != 1 && handle != 5;
}

bool handle_moves_y(int handle)
{
    return handle != 3 && handle != 7;
}

float keep_away_from_zero(float value, float fallback_sign)
{
    if (std::abs(value) >= minimum_scale) {
        return value;
    }
    return fallback_sign < 0.0f ? -minimum_scale : minimum_scale;
}

} // namespace

// ---------------------------------------------------------------------------
// Lifetime

void SpriteLoop2DEditorPlugin::_enter_tree()
{
    get_tree()->connect("node_added", callable_mp(this, &SpriteLoop2DEditorPlugin::on_node_added));
}

void SpriteLoop2DEditorPlugin::_exit_tree()
{
    const Callable on_added = callable_mp(this, &SpriteLoop2DEditorPlugin::on_node_added);
    if (get_tree()->is_connected("node_added", on_added)) {
        get_tree()->disconnect("node_added", on_added);
    }
    remove_all_pick_rects();
}

// ---------------------------------------------------------------------------
// Picking

void SpriteLoop2DEditorPlugin::on_node_added(Node* node)
{
    SpriteLoop2D* sprite = Object::cast_to<SpriteLoop2D>(node);
    if (sprite == nullptr) {
        return;
    }
    // Deferred: the parent is still setting up its children here, and the
    // editor assigns the owner of a newly created node after adding it.
    callable_mp(this, &SpriteLoop2DEditorPlugin::attach_pick_rect).call_deferred(sprite->get_instance_id());
}

Control* SpriteLoop2DEditorPlugin::find_pick_rect(SpriteLoop2D* node)
{
    for (int i = 0; i < node->get_child_count(true); ++i) {
        Node* child = node->get_child(i, true);
        if (child->get_name() == StringName(pick_rect_name) && Object::cast_to<Control>(child) != nullptr) {
            return Object::cast_to<Control>(child);
        }
    }
    return nullptr;
}

void SpriteLoop2DEditorPlugin::attach_pick_rect(uint64_t node_id)
{
    SpriteLoop2D* node = Object::cast_to<SpriteLoop2D>(ObjectDB::get_instance(node_id));
    if (node == nullptr || !node->is_inside_tree()) {
        return;
    }

    // "Save Branch as Scene" packs nodes owned by the new root, which can
    // include the pick rect; such a saved copy loads as a regular child.
    for (int i = node->get_child_count(false) - 1; i >= 0; --i) {
        Node* child = node->get_child(i, false);
        if (Object::cast_to<Control>(child) != nullptr &&
            String(child->get_name()).begins_with(pick_rect_name)) {
            child->queue_free();
        }
    }

    Control* rect = find_pick_rect(node);
    if (rect == nullptr) {
        rect = memnew(Control);
        rect->set_name(pick_rect_name);
        rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        node->add_child(rect, false, Node::INTERNAL_MODE_FRONT);
        node->connect("item_rect_changed",
                      callable_mp(this, &SpriteLoop2DEditorPlugin::update_pick_rect).bind(node_id));
    }
    // A hit is kept only if it resolves to a node owned by the edited scene;
    // owned by the SpriteLoop2D, the rect resolves to it and is not saved with
    // the scene. A scene root has no owner and stays unpickable.
    rect->set_owner(node->get_owner() != nullptr ? node : nullptr);
    update_pick_rect(node_id);
}

void SpriteLoop2DEditorPlugin::update_pick_rect(uint64_t node_id)
{
    SpriteLoop2D* node = Object::cast_to<SpriteLoop2D>(ObjectDB::get_instance(node_id));
    if (node == nullptr) {
        return;
    }
    if (Control* rect = find_pick_rect(node)) {
        const Rect2 bounds = node->get_rect();
        rect->set_position(bounds.position);
        rect->set_size(bounds.size);
    }
    update_overlays();
}

void SpriteLoop2DEditorPlugin::remove_all_pick_rects()
{
    const auto remove = [this](Node* node, const auto& self) -> void {
        if (SpriteLoop2D* sprite = Object::cast_to<SpriteLoop2D>(node)) {
            if (Control* rect = find_pick_rect(sprite)) {
                rect->queue_free();
            }
            const Callable update =
                callable_mp(this, &SpriteLoop2DEditorPlugin::update_pick_rect).bind(sprite->get_instance_id());
            if (sprite->is_connected("item_rect_changed", update)) {
                sprite->disconnect("item_rect_changed", update);
            }
        }
        for (int i = 0; i < node->get_child_count(false); ++i) {
            self(node->get_child(i, false), self);
        }
    };
    if (Node* root = EditorInterface::get_singleton()->get_edited_scene_root()) {
        remove(root, remove);
    }
}

// ---------------------------------------------------------------------------
// Handles

bool SpriteLoop2DEditorPlugin::_handles(Object* object) const
{
    return Object::cast_to<SpriteLoop2D>(object) != nullptr;
}

void SpriteLoop2DEditorPlugin::_edit(Object* object)
{
    SpriteLoop2D* node = Object::cast_to<SpriteLoop2D>(object);
    edited_ = node != nullptr ? node->get_instance_id() : 0;
    drag_handle_ = -1;
    update_overlays();
}

SpriteLoop2D* SpriteLoop2DEditorPlugin::edited_node() const
{
    SpriteLoop2D* node = Object::cast_to<SpriteLoop2D>(ObjectDB::get_instance(edited_));
    if (node == nullptr || !node->is_inside_tree() || !node->is_visible_in_tree()) {
        return nullptr;
    }
    // Like the built-in handles, only for a single selected node.
    EditorSelection* selection = EditorInterface::get_singleton()->get_selection();
    if (selection == nullptr || selection->get_selected_nodes().size() != 1) {
        return nullptr;
    }
    const Rect2 rect = node->get_rect();
    return rect.size.x > 0.0f && rect.size.y > 0.0f ? node : nullptr;
}

void SpriteLoop2DEditorPlugin::handle_points(const Rect2& rect, Vector2 (&points)[8])
{
    const Vector2 begin = rect.position;
    const Vector2 end = rect.get_end();
    const Vector2 center = rect.get_center();
    points[0] = begin;
    points[1] = Vector2(center.x, begin.y);
    points[2] = Vector2(end.x, begin.y);
    points[3] = Vector2(end.x, center.y);
    points[4] = end;
    points[5] = Vector2(center.x, end.y);
    points[6] = Vector2(begin.x, end.y);
    points[7] = Vector2(begin.x, center.y);
}

Transform2D SpriteLoop2DEditorPlugin::local_to_screen(SpriteLoop2D* node)
{
    return node->get_viewport_transform() * node->get_global_transform();
}

Transform2D SpriteLoop2DEditorPlugin::parent_to_screen(SpriteLoop2D* node)
{
    return local_to_screen(node) * node->get_transform().affine_inverse();
}

void SpriteLoop2DEditorPlugin::_forward_canvas_draw_over_viewport(Control* overlay)
{
    SpriteLoop2D* node = edited_node();
    if (node == nullptr) {
        return;
    }
    const Transform2D transform = local_to_screen(node);
    Vector2 points[8];
    handle_points(node->get_rect(), points);

    PackedVector2Array outline;
    for (int corner : {0, 2, 4, 6, 0}) {
        outline.push_back(transform.xform(points[corner]));
    }
    overlay->draw_polyline(outline, Color(1.0f, 0.6f, 0.4f, 0.7f), 1.0f);

    Ref<Texture2D> icon;
    const Ref<Theme> theme = EditorInterface::get_singleton()->get_editor_theme();
    if (theme.is_valid() && theme->has_icon("EditorHandle", "EditorIcons")) {
        icon = theme->get_icon("EditorHandle", "EditorIcons");
    }
    for (const Vector2& point : points) {
        const Vector2 screen = transform.xform(point);
        if (icon.is_valid()) {
            overlay->draw_texture(icon, (screen - icon->get_size() * 0.5f).round());
        } else {
            overlay->draw_rect(Rect2(screen - Vector2(4, 4), Vector2(8, 8)), Color(1, 1, 1));
        }
    }
}

bool SpriteLoop2DEditorPlugin::_forward_canvas_gui_input(const Ref<InputEvent>& event)
{
    SpriteLoop2D* node = edited_node();
    if (node == nullptr) {
        drag_handle_ = -1;
        return false;
    }

    const Ref<InputEventMouseButton> button = event;
    if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
        if (button->is_pressed() && drag_handle_ < 0) {
            const Transform2D transform = local_to_screen(node);
            Vector2 points[8];
            handle_points(node->get_rect(), points);
            for (int i = 0; i < 8; ++i) {
                if (transform.xform(points[i]).distance_to(button->get_position()) <= handle_grab_radius) {
                    drag_handle_ = i;
                    drag_start_scale_ = node->get_scale();
                    drag_start_position_ = node->get_position();
                    return true;
                }
            }
            return false;
        }
        if (!button->is_pressed() && drag_handle_ >= 0) {
            drag_handle_ = -1;
            EditorUndoRedoManager* undo_redo = get_undo_redo();
            undo_redo->create_action("Scale SpriteLoop2D");
            undo_redo->add_do_property(node, "scale", node->get_scale());
            undo_redo->add_do_property(node, "position", node->get_position());
            undo_redo->add_undo_property(node, "scale", drag_start_scale_);
            undo_redo->add_undo_property(node, "position", drag_start_position_);
            undo_redo->commit_action(false);
            update_overlays();
            return true;
        }
        return false;
    }

    const Ref<InputEventMouseMotion> motion = event;
    if (motion.is_valid() && drag_handle_ >= 0) {
        // Scale so the opposite handle stays fixed and the dragged one follows
        // the mouse along the node's own axes, as for a Sprite2D.
        Vector2 points[8];
        handle_points(node->get_rect(), points);
        const Vector2 handle = points[drag_handle_];
        const Vector2 anchor = points[(drag_handle_ + 4) % 8];
        const Vector2 mouse = parent_to_screen(node).affine_inverse().xform(motion->get_position());
        const Transform2D axes(node->get_rotation(), Vector2(1, 1), node->get_skew(), Vector2());
        const Vector2 anchor_parent = drag_start_position_ + axes.basis_xform(drag_start_scale_ * anchor);
        const Vector2 offset = axes.affine_inverse().basis_xform(mouse - anchor_parent);
        const Vector2 span = handle - anchor;

        Vector2 scale = drag_start_scale_;
        if (handle_moves_x(drag_handle_) && std::abs(span.x) > 0.0001f) {
            scale.x = offset.x / span.x;
        }
        if (handle_moves_y(drag_handle_) && std::abs(span.y) > 0.0001f) {
            scale.y = offset.y / span.y;
        }
        if (motion->is_shift_pressed()) {
            // Keep the aspect ratio, following the axis that changed most.
            const float fx = drag_start_scale_.x != 0.0f ? scale.x / drag_start_scale_.x : 1.0f;
            const float fy = drag_start_scale_.y != 0.0f ? scale.y / drag_start_scale_.y : 1.0f;
            float factor = fx;
            if (!handle_moves_x(drag_handle_) ||
                (handle_moves_y(drag_handle_) && std::abs(fy - 1.0f) > std::abs(fx - 1.0f))) {
                factor = fy;
            }
            scale = drag_start_scale_ * factor;
        }
        scale.x = keep_away_from_zero(scale.x, drag_start_scale_.x);
        scale.y = keep_away_from_zero(scale.y, drag_start_scale_.y);

        node->set_scale(scale);
        node->set_position(anchor_parent - axes.basis_xform(scale * anchor));
        update_overlays();
        return true;
    }
    return false;
}

} // namespace godot

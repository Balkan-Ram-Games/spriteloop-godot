#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/transform2d.hpp>

namespace godot {

class SpriteLoop2D;

// 2D editor support for SpriteLoop2D, the counterpart of Godot's
// Sprite2DEditorPlugin. Godot gives built-in 2D nodes picking and rect handles
// through CanvasItem::_edit_get_rect(), which GDExtension cannot implement, so
// this plugin provides both:
//
// - Picking: every SpriteLoop2D in the edited scene gets an invisible,
//   internal Control over its bounds, owned by the node. The 2D editor picks
//   internal children and resolves the hit through owners to the node, so a
//   click on the artwork selects it and native drag-move works. The Control is
//   editor-only and is not saved with the scene.
// - Handles: the selected node's bounds are drawn with Sprite2D-style handles;
//   dragging one scales the node with the opposite side fixed (Shift keeps the
//   aspect ratio), with undo/redo.
class SpriteLoop2DEditorPlugin : public EditorPlugin {
    GDCLASS(SpriteLoop2DEditorPlugin, EditorPlugin)

public:
    void _enter_tree() override;
    void _exit_tree() override;
    bool _handles(Object* object) const override;
    void _edit(Object* object) override;
    void _forward_canvas_draw_over_viewport(Control* overlay) override;
    bool _forward_canvas_gui_input(const Ref<InputEvent>& event) override;

protected:
    static void _bind_methods() {}

private:
    // Picking.
    void on_node_added(Node* node);
    void attach_pick_rect(uint64_t node_id);
    void update_pick_rect(uint64_t node_id);
    static Control* find_pick_rect(SpriteLoop2D* node);
    void remove_all_pick_rects();

    // Handles.
    SpriteLoop2D* edited_node() const;
    static void handle_points(const Rect2& rect, Vector2 (&points)[8]);
    static Transform2D local_to_screen(SpriteLoop2D* node);
    static Transform2D parent_to_screen(SpriteLoop2D* node);

    uint64_t edited_ = 0;
    int drag_handle_ = -1;
    Vector2 drag_start_scale_;
    Vector2 drag_start_position_;
};

} // namespace godot

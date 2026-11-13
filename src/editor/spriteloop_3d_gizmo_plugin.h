#pragma once

#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace godot {

// Makes SpriteLoop3D pickable in the 3D editor: the editor selects nodes by
// ray-testing gizmo collision triangles. Covers the node's bounds quad.
class SpriteLoop3DGizmoPlugin : public EditorNode3DGizmoPlugin {
    GDCLASS(SpriteLoop3DGizmoPlugin, EditorNode3DGizmoPlugin)

public:
    bool _has_gizmo(Node3D* node) const override;
    String _get_gizmo_name() const override;
    int32_t _get_priority() const override;
    void _redraw(const Ref<EditorNode3DGizmo>& gizmo) override;

protected:
    static void _bind_methods() {}
};

} // namespace godot

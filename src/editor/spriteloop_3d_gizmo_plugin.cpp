#include "editor/spriteloop_3d_gizmo_plugin.h"

#include "nodes/spriteloop_3d.h"

#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/triangle_mesh.hpp>

namespace godot {

bool SpriteLoop3DGizmoPlugin::_has_gizmo(Node3D* node) const
{
    return Object::cast_to<SpriteLoop3D>(node) != nullptr;
}

String SpriteLoop3DGizmoPlugin::_get_gizmo_name() const
{
    return "SpriteLoop3D";
}

int32_t SpriteLoop3DGizmoPlugin::_get_priority() const
{
    return -1;
}

void SpriteLoop3DGizmoPlugin::_redraw(const Ref<EditorNode3DGizmo>& gizmo)
{
    gizmo->clear();
    SpriteLoop3D* node = Object::cast_to<SpriteLoop3D>(gizmo->get_node_3d());
    if (node == nullptr) {
        return;
    }
    const Rect2 rect = node->get_local_rect();
    if (rect.size.x <= 0.0f || rect.size.y <= 0.0f) {
        return;
    }
    // The quad in the node's XY plane. Billboarded nodes are picked by this
    // quad too, like Sprite3D.
    const Vector3 a(rect.position.x, rect.position.y, 0.0f);
    const Vector3 b(rect.get_end().x, rect.position.y, 0.0f);
    const Vector3 c(rect.get_end().x, rect.get_end().y, 0.0f);
    const Vector3 d(rect.position.x, rect.get_end().y, 0.0f);
    PackedVector3Array faces;
    for (const Vector3& point : {a, b, c, a, c, d}) {
        faces.push_back(point);
    }
    Ref<TriangleMesh> triangles;
    triangles.instantiate();
    if (triangles->create_from_faces(faces)) {
        gizmo->add_collision_triangles(triangles);
    }
}

} // namespace godot

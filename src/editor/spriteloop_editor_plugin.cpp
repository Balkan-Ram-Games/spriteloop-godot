#include "editor/spriteloop_editor_plugin.h"

#include "core/material_3d.h"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {

void SpriteLoopEditorPlugin::_enter_tree()
{
    import_plugin_.instantiate();
    add_import_plugin(Ref<EditorImportPlugin>(import_plugin_));
    gizmo_plugin_.instantiate();
    add_node_3d_gizmo_plugin(Ref<EditorNode3DGizmoPlugin>(gizmo_plugin_));
    // Inspector edits of a SpriteLoop3D.material go through undo/redo but do
    // not emit `changed`; refresh the node copies after every step.
    get_undo_redo()->connect("version_changed",
                             callable_mp_static(&spriteloop_godot::refresh_material_3d_templates));
}

void SpriteLoopEditorPlugin::_exit_tree()
{
    const Callable refresh = callable_mp_static(&spriteloop_godot::refresh_material_3d_templates);
    if (get_undo_redo()->is_connected("version_changed", refresh)) {
        get_undo_redo()->disconnect("version_changed", refresh);
    }
    if (import_plugin_.is_valid()) {
        remove_import_plugin(Ref<EditorImportPlugin>(import_plugin_));
        import_plugin_.unref();
    }
    if (gizmo_plugin_.is_valid()) {
        remove_node_3d_gizmo_plugin(Ref<EditorNode3DGizmoPlugin>(gizmo_plugin_));
        gizmo_plugin_.unref();
    }
}

} // namespace godot

#include "editor/spriteloop_editor_plugin.h"

namespace godot {

void SpriteLoopEditorPlugin::_enter_tree()
{
    import_plugin_.instantiate();
    add_import_plugin(Ref<EditorImportPlugin>(import_plugin_));
    gizmo_plugin_.instantiate();
    add_node_3d_gizmo_plugin(Ref<EditorNode3DGizmoPlugin>(gizmo_plugin_));
}

void SpriteLoopEditorPlugin::_exit_tree()
{
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

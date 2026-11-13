#pragma once

#include "editor/spla_import_plugin.h"
#include "editor/spriteloop_3d_gizmo_plugin.h"

#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot {

// Registers the .spla importer and the SpriteLoop3D gizmo. 2D editing lives in
// SpriteLoop2DEditorPlugin.
class SpriteLoopEditorPlugin : public EditorPlugin {
    GDCLASS(SpriteLoopEditorPlugin, EditorPlugin)

public:
    void _enter_tree() override;
    void _exit_tree() override;

protected:
    static void _bind_methods() {}

private:
    Ref<SplaImportPlugin> import_plugin_;
    Ref<SpriteLoop3DGizmoPlugin> gizmo_plugin_;
};

} // namespace godot

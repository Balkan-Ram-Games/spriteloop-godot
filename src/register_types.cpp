#include "core/material_3d.h"
#include "nodes/spriteloop_2d.h"
#include "nodes/spriteloop_3d.h"
#include "spriteloop_data.h"
#include "spriteloop_server.h"

#ifdef SPRITELOOP_EDITOR
#include "editor/spla_import_plugin.h"
#include "editor/spriteloop_2d_editor_plugin.h"
#include "editor/spriteloop_3d_gizmo_plugin.h"
#include "editor/spriteloop_editor_plugin.h"

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#endif

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

namespace {

SpriteLoopServer* server = nullptr;

void initialize_spriteloop(ModuleInitializationLevel level)
{
    if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        GDREGISTER_CLASS(SpriteLoopServer);
        GDREGISTER_CLASS(SpriteLoopData);
        GDREGISTER_CLASS(SpriteLoop2D);
        GDREGISTER_CLASS(SpriteLoop3D);
        server = memnew(SpriteLoopServer);
        Engine::get_singleton()->register_singleton("SpriteLoopServer", server);
    }
#ifdef SPRITELOOP_EDITOR
    if (level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        GDREGISTER_INTERNAL_CLASS(SplaImportPlugin);
        GDREGISTER_INTERNAL_CLASS(SpriteLoop3DGizmoPlugin);
        GDREGISTER_INTERNAL_CLASS(SpriteLoopEditorPlugin);
        GDREGISTER_INTERNAL_CLASS(SpriteLoop2DEditorPlugin);
        EditorPlugins::add_by_type<SpriteLoopEditorPlugin>();
        EditorPlugins::add_by_type<SpriteLoop2DEditorPlugin>();
    }
#endif
}

void uninitialize_spriteloop(ModuleInitializationLevel level)
{
#ifdef SPRITELOOP_EDITOR
    if (level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        EditorPlugins::remove_by_type<SpriteLoop2DEditorPlugin>();
        EditorPlugins::remove_by_type<SpriteLoopEditorPlugin>();
    }
#endif
    if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        spriteloop_godot::clear_material_3d_cache();
    }
    if (level == MODULE_INITIALIZATION_LEVEL_SCENE && server != nullptr) {
        Engine::get_singleton()->unregister_singleton("SpriteLoopServer");
        memdelete(server);
        server = nullptr;
    }
}

} // namespace

extern "C" {

GDExtensionBool GDE_EXPORT spriteloop_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                                                   GDExtensionClassLibraryPtr library,
                                                   GDExtensionInitialization* initialization)
{
    GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(initialize_spriteloop);
    init.register_terminator(uninitialize_spriteloop);
    init.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}

} // extern "C"

#include "core/mask_cache.h"
#include "core/mask_texture.h"
#include "core/material_3d.h"
#include "nodes/spriteloop_2d.h"
#include "nodes/spriteloop_3d.h"
#include "nodes/spriteloop_attachment.h"
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
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

namespace {

SpriteLoopServer* server = nullptr;

constexpr const char* mask_cache_budget_setting = "spriteloop/masks/cache_budget_mb";

void register_project_settings()
{
    ProjectSettings* settings = ProjectSettings::get_singleton();
    if (!settings->has_setting(mask_cache_budget_setting)) {
        settings->set_setting(mask_cache_budget_setting, 32);
    }
    settings->set_initial_value(mask_cache_budget_setting, 32);
    settings->set_as_basic(mask_cache_budget_setting, true);
    Dictionary info;
    info["name"] = mask_cache_budget_setting;
    info["type"] = Variant::INT;
    info["hint"] = PROPERTY_HINT_RANGE;
    info["hint_string"] = "0,128,1,suffix:MiB";
    settings->add_property_info(info);
    const int budget = settings->get_setting(mask_cache_budget_setting);
    if (budget < 0 || budget > 128) {
        WARN_PRINT("spriteloop/masks/cache_budget_mb must be between 0 and 128 MiB; using 32.");
        server->set_mask_cache_budget_mb(32);
    } else {
        server->set_mask_cache_budget_mb(budget);
    }
}

void initialize_spriteloop(ModuleInitializationLevel level)
{
    if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        GDREGISTER_CLASS(SpriteLoopServer);
        GDREGISTER_CLASS(SpriteLoopData);
        GDREGISTER_CLASS(SpriteLoop2D);
        GDREGISTER_CLASS(SpriteLoop3D);
        GDREGISTER_CLASS(SpriteLoopAttachment2D);
        GDREGISTER_CLASS(SpriteLoopAttachment3D);
        GDREGISTER_INTERNAL_CLASS(SpriteLoopMaskTexture);
        server = memnew(SpriteLoopServer);
        Engine::get_singleton()->register_singleton("SpriteLoopServer", server);
        register_project_settings();
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
        spriteloop_godot::mask_cache::clear();
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

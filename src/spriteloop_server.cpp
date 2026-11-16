#include "spriteloop_server.h"

#include "core/mask_cache.h"
#include "core/material_3d.h"
#include "core/stats.h"
#include "version.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

SpriteLoopServer* SpriteLoopServer::singleton_ = nullptr;

SpriteLoopServer::SpriteLoopServer()
{
    singleton_ = this;
}

SpriteLoopServer::~SpriteLoopServer()
{
    if (singleton_ == this) {
        singleton_ = nullptr;
    }
}

void SpriteLoopServer::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_stats"), &SpriteLoopServer::get_stats);
    ClassDB::bind_method(D_METHOD("get_version"), &SpriteLoopServer::get_version);
    ClassDB::bind_method(D_METHOD("set_mask_cache_budget_mb", "megabytes"),
                         &SpriteLoopServer::set_mask_cache_budget_mb);
    ClassDB::bind_method(D_METHOD("get_mask_cache_budget_mb"), &SpriteLoopServer::get_mask_cache_budget_mb);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mask_cache_budget_mb", PROPERTY_HINT_RANGE, "0,128,1,suffix:MiB"),
                 "set_mask_cache_budget_mb", "get_mask_cache_budget_mb");
}

Dictionary SpriteLoopServer::get_stats() const
{
    const spriteloop_godot::Stats& stats = spriteloop_godot::stats();
    Dictionary result;
    result["packages"] = static_cast<int64_t>(stats.packages);
    result["atlas_bytes"] = static_cast<int64_t>(stats.atlas_bytes);
    result["baked_sets"] = static_cast<int64_t>(stats.baked_sets);
    result["bakes"] = static_cast<int64_t>(stats.bakes);
    result["meshes"] = static_cast<int64_t>(stats.meshes);
    result["mesh_bytes"] = static_cast<int64_t>(stats.mesh_vertex_bytes);
    result["nodes"] = static_cast<int64_t>(stats.nodes);
    result["redraws"] = static_cast<int64_t>(stats.redraws);
    result["mask_frames"] = static_cast<int64_t>(stats.mask_frames);
    result["mask_frame_bytes"] = static_cast<int64_t>(stats.mask_frame_bytes);
    result["mask_cache_bytes"] = static_cast<int64_t>(spriteloop_godot::mask_cache::retained_bytes());
    result["mask_compositions"] = static_cast<int64_t>(stats.mask_compositions);
    result["mask_cache_hits"] = static_cast<int64_t>(stats.mask_cache_hits);
    result["materials_3d"] = static_cast<int64_t>(spriteloop_godot::live_material_3d_count());
    result["sdk_version"] = SPRITELOOP_SDK_VERSION;
    result["extension_version"] = SPRITELOOP_GODOT_VERSION;
    return result;
}

String SpriteLoopServer::get_version() const
{
    return SPRITELOOP_GODOT_VERSION;
}

void SpriteLoopServer::set_mask_cache_budget_mb(int megabytes)
{
    if (megabytes < 0 || megabytes > 128) {
        ERR_PRINT("SpriteLoop: mask_cache_budget_mb must be between 0 and 128.");
        return;
    }
    spriteloop_godot::mask_cache::set_budget(static_cast<std::size_t>(megabytes) * 1024 * 1024);
}

int SpriteLoopServer::get_mask_cache_budget_mb() const
{
    return static_cast<int>(spriteloop_godot::mask_cache::budget() / (1024 * 1024));
}

} // namespace godot

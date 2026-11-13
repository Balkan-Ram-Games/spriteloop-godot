#include "spriteloop_server.h"

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
    result["sdk_version"] = SPRITELOOP_SDK_VERSION;
    result["extension_version"] = SPRITELOOP_GODOT_VERSION;
    return result;
}

String SpriteLoopServer::get_version() const
{
    return SPRITELOOP_GODOT_VERSION;
}

} // namespace godot

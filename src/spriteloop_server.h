#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

// Engine singleton with process-wide SpriteLoop diagnostics.
class SpriteLoopServer : public Object {
    GDCLASS(SpriteLoopServer, Object)

public:
    static SpriteLoopServer* get_singleton() { return singleton_; }

    SpriteLoopServer();
    ~SpriteLoopServer() override;

    // packages, atlas_bytes, baked_sets, bakes, meshes, mesh_bytes, nodes,
    // redraws, sdk_version, extension_version.
    Dictionary get_stats() const;
    String get_version() const;

protected:
    static void _bind_methods();

private:
    static SpriteLoopServer* singleton_;
};

} // namespace godot

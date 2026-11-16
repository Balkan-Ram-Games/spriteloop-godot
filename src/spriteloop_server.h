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
    // redraws, mask_frames, mask_frame_bytes, mask_cache_bytes,
    // mask_compositions, mask_cache_hits, materials_3d, sdk_version,
    // extension_version.
    Dictionary get_stats() const;
    String get_version() const;

    // Retained composed mask frames, in MiB (0-128). Starts from the project
    // setting spriteloop/masks/cache_budget_mb (default 32). 0 keeps only the
    // frames nodes show.
    void set_mask_cache_budget_mb(int megabytes);
    int get_mask_cache_budget_mb() const;

protected:
    static void _bind_methods();

private:
    static SpriteLoopServer* singleton_;
};

} // namespace godot

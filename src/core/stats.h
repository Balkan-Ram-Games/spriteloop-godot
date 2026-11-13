#pragma once

#include <cstddef>
#include <cstdint>

namespace spriteloop_godot {

// Process-wide counters exposed through SpriteLoopServer.get_stats() for
// tests (leak checks) and benchmarks. Main-thread only.
struct Stats {
    std::size_t packages = 0;          // Live parsed packages with an atlas texture.
    std::size_t atlas_bytes = 0;       // RGBA8 payload of live atlas textures.
    std::size_t baked_sets = 0;        // Live baked sets (one per package + skin state).
    std::uint64_t bakes = 0;           // Baked sets created since start.
    std::size_t meshes = 0;            // Live frame meshes (2D and 3D).
    std::size_t mesh_vertex_bytes = 0; // Approximate payload of live frame meshes.
    std::size_t nodes = 0;             // Live SpriteLoop nodes.
    std::uint64_t redraws = 0;         // Frame re-records since start.
};

inline Stats& stats()
{
    static Stats value;
    return value;
}

} // namespace spriteloop_godot

#pragma once

#include "spriteloop/spla_atlas.hpp"
#include "spriteloop/spla_baked.hpp"
#include "spriteloop/spla_package.hpp"

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace spriteloop_godot {

// Baked geometry for one skin state (active skin + manual variant overrides).
//
// Shared by every node that shows the same package with the same skin state, so
// a crowd of identical characters bakes once. Frame meshes are immutable and
// created lazily on first use; nodes only switch which mesh they reference.
class BakedSet {
public:
    BakedSet() = default;
    BakedSet(const BakedSet&) = delete;
    BakedSet& operator=(const BakedSet&) = delete;
    ~BakedSet();

    // Same order as SplaPackage::animations.
    std::vector<spriteloop::SplaBakedAnimation> animations;
    spriteloop::SplaBounds bounds;

    // Bounds in Godot 2D coordinates (pixels, Y down, centered on the node).
    godot::Rect2 bounds_2d() const;

    // Mesh RIDs for one frame. An invalid RID means the frame has no geometry.
    // 2D meshes are in pixels, Y down. 3D meshes are in world units (pixels *
    // pixel_size), Y up, facing +Z, cached per pixel size.
    godot::RID mesh_2d(int animation_index, int frame_index);
    godot::RID mesh_3d(int animation_index, int frame_index, float pixel_size);

    const spriteloop::SplaBakedFrame* frame(int animation_index, int frame_index) const;

    std::size_t mesh_count() const { return mesh_count_; }
    std::size_t mesh_vertex_bytes() const { return mesh_vertex_bytes_; }

private:
    struct FrameMesh {
        godot::RID rid;
        bool built = false;
    };

    using MeshCache = std::vector<std::vector<FrameMesh>>;

    godot::RID build_mesh(const spriteloop::SplaBakedFrame& frame, bool is_2d, float scale);
    godot::RID cached_mesh(MeshCache& cache, int animation_index, int frame_index, bool is_2d,
                           float scale);

    MeshCache meshes_2d_;
    std::map<float, MeshCache> meshes_3d_;
    std::size_t mesh_count_ = 0;
    std::size_t mesh_vertex_bytes_ = 0;
};

// One parsed .spla package with its atlas texture. Refcounted through
// shared_ptr and owned by SpriteLoopData; nodes keep it alive while they use it.
class PackageAsset {
public:
    // mipmaps: generate atlas mip levels (with wider padding between images).
    static std::shared_ptr<PackageAsset> create(const std::uint8_t* bytes,
                                                std::size_t byte_count,
                                                const std::string& debug_name,
                                                bool mipmaps,
                                                std::string& error);

    PackageAsset() = default;
    PackageAsset(const PackageAsset&) = delete;
    PackageAsset& operator=(const PackageAsset&) = delete;
    ~PackageAsset();

    const spriteloop::SplaPackage& package() const { return package_; }
    godot::RID texture_rid() const;
    const godot::Ref<godot::ImageTexture>& texture() const { return texture_; }
    int atlas_width() const { return atlas_.width; }
    int atlas_height() const { return atlas_.height; }
    std::size_t atlas_texture_bytes() const { return atlas_texture_bytes_; }
    bool has_mipmaps() const { return mipmaps_; }

    // Returns the shared baked set for a skin state, baking it on first use.
    std::shared_ptr<BakedSet> baked_set(const spriteloop::SplaSkinState& skin_state);

    // Normalizes equivalent skin states (an empty skin equals no skin, trailing
    // cleared overrides are dropped) so they share one BakedSet.
    std::string skin_state_key(const spriteloop::SplaSkinState& skin_state) const;

    std::size_t live_baked_set_count() const;

private:
    spriteloop::SplaPackage package_;
    spriteloop::SplaAtlas atlas_; // Regions only; pixels are released after upload.
    spriteloop::SplaPartImageMap image_map_;
    godot::Ref<godot::ImageTexture> texture_;
    std::size_t atlas_texture_bytes_ = 0;
    bool mipmaps_ = false;
    std::map<std::string, std::weak_ptr<BakedSet>> baked_sets_;
};

} // namespace spriteloop_godot

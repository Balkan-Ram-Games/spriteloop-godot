#pragma once

#include "spriteloop/spla_baked.hpp"

#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace spriteloop_godot {

class BakedSet;
class PackageAsset;

// One baked frame with masks, composed once into an offscreen texture.
//
// RenderingServer viewports and canvas items, like Defold's offscreen
// composite: ordinary ranges draw into a composition; each mask group draws
// its artwork and its coverage (mask source with the eroded alpha atlas) into
// two scratch textures, and the composition adds group * coverage.a
// (premultiplied, once: the SDK rule). A last pass converts the composition to
// straight alpha, so the texture draws like any other Godot texture.
//
// Godot's clip children are not used: their composite writes the coverage as
// alpha and drops the group's own alpha, which is wrong for a texture that is
// blended again.
//
// Nodes keep the frame they show alive; the cache only decides which frames
// stay around for reuse.
class MaskFrame : public std::enable_shared_from_this<MaskFrame> {
public:
    MaskFrame() = default;
    MaskFrame(const MaskFrame&) = delete;
    MaskFrame& operator=(const MaskFrame&) = delete;
    ~MaskFrame();

    godot::RID texture() const { return texture_; }
    // Where the texture goes in 2D node space (pixels, Y down, centered).
    const godot::Rect2& rect() const { return rect_; }
    std::size_t bytes() const { return bytes_; }

    // The texture as a Texture2D resource (one per frame while in use), for
    // SpriteLoop3D materials. It keeps this frame alive.
    godot::Ref<godot::Texture2D> texture_resource();
    // A quad covering rect() for SpriteLoop3D, in world units (pixels *
    // pixel_size, Y up, facing +Z), cached per pixel size.
    godot::RID quad_mesh_3d(float pixel_size);

    // One offscreen 2D pass (viewport, canvas, items, materials).
    struct Pass {
        godot::RID viewport;
        godot::RID canvas;
        std::vector<godot::RID> items;
        std::vector<godot::RID> materials;
    };

    bool has_scratch() const { return !scratch_.empty(); }
    std::uint64_t scratch_frame() const { return scratch_frame_; }
    // Frees the group and coverage passes (after the result has rendered).
    void release_scratch();

private:
    static void free_pass(Pass& pass);

    friend std::shared_ptr<MaskFrame> compose_mask_frame(const std::shared_ptr<PackageAsset>&,
                                                         const spriteloop::SplaBakedFrame&, const godot::Rect2&,
                                                         bool);

    // Keeps the artwork and mask atlases alive while the canvas items use them.
    std::shared_ptr<PackageAsset> asset_;
    Pass result_;
    std::vector<Pass> scratch_;
    std::uint64_t scratch_frame_ = 0;
    godot::RID texture_;
    godot::Rect2 rect_;
    std::size_t bytes_ = 0;
    bool counted_ = false; // Included in the mask frame stats.
    godot::ObjectID texture_resource_;
    std::map<float, godot::RID> quads_3d_;
};

// True when any draw of the frame is masked.
bool frame_has_masks(const spriteloop::SplaBakedFrame& frame);

// Composes a frame. `bounds` (node space) is the area of the texture, usually
// the bounds of every frame of the baked set so all frames share a size.
// Returns null (with an error) if the frame cannot be composed.
std::shared_ptr<MaskFrame> compose_mask_frame(const std::shared_ptr<PackageAsset>& asset,
                                              const spriteloop::SplaBakedFrame& frame, const godot::Rect2& bounds,
                                              bool nearest);

// Shared cache of composed frames, keyed by baked set, animation, frame and
// filtering. Frames that no node shows are evicted, least recently used
// first, once the retained textures exceed the budget.
namespace mask_cache {

std::shared_ptr<MaskFrame> get(const BakedSet& set, const std::shared_ptr<PackageAsset>& asset, int animation_index,
                               int frame_index, bool nearest);
// Drops the entries of a baked set that is being destroyed.
void forget(const BakedSet& set);
void set_budget(std::size_t bytes);
std::size_t budget();
std::size_t retained_bytes();
// Releases every entry and the material (extension shutdown).
void clear();

} // namespace mask_cache

} // namespace spriteloop_godot

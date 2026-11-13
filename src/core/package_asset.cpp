#include "core/package_asset.h"

#include "core/stats.h"

#include "spriteloop/spla_loader.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace godot;

namespace spriteloop_godot {

namespace {

// Godot's largest supported 2D texture dimension.
constexpr int max_texture_dimension = 16384;

// Depth separation between consecutive draws of a 3D frame, in package
// pixels (0.001 world units at the default pixel_size of 0.01).
constexpr float depth_step_pixels = 0.1f;

const spriteloop::SplaAsset* find_asset(const spriteloop::SplaPackage& package,
                                        const std::string& path)
{
    for (const spriteloop::SplaAsset& asset : package.assets) {
        if (asset.path == path) {
            return &asset;
        }
    }
    return nullptr;
}

// Mesh UVs may extend outside an image (contain-fit and deformed meshes). Pad
// atlas regions enough that such samples never reach a neighbouring image.
// Same rule as the Defold adapter.
int required_atlas_padding(const spriteloop::SplaPackage& package)
{
    float maximum = 0.0f;
    for (std::size_t part_index = 0; part_index < package.parts.size(); ++part_index) {
        const spriteloop::SplaPart& part = package.parts[part_index];
        if (part.mesh.vertices.empty()) {
            continue;
        }
        float image_width = static_cast<float>(std::max(1, part.width));
        float image_height = static_cast<float>(std::max(1, part.height));
        for (const spriteloop::SplaVariant& variant : package.variants) {
            if (variant.part_index == static_cast<int>(part_index)) {
                image_width = std::max(image_width, static_cast<float>(variant.width));
                image_height = std::max(image_height, static_cast<float>(variant.height));
            }
        }
        for (const spriteloop::SplaMeshVertex& vertex : part.mesh.vertices) {
            maximum = std::max(maximum, std::max(std::max(0.0f, -vertex.u) * image_width,
                                                 std::max(0.0f, vertex.u - 1.0f) * image_width));
            maximum = std::max(maximum, std::max(std::max(0.0f, -vertex.v) * image_height,
                                                 std::max(0.0f, vertex.v - 1.0f) * image_height));
        }
    }
    return std::max(1, static_cast<int>(std::ceil(maximum)) + 1);
}

PackedByteArray to_packed(const std::uint8_t* data, std::size_t size)
{
    PackedByteArray bytes;
    bytes.resize(static_cast<int64_t>(size));
    if (size > 0) {
        std::memcpy(bytes.ptrw(), data, size);
    }
    return bytes;
}

} // namespace

// ---------------------------------------------------------------------------
// BakedSet

BakedSet::~BakedSet()
{
    RenderingServer* rs = RenderingServer::get_singleton();
    if (rs == nullptr) {
        return;
    }
    const auto free_cache = [rs](MeshCache& cache) {
        for (auto& animation : cache) {
            for (FrameMesh& mesh : animation) {
                if (mesh.rid.is_valid()) {
                    rs->free_rid(mesh.rid);
                }
            }
        }
    };
    free_cache(meshes_2d_);
    for (auto& entry : meshes_3d_) {
        free_cache(entry.second);
    }
    stats().meshes -= mesh_count_;
    stats().mesh_vertex_bytes -= mesh_vertex_bytes_;
    --stats().baked_sets;
}

Rect2 BakedSet::bounds_2d() const
{
    // SDK bounds are Y up; Godot 2D is Y down.
    return Rect2(bounds.min_x, -bounds.max_y, bounds.max_x - bounds.min_x,
                 bounds.max_y - bounds.min_y);
}

const spriteloop::SplaBakedFrame* BakedSet::frame(int animation_index, int frame_index) const
{
    if (animation_index < 0 || animation_index >= static_cast<int>(animations.size())) {
        return nullptr;
    }
    const auto& frames = animations[static_cast<std::size_t>(animation_index)].frames;
    if (frame_index < 0 || frame_index >= static_cast<int>(frames.size())) {
        return nullptr;
    }
    return &frames[static_cast<std::size_t>(frame_index)];
}

RID BakedSet::mesh_2d(int animation_index, int frame_index)
{
    return cached_mesh(meshes_2d_, animation_index, frame_index, true, 1.0f);
}

RID BakedSet::mesh_3d(int animation_index, int frame_index, float pixel_size)
{
    return cached_mesh(meshes_3d_[pixel_size], animation_index, frame_index, false, pixel_size);
}

RID BakedSet::cached_mesh(MeshCache& cache, int animation_index, int frame_index, bool is_2d,
                          float scale)
{
    const spriteloop::SplaBakedFrame* baked = frame(animation_index, frame_index);
    if (baked == nullptr) {
        return RID();
    }
    if (cache.empty()) {
        cache.resize(animations.size());
    }
    auto& frames = cache[static_cast<std::size_t>(animation_index)];
    if (frames.empty()) {
        frames.resize(animations[static_cast<std::size_t>(animation_index)].frames.size());
    }
    FrameMesh& mesh = frames[static_cast<std::size_t>(frame_index)];
    if (!mesh.built) {
        mesh.rid = build_mesh(*baked, is_2d, scale);
        mesh.built = true;
    }
    return mesh.rid;
}

RID BakedSet::build_mesh(const spriteloop::SplaBakedFrame& frame, bool is_2d, float scale)
{
    if (frame.indices.empty() || frame.vertices.empty()) {
        return RID();
    }

    const int64_t vertex_count = static_cast<int64_t>(frame.vertices.size());
    PackedVector2Array uvs;
    PackedColorArray colors;
    PackedInt32Array indices;
    uvs.resize(vertex_count);
    colors.resize(vertex_count);
    indices.resize(static_cast<int64_t>(frame.indices.size()));

    Vector2* uv_out = uvs.ptrw();
    Color* color_out = colors.ptrw();
    for (int64_t i = 0; i < vertex_count; ++i) {
        const spriteloop::SplaBakedVertex& vertex = frame.vertices[static_cast<std::size_t>(i)];
        uv_out[i] = Vector2(vertex.u, vertex.v);
        color_out[i] = Color(vertex.r, vertex.g, vertex.b, vertex.a);
    }
    int32_t* index_out = indices.ptrw();
    for (std::size_t i = 0; i < frame.indices.size(); ++i) {
        index_out[i] = static_cast<int32_t>(frame.indices[i]);
    }

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    std::size_t position_bytes = 0;
    if (is_2d) {
        PackedVector2Array positions;
        positions.resize(vertex_count);
        Vector2* out = positions.ptrw();
        for (int64_t i = 0; i < vertex_count; ++i) {
            const auto& vertex = frame.vertices[static_cast<std::size_t>(i)];
            // SDK geometry is centered and Y up; Godot 2D is Y down.
            out[i] = Vector2(vertex.x, -vertex.y);
        }
        arrays[Mesh::ARRAY_VERTEX] = positions;
        position_bytes = sizeof(float) * 2;
    } else {
        // Draw-order layer per vertex, stored in UV2: x = layer index, y = layer
        // depth in world units. The SpriteLoop3D shader moves later layers
        // towards the viewer from either side, so with depth writes (alpha cut
        // modes, shadow maps) coplanar parts keep painter's order instead of
        // z-fighting. The geometry itself stays flat.
        std::vector<float> layers(static_cast<std::size_t>(vertex_count), 0.0f);
        const auto assign_layer = [&](std::size_t start, std::size_t count, float layer) {
            for (std::size_t i = start; i < start + count && i < frame.indices.size(); ++i) {
                const std::uint32_t index = frame.indices[i];
                if (index < layers.size()) {
                    layers[index] = layer;
                }
            }
        };
        if (!frame.draws.empty()) {
            for (std::size_t draw = 0; draw < frame.draws.size(); ++draw) {
                assign_layer(frame.draws[draw].index_start, frame.draws[draw].index_count,
                             static_cast<float>(draw));
            }
        } else {
            for (std::size_t triangle = 0; triangle * 3 < frame.indices.size(); ++triangle) {
                assign_layer(triangle * 3, 3, static_cast<float>(triangle));
            }
        }

        PackedVector3Array positions;
        PackedVector3Array normals;
        PackedVector2Array layer_data;
        positions.resize(vertex_count);
        normals.resize(vertex_count);
        layer_data.resize(vertex_count);
        Vector3* out = positions.ptrw();
        Vector3* normal_out = normals.ptrw();
        Vector2* layer_out = layer_data.ptrw();
        for (int64_t i = 0; i < vertex_count; ++i) {
            const auto& vertex = frame.vertices[static_cast<std::size_t>(i)];
            const float layer = layers[static_cast<std::size_t>(i)];
            // World units, Y up, facing +Z like Sprite3D.
            out[i] = Vector3(vertex.x * scale, vertex.y * scale, 0.0f);
            normal_out[i] = Vector3(0.0f, 0.0f, 1.0f);
            layer_out[i] = Vector2(layer, layer * depth_step_pixels * scale);
        }
        arrays[Mesh::ARRAY_VERTEX] = positions;
        arrays[Mesh::ARRAY_NORMAL] = normals;
        arrays[Mesh::ARRAY_TEX_UV2] = layer_data;
        position_bytes = sizeof(float) * 8;
    }
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_COLOR] = colors;
    arrays[Mesh::ARRAY_INDEX] = indices;

    RenderingServer* rs = RenderingServer::get_singleton();
    RID mesh = rs->mesh_create();
    const uint64_t flags = is_2d ? RenderingServer::ARRAY_FLAG_USE_2D_VERTICES : 0;
    rs->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_TRIANGLES, arrays, Array(),
                                     Dictionary(), flags);

    const std::size_t bytes =
        static_cast<std::size_t>(vertex_count) * (position_bytes + sizeof(float) * 2 + sizeof(float) * 4) +
        frame.indices.size() * sizeof(int32_t);
    ++mesh_count_;
    mesh_vertex_bytes_ += bytes;
    ++stats().meshes;
    stats().mesh_vertex_bytes += bytes;
    return mesh;
}

// ---------------------------------------------------------------------------
// PackageAsset

std::shared_ptr<PackageAsset> PackageAsset::create(const std::uint8_t* bytes,
                                                   std::size_t byte_count,
                                                   const std::string& debug_name,
                                                   std::string& error)
{
    auto package_result = spriteloop::load_package_from_memory(bytes, byte_count, debug_name);
    if (!package_result) {
        error = package_result.error().message;
        return nullptr;
    }

    auto asset = std::make_shared<PackageAsset>();
    asset->package_ = std::move(package_result).value();
    asset->image_map_ = spriteloop::build_part_image_map_by_asset(asset->package_);

    // Decode every referenced image. Region i of the packed atlas belongs to
    // source asset i, which is what the SDK image map expects.
    const std::vector<std::string>& asset_paths = asset->image_map_.source_asset_paths;
    std::vector<spriteloop::SplaRgbaImage> images;
    images.reserve(asset_paths.size());
    for (const std::string& path : asset_paths) {
        const spriteloop::SplaAsset* source = find_asset(asset->package_, path);
        if (source == nullptr) {
            error = "missing embedded asset: " + path;
            return nullptr;
        }
        Ref<Image> image;
        image.instantiate();
        if (image->load_png_from_buffer(to_packed(source->bytes.data(), source->bytes.size())) != OK ||
            image->is_empty()) {
            error = "failed to decode PNG asset '" + path + "'";
            return nullptr;
        }
        if (image->get_format() != Image::FORMAT_RGBA8) {
            image->convert(Image::FORMAT_RGBA8);
        }
        const PackedByteArray pixels = image->get_data();
        spriteloop::SplaRgbaImage rgba;
        rgba.id = path;
        rgba.width = image->get_width();
        rgba.height = image->get_height();
        rgba.pixels.assign(pixels.ptr(), pixels.ptr() + pixels.size());
        images.push_back(std::move(rgba));
    }
    if (images.empty()) {
        spriteloop::SplaRgbaImage empty;
        empty.id = "__spriteloop_empty";
        empty.width = 1;
        empty.height = 1;
        empty.pixels = {0, 0, 0, 0};
        images.push_back(std::move(empty));
    }

    spriteloop::SplaAtlasOptions options;
    options.padding = required_atlas_padding(asset->package_);
    auto atlas_result = spriteloop::pack_rgba_atlas(images, options);
    if (!atlas_result) {
        error = "failed to pack SpriteLoop atlas: " + atlas_result.error().message;
        return nullptr;
    }
    asset->atlas_ = std::move(atlas_result).value();
    if (asset->atlas_.width > max_texture_dimension || asset->atlas_.height > max_texture_dimension) {
        error = "SpriteLoop atlas is too large (" + std::to_string(asset->atlas_.width) + "x" +
                std::to_string(asset->atlas_.height) + ", limit " +
                std::to_string(max_texture_dimension) + ")";
        return nullptr;
    }

    Ref<Image> atlas_image = Image::create_from_data(
        asset->atlas_.width, asset->atlas_.height, false, Image::FORMAT_RGBA8,
        to_packed(asset->atlas_.pixels.data(), asset->atlas_.pixels.size()));
    if (atlas_image.is_null()) {
        error = "failed to create the SpriteLoop atlas image";
        return nullptr;
    }
    // Godot blends straight alpha. Bleed edge colors into transparent texels so
    // linear filtering does not darken outlines (Godot's own importer default).
    atlas_image->fix_alpha_edges();
    asset->texture_ = ImageTexture::create_from_image(atlas_image);
    if (asset->texture_.is_null()) {
        error = "failed to create the SpriteLoop atlas texture";
        return nullptr;
    }
    asset->atlas_texture_bytes_ = asset->atlas_.pixels.size();
    std::vector<std::uint8_t>().swap(asset->atlas_.pixels);

    ++stats().packages;
    stats().atlas_bytes += asset->atlas_texture_bytes_;
    return asset;
}

PackageAsset::~PackageAsset()
{
    if (texture_.is_valid()) {
        --stats().packages;
        stats().atlas_bytes -= atlas_texture_bytes_;
    }
}

RID PackageAsset::texture_rid() const
{
    return texture_.is_valid() ? texture_->get_rid() : RID();
}

std::string PackageAsset::skin_state_key(const spriteloop::SplaSkinState& skin_state) const
{
    int skin_index = skin_state.skin_index;
    if (skin_index < 0 || skin_index >= static_cast<int>(package_.skins.size()) ||
        package_.skins[static_cast<std::size_t>(skin_index)].parts.empty()) {
        skin_index = -1;
    }
    std::size_t override_count = skin_state.variant_overrides_by_part.size();
    while (override_count > 0 && skin_state.variant_overrides_by_part[override_count - 1] < 0) {
        --override_count;
    }
    std::string key = std::to_string(skin_index);
    for (std::size_t i = 0; i < override_count; ++i) {
        key += ',';
        key += std::to_string(std::max(-1, skin_state.variant_overrides_by_part[i]));
    }
    return key;
}

std::shared_ptr<BakedSet> PackageAsset::baked_set(const spriteloop::SplaSkinState& skin_state)
{
    const std::string key = skin_state_key(skin_state);
    auto existing = baked_sets_.find(key);
    if (existing != baked_sets_.end()) {
        if (std::shared_ptr<BakedSet> live = existing->second.lock()) {
            return live;
        }
    }

    auto set = std::make_shared<BakedSet>();
    const std::vector<spriteloop::SplaBakedImage> images =
        spriteloop::build_baked_images_from_atlas(package_, atlas_, image_map_, skin_state);
    set->bounds = spriteloop::calculate_baked_bounds(package_, images);
    set->animations = spriteloop::build_baked_animations(package_, atlas_, image_map_, skin_state);
    ++stats().baked_sets;
    ++stats().bakes;

    for (auto it = baked_sets_.begin(); it != baked_sets_.end();) {
        it = it->second.expired() ? baked_sets_.erase(it) : std::next(it);
    }
    baked_sets_[key] = set;
    return set;
}

std::size_t PackageAsset::live_baked_set_count() const
{
    std::size_t count = 0;
    for (const auto& entry : baked_sets_) {
        if (!entry.second.expired()) {
            ++count;
        }
    }
    return count;
}

} // namespace spriteloop_godot

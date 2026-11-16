#include "core/mask_cache.h"

#include "core/mask_texture.h"
#include "core/package_asset.h"
#include "core/stats.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

using namespace godot;

namespace spriteloop_godot {

namespace {

// Godot's largest supported 2D texture dimension.
constexpr int max_texture_dimension = 16384;

struct MaskGroup {
    std::size_t index_start = 0;
    std::size_t index_count = 0;
    int source = -1;
    float padding = 0.0f;
};

// Consecutive draws with the same mask source and padding form one group.
// Empty ordinary draws stay boundaries: joining across one changes soft-alpha
// composition even when UV clipping removed all of its triangles. Same rule as
// the Defold adapter.
std::vector<MaskGroup> mask_groups(const spriteloop::SplaBakedFrame& frame)
{
    std::vector<MaskGroup> groups;
    for (const spriteloop::SplaBakedDraw& draw : frame.draws) {
        if (!groups.empty()) {
            MaskGroup& group = groups.back();
            if (group.source == draw.mask_source_part_index &&
                (group.source < 0 || group.padding == draw.mask_padding) &&
                group.index_start + group.index_count == draw.index_start) {
                group.index_count += draw.index_count;
                continue;
            }
        }
        groups.push_back({draw.index_start, draw.index_count, draw.mask_source_part_index, draw.mask_padding});
    }
    return groups;
}

// Vertex arrays in 2D node space (SDK geometry is Y up).
struct Arrays {
    PackedVector2Array points;
    PackedVector2Array uvs;
    PackedColorArray colors;
};

Arrays to_arrays(const std::vector<spriteloop::SplaBakedVertex>& vertices)
{
    Arrays arrays;
    const int64_t count = static_cast<int64_t>(vertices.size());
    arrays.points.resize(count);
    arrays.uvs.resize(count);
    arrays.colors.resize(count);
    for (int64_t i = 0; i < count; ++i) {
        const spriteloop::SplaBakedVertex& v = vertices[static_cast<std::size_t>(i)];
        arrays.points.set(i, Vector2(v.x, -v.y));
        arrays.uvs.set(i, Vector2(v.u, v.v));
        arrays.colors.set(i, Color(v.r, v.g, v.b, v.a));
    }
    return arrays;
}

PackedInt32Array index_range(const std::vector<std::uint32_t>& indices, std::size_t start, std::size_t count)
{
    PackedInt32Array range;
    range.resize(static_cast<int64_t>(count));
    for (std::size_t i = 0; i < count; ++i) {
        range.set(static_cast<int64_t>(i), static_cast<int32_t>(indices[start + i]));
    }
    return range;
}

// Cache ---------------------------------------------------------------------

using Key = std::tuple<const BakedSet*, int, int, bool>;

struct Entry {
    std::shared_ptr<MaskFrame> frame;
    std::uint64_t touched = 0;
};

struct Cache {
    std::map<Key, Entry> entries;
    std::size_t budget = 32 * 1024 * 1024;
    std::size_t retained = 0;
    std::uint64_t clock = 0;
    RID composite_shader;
    RID straight_shader;
    RID straight_material;
    // Frames whose scratch passes are freed once the result has rendered.
    std::vector<std::weak_ptr<MaskFrame>> pending_scratch;

    void erase(std::map<Key, Entry>::iterator it)
    {
        retained -= it->second.frame->bytes();
        entries.erase(it);
    }

    // Evicts least recently used entries until `incoming` more bytes fit.
    // Nodes still showing an evicted frame keep it until they move on.
    void trim(std::size_t incoming = 0)
    {
        while (!entries.empty() && retained + incoming > budget) {
            auto oldest = entries.begin();
            for (auto it = entries.begin(); it != entries.end(); ++it) {
                if (it->second.touched < oldest->second.touched) {
                    oldest = it;
                }
            }
            erase(oldest);
        }
    }
};

Cache& cache()
{
    static Cache value;
    return value;
}

} // namespace

// ---------------------------------------------------------------------------
// MaskFrame

MaskFrame::~MaskFrame()
{
    release_scratch();
    free_pass(result_);
    if (RenderingServer::get_singleton() != nullptr) {
        for (const auto& entry : quads_3d_) {
            RenderingServer::get_singleton()->free_rid(entry.second);
        }
    }
    if (counted_) {
        --stats().mask_frames;
        stats().mask_frame_bytes -= bytes_;
    }
}

void MaskFrame::free_pass(Pass& pass)
{
    RenderingServer* rs = RenderingServer::get_singleton();
    if (rs != nullptr) {
        for (auto it = pass.items.rbegin(); it != pass.items.rend(); ++it) {
            rs->free_rid(*it);
        }
        for (const RID& material : pass.materials) {
            rs->free_rid(material);
        }
        if (pass.canvas.is_valid()) {
            rs->free_rid(pass.canvas);
        }
        if (pass.viewport.is_valid()) {
            rs->free_rid(pass.viewport);
        }
    }
    pass.items.clear();
    pass.materials.clear();
    pass.canvas = RID();
    pass.viewport = RID();
}

Ref<Texture2D> MaskFrame::texture_resource()
{
    if (Object* existing = ObjectDB::get_instance(texture_resource_)) {
        return Ref<Texture2D>(Object::cast_to<Texture2D>(existing));
    }
    Ref<SpriteLoopMaskTexture> texture;
    texture.instantiate();
    texture->set_frame(shared_from_this());
    texture_resource_ = ObjectID(texture->get_instance_id());
    return texture;
}

RID MaskFrame::quad_mesh_3d(float pixel_size)
{
    auto found = quads_3d_.find(pixel_size);
    if (found != quads_3d_.end()) {
        return found->second;
    }
    // Node space is Y down; 3D is Y up. Clockwise seen from +Z (Godot's front).
    const float left = rect_.position.x * pixel_size;
    const float right = rect_.get_end().x * pixel_size;
    const float top = -rect_.position.y * pixel_size;
    const float bottom = -rect_.get_end().y * pixel_size;
    PackedVector3Array positions;
    positions.push_back(Vector3(left, top, 0.0f));
    positions.push_back(Vector3(right, top, 0.0f));
    positions.push_back(Vector3(right, bottom, 0.0f));
    positions.push_back(Vector3(left, bottom, 0.0f));
    PackedVector3Array normals;
    PackedVector2Array uvs;
    PackedVector2Array layers; // UV2: draw-order layer 0 (see BakedSet::build_mesh).
    PackedColorArray colors;
    const Vector2 corner_uvs[4] = {Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1)};
    for (const Vector2& uv : corner_uvs) {
        normals.push_back(Vector3(0.0f, 0.0f, 1.0f));
        uvs.push_back(uv);
        layers.push_back(Vector2());
        colors.push_back(Color(1, 1, 1, 1));
    }
    PackedInt32Array indices;
    for (int index : {0, 1, 2, 0, 2, 3}) {
        indices.push_back(index);
    }
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = positions;
    arrays[Mesh::ARRAY_NORMAL] = normals;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_TEX_UV2] = layers;
    arrays[Mesh::ARRAY_COLOR] = colors;
    arrays[Mesh::ARRAY_INDEX] = indices;
    RenderingServer* rs = RenderingServer::get_singleton();
    const RID mesh = rs->mesh_create();
    rs->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_TRIANGLES, arrays);
    quads_3d_[pixel_size] = mesh;
    return mesh;
}

void MaskFrame::release_scratch()
{
    for (Pass& pass : scratch_) {
        free_pass(pass);
    }
    scratch_.clear();
}

bool frame_has_masks(const spriteloop::SplaBakedFrame& frame)
{
    return std::any_of(frame.draws.begin(), frame.draws.end(),
                       [](const spriteloop::SplaBakedDraw& draw) { return draw.mask_source_part_index >= 0; });
}

namespace {

// Group composite: premultiplied group times coverage, once (the SDK rule).
constexpr const char* composite_shader_code =
    "shader_type canvas_item;\n"
    "render_mode blend_premul_alpha, unshaded;\n"
    "uniform sampler2D coverage : filter_nearest;\n"
    "void fragment() {\n"
    "    COLOR = texture(TEXTURE, UV) * texture(coverage, UV).a;\n"
    "}\n";

// Converts the premultiplied composition to straight alpha, like every other
// Godot texture, so a masked node draws it on its own canvas item with normal
// blending (modulate, self_modulate, materials and lights work as usual).
// Transparent pixels take the average color of their visible neighbours, so
// linear filtering does not darken edges (Image::fix_alpha_edges does the same
// for the artwork atlas). Blending is off: the values are written as is.
constexpr const char* straight_shader_code =
    "shader_type canvas_item;\n"
    "render_mode blend_disabled, unshaded;\n"
    "void fragment() {\n"
    "    vec4 c = texture(TEXTURE, UV);\n"
    "    if (c.a > 0.0) {\n"
    "        COLOR = vec4(min(c.rgb / c.a, vec3(1.0)), c.a);\n"
    "    } else {\n"
    "        vec3 sum = vec3(0.0);\n"
    "        float count = 0.0;\n"
    "        for (int y = -1; y <= 1; y++) {\n"
    "            for (int x = -1; x <= 1; x++) {\n"
    "                vec4 n = texture(TEXTURE, UV + vec2(float(x), float(y)) * TEXTURE_PIXEL_SIZE);\n"
    "                if (n.a > 0.0) {\n"
    "                    sum += min(n.rgb / n.a, vec3(1.0));\n"
    "                    count += 1.0;\n"
    "                }\n"
    "            }\n"
    "        }\n"
    "        COLOR = vec4(count > 0.0 ? sum / count : vec3(0.0), 0.0);\n"
    "    }\n"
    "}\n";

RID straight_material()
{
    RenderingServer* rs = RenderingServer::get_singleton();
    Cache& c = cache();
    if (!c.straight_material.is_valid()) {
        c.straight_shader = rs->shader_create();
        rs->shader_set_code(c.straight_shader, straight_shader_code);
        c.straight_material = rs->material_create();
        rs->material_set_shader(c.straight_material, c.straight_shader);
    }
    return c.straight_material;
}

RID composite_shader()
{
    RID& shader = cache().composite_shader;
    if (!shader.is_valid()) {
        shader = RenderingServer::get_singleton()->shader_create();
        RenderingServer::get_singleton()->shader_set_code(shader, composite_shader_code);
    }
    return shader;
}

// An offscreen 2D pass of the frame's size, rendered once. Its canvas uses the
// frame's node-space coordinates.
MaskFrame::Pass create_pass(const Rect2& rect, bool nearest)
{
    RenderingServer* rs = RenderingServer::get_singleton();
    MaskFrame::Pass pass;
    pass.viewport = rs->viewport_create();
    rs->viewport_set_size(pass.viewport, static_cast<int>(rect.size.x), static_cast<int>(rect.size.y));
    rs->viewport_set_disable_3d(pass.viewport, true);
    rs->viewport_set_transparent_background(pass.viewport, true);
    rs->viewport_set_default_canvas_item_texture_filter(
        pass.viewport, nearest ? RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST
                               : RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_LINEAR);
    pass.canvas = rs->canvas_create();
    rs->viewport_attach_canvas(pass.viewport, pass.canvas);
    rs->viewport_set_canvas_transform(pass.viewport, pass.canvas, Transform2D(0.0f, -rect.position));
    rs->viewport_set_update_mode(pass.viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
    rs->viewport_set_active(pass.viewport, true);
    return pass;
}

RID add_item(MaskFrame::Pass& pass)
{
    RenderingServer* rs = RenderingServer::get_singleton();
    const RID item = rs->canvas_item_create();
    rs->canvas_item_set_parent(item, pass.canvas);
    rs->canvas_item_set_draw_index(item, static_cast<int>(pass.items.size()));
    pass.items.push_back(item);
    return item;
}

void add_triangles(RID item, const std::vector<std::uint32_t>& indices, std::size_t start, std::size_t count,
                   const Arrays& arrays, RID texture)
{
    RenderingServer::get_singleton()->canvas_item_add_triangle_array(
        item, index_range(indices, start, count), arrays.points, arrays.colors, arrays.uvs, PackedInt32Array(),
        PackedFloat32Array(), texture);
}

} // namespace

std::shared_ptr<MaskFrame> compose_mask_frame(const std::shared_ptr<PackageAsset>& asset,
                                              const spriteloop::SplaBakedFrame& frame, const Rect2& bounds,
                                              bool nearest)
{
    // Whole pixels with a one-pixel transparent border, so linear filtering of
    // the texture never clamps against visible artwork.
    const float left = std::floor(bounds.position.x) - 1.0f;
    const float top = std::floor(bounds.position.y) - 1.0f;
    const float right = std::ceil(bounds.position.x + bounds.size.x) + 1.0f;
    const float bottom = std::ceil(bounds.position.y + bounds.size.y) + 1.0f;
    const int width = std::max(1, static_cast<int>(right - left));
    const int height = std::max(1, static_cast<int>(bottom - top));
    if (width > max_texture_dimension || height > max_texture_dimension) {
        UtilityFunctions::push_error("SpriteLoop mask: frame bounds exceed the texture size limit (", width, "x",
                                     height, ", limit ", max_texture_dimension, ")");
        return nullptr;
    }
    const RID artwork = asset->texture_rid();
    if (!artwork.is_valid()) {
        return nullptr;
    }

    RenderingServer* rs = RenderingServer::get_singleton();
    auto result = std::make_shared<MaskFrame>();
    result->asset_ = asset;
    result->rect_ = Rect2(left, top, static_cast<float>(width), static_cast<float>(height));
    result->bytes_ = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    // Passes, each rendered before its parent viewport in the same frame:
    // group and coverage passes -> composition (premultiplied) -> result
    // (straight alpha, the cached texture). All but the result are scratch.
    result->result_ = create_pass(result->rect_, nearest);
    result->texture_ = rs->viewport_get_texture(result->result_.viewport);
    MaskFrame::Pass composition = create_pass(result->rect_, nearest);
    rs->viewport_set_parent_viewport(composition.viewport, result->result_.viewport);
    result->counted_ = true;
    ++stats().mask_frames;
    stats().mask_frame_bytes += result->bytes_;
    ++stats().mask_compositions;

    // Ordinary ranges draw straight into the composition. A mask group draws
    // its artwork and its coverage into two passes, which the composition then
    // adds with the composite shader. All passes start transparent and use mix
    // blending, so they hold premultiplied color, and overlapping coverage
    // accumulates as 1 - (1 - a1)(1 - a2).
    const Arrays art = to_arrays(frame.vertices);
    for (const MaskGroup& group : mask_groups(frame)) {
        if (group.index_count == 0) {
            continue;
        }
        if (group.source < 0) {
            add_triangles(add_item(composition), frame.indices, group.index_start, group.index_count, art,
                          artwork);
            continue;
        }
        // An absent or hidden source is zero coverage: the group is invisible.
        const auto source = std::find_if(
            frame.mask_sources.begin(), frame.mask_sources.end(),
            [&group](const spriteloop::SplaBakedMaskSource& value) { return value.part_index == group.source; });
        if (source == frame.mask_sources.end() || source->indices.empty() || source->vertices.empty()) {
            continue;
        }
        const RID alpha = asset->mask_alpha_texture_rid(group.padding, nearest);
        if (!alpha.is_valid()) {
            return nullptr;
        }
        MaskFrame::Pass group_pass = create_pass(result->rect_, nearest);
        add_triangles(add_item(group_pass), frame.indices, group.index_start, group.index_count, art, artwork);
        MaskFrame::Pass coverage_pass = create_pass(result->rect_, nearest);
        add_triangles(add_item(coverage_pass), source->indices, 0, source->indices.size(),
                      to_arrays(source->vertices), alpha);
        rs->viewport_set_parent_viewport(group_pass.viewport, composition.viewport);
        rs->viewport_set_parent_viewport(coverage_pass.viewport, composition.viewport);

        const RID material = rs->material_create();
        rs->material_set_shader(material, composite_shader());
        rs->material_set_param(material, "coverage", rs->viewport_get_texture(coverage_pass.viewport));
        composition.materials.push_back(material);
        const RID composite = add_item(composition);
        rs->canvas_item_set_material(composite, material);
        rs->canvas_item_set_default_texture_filter(composite, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
        rs->canvas_item_add_texture_rect(composite, result->rect_, rs->viewport_get_texture(group_pass.viewport));

        result->scratch_.push_back(std::move(group_pass));
        result->scratch_.push_back(std::move(coverage_pass));
    }
    const RID output = add_item(result->result_);
    rs->canvas_item_set_material(output, straight_material());
    rs->canvas_item_set_default_texture_filter(output, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
    rs->canvas_item_add_texture_rect(output, result->rect_, rs->viewport_get_texture(composition.viewport));
    result->scratch_.push_back(std::move(composition));
    result->scratch_frame_ = Engine::get_singleton()->get_frames_drawn();
    return result;
}

} // namespace spriteloop_godot

namespace godot {

RID SpriteLoopMaskTexture::_get_rid() const
{
    return frame_ ? frame_->texture() : RID();
}

int32_t SpriteLoopMaskTexture::_get_width() const
{
    return frame_ ? static_cast<int32_t>(frame_->rect().size.x) : 0;
}

int32_t SpriteLoopMaskTexture::_get_height() const
{
    return frame_ ? static_cast<int32_t>(frame_->rect().size.y) : 0;
}

} // namespace godot

namespace spriteloop_godot {

// ---------------------------------------------------------------------------
// Cache

namespace mask_cache {

std::shared_ptr<MaskFrame> get(const BakedSet& set, const std::shared_ptr<PackageAsset>& asset, int animation_index,
                               int frame_index, bool nearest)
{
    Cache& c = cache();
    // A result renders in the frame it was composed; its scratch passes are
    // no longer needed two frames later.
    const std::uint64_t now = Engine::get_singleton()->get_frames_drawn();
    for (auto it = c.pending_scratch.begin(); it != c.pending_scratch.end();) {
        std::shared_ptr<MaskFrame> pending = it->lock();
        if (!pending || now >= pending->scratch_frame() + 2) {
            if (pending) {
                pending->release_scratch();
            }
            it = c.pending_scratch.erase(it);
        } else {
            ++it;
        }
    }
    const Key key(&set, animation_index, frame_index, nearest);
    auto found = c.entries.find(key);
    if (found != c.entries.end()) {
        found->second.touched = ++c.clock;
        ++stats().mask_cache_hits;
        return found->second.frame;
    }
    const spriteloop::SplaBakedFrame* frame = set.frame(animation_index, frame_index);
    if (frame == nullptr || !asset) {
        return nullptr;
    }
    std::shared_ptr<MaskFrame> composed = compose_mask_frame(asset, *frame, set.bounds_2d(), nearest);
    if (!composed) {
        return nullptr;
    }
    if (composed->has_scratch()) {
        c.pending_scratch.push_back(composed);
    }
    if (composed->bytes() <= c.budget) {
        c.trim(composed->bytes());
        c.entries[key] = Entry{composed, ++c.clock};
        c.retained += composed->bytes();
    }
    return composed;
}

void forget(const BakedSet& set)
{
    Cache& c = cache();
    for (auto it = c.entries.begin(); it != c.entries.end();) {
        if (std::get<0>(it->first) == &set) {
            auto next = std::next(it);
            c.erase(it);
            it = next;
        } else {
            ++it;
        }
    }
}

void set_budget(std::size_t bytes)
{
    cache().budget = bytes;
    cache().trim();
}

std::size_t budget()
{
    return cache().budget;
}

std::size_t retained_bytes()
{
    return cache().retained;
}

void clear()
{
    cache().pending_scratch.clear();
    cache().entries.clear();
    cache().retained = 0;
    for (RID* rid : {&cache().straight_material, &cache().straight_shader, &cache().composite_shader}) {
        if (rid->is_valid()) {
            RenderingServer::get_singleton()->free_rid(*rid);
            *rid = RID();
        }
    }
}

} // namespace mask_cache

} // namespace spriteloop_godot

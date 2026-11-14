#include "core/material_3d.h"

#include "gen/spriteloop_3d_shader.gen.h"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <map>
#include <tuple>
#include <utility>

using namespace godot;

namespace spriteloop_godot {

namespace {

std::map<std::uint32_t, Ref<Shader>>& shaders()
{
    static std::map<std::uint32_t, Ref<Shader>> cache;
    return cache;
}

// Weak cache keyed by (texture instance ID, options) -> material instance ID.
// Nodes own the materials; a material (and the atlas it references) is freed
// when its last node lets go, and its entry is pruned on the next insert.
// (texture, shader key | uniform flags, render priority, modulate rgba,
//  alpha threshold, custom template id, custom template version, offset xy)
using MaterialKey = std::tuple<std::uint64_t, std::uint32_t, int, float, float, float, float, float,
                               std::uint64_t, std::uint64_t, float, float>;

std::map<MaterialKey, std::uint64_t>& materials()
{
    static std::map<MaterialKey, std::uint64_t> cache;
    return cache;
}

ShaderMaterial* live_material(std::uint64_t id)
{
    return Object::cast_to<ShaderMaterial>(ObjectDB::get_instance(id));
}

// Custom material templates -> version. One listener per template bumps the
// version when it emits `changed`, so copies made earlier no longer match the
// cache key; nodes re-request their material after the change.
std::map<std::uint64_t, std::uint64_t>& templates()
{
    static std::map<std::uint64_t, std::uint64_t> versions;
    return versions;
}

void on_template_changed(std::uint64_t template_id)
{
    ++templates()[template_id];
}

std::uint64_t watch_template(const Ref<ShaderMaterial>& material)
{
    const std::uint64_t id = material->get_instance_id();
    auto& versions = templates();
    for (auto it = versions.begin(); it != versions.end();) {
        it = ObjectDB::get_instance(it->first) == nullptr ? versions.erase(it) : std::next(it);
    }
    if (versions.count(id) == 0) {
        versions[id] = 0;
        material->connect("changed", callable_mp_static(&on_template_changed).bind(id));
    }
    return versions[id];
}

// The shader is the addon's spriteloop_3d.gdshader (with its include),
// embedded at build time. A variant only selects options through defines.
// Only options that change render modes or the alpha output are defines;
// billboard mode, filtering, flips and tint are uniforms, because every
// variant is compiled on first use (hundreds of ms in the Compatibility
// renderer, every run).
String shader_code(const Material3DOptions& options)
{
    String defines;
    const auto define = [&defines](const char* name) {
        defines += String("#define ") + name + "\n";
    };
    if (options.shaded) {
        define("SPRITELOOP_SHADED");
    }
    if (!options.double_sided) {
        // Mirroring reverses triangle winding; a mirrored single-sided
        // character must cull the other side to stay visible.
        define(options.flip_h != options.flip_v ? "SPRITELOOP_CULL_FRONT" : "SPRITELOOP_CULL_BACK");
    }
    if (options.no_depth_test) {
        define("SPRITELOOP_NO_DEPTH_TEST");
    }
    switch (options.alpha_cut) {
    case Material3DOptions::alpha_discard:
        define("SPRITELOOP_ALPHA_DISCARD");
        break;
    case Material3DOptions::alpha_opaque_prepass:
        define("SPRITELOOP_ALPHA_PREPASS");
        break;
    case Material3DOptions::alpha_hash:
        define("SPRITELOOP_ALPHA_HASH");
        break;
    case Material3DOptions::alpha_blend:
        break;
    }
    return defines + String::utf8(SPRITELOOP_3D_SHADER);
}

} // namespace

std::uint32_t Material3DOptions::shader_key() const
{
    return static_cast<std::uint32_t>(alpha_cut) | (shaded ? 1u << 2 : 0u) | (double_sided ? 1u << 3 : 0u) |
           (no_depth_test ? 1u << 4 : 0u) | (!double_sided && flip_h != flip_v ? 1u << 5 : 0u);
}

Ref<ShaderMaterial> material_3d(const Ref<Texture2D>& texture, const Material3DOptions& options)
{
    if (texture.is_null()) {
        return Ref<ShaderMaterial>();
    }
    const bool custom = options.custom.is_valid() && options.custom->get_shader().is_valid();
    // Custom shaders bring their own render modes; only uniforms vary.
    const std::uint32_t shader_key = custom ? 0u : options.shader_key();
    const bool discard = custom || options.alpha_cut == Material3DOptions::alpha_discard;
    // Uniform-driven options: flips, filter and billboard mode.
    const std::uint32_t uniforms = (options.flip_h ? 1u : 0u) | (options.flip_v ? 2u : 0u) |
                                   (options.nearest ? 4u : 0u) | (static_cast<std::uint32_t>(options.billboard) << 3);
    std::uint64_t template_id = 0;
    std::uint64_t template_version = 0;
    if (custom) {
        template_id = options.custom->get_instance_id();
        template_version = watch_template(options.custom);
    }
    auto& cache = materials();
    const MaterialKey material_key(texture->get_instance_id(), shader_key | (uniforms << 8),
                                   options.render_priority, options.modulate.r, options.modulate.g,
                                   options.modulate.b, options.modulate.a,
                                   discard ? options.alpha_scissor_threshold : 0.0f, template_id,
                                   template_version, options.offset.x, options.offset.y);
    auto existing = cache.find(material_key);
    if (existing != cache.end()) {
        if (ShaderMaterial* live = live_material(existing->second)) {
            return Ref<ShaderMaterial>(live);
        }
    }

    for (auto it = cache.begin(); it != cache.end();) {
        it = live_material(it->second) == nullptr ? cache.erase(it) : std::next(it);
    }

    Ref<ShaderMaterial> material;
    if (custom) {
        // A copy keeps the user's shader and parameter values; the SpriteLoop
        // uniforms below are per look.
        material = options.custom->duplicate(false);
    } else {
        Ref<Shader>& shader = shaders()[shader_key];
        if (shader.is_null()) {
            shader.instantiate();
            shader->set_code(shader_code(options));
        }
        material.instantiate();
        material->set_shader(shader);
    }
    material->set_shader_parameter("atlas_linear", texture);
    material->set_shader_parameter("atlas_nearest", texture);
    material->set_shader_parameter("nearest", options.nearest);
    material->set_shader_parameter("billboard", static_cast<int>(options.billboard));
    material->set_shader_parameter("modulate", options.modulate);
    material->set_shader_parameter("flip", Vector2(options.flip_h ? -1.0f : 1.0f, options.flip_v ? -1.0f : 1.0f));
    material->set_shader_parameter("sprite_offset", options.offset);
    if (discard) {
        material->set_shader_parameter("alpha_scissor_threshold", options.alpha_scissor_threshold);
    }
    material->set_render_priority(options.render_priority);
    cache[material_key] = material->get_instance_id();
    return material;
}

bool is_material_3d_template(const Object* material)
{
    return material != nullptr && templates().count(material->get_instance_id()) > 0;
}

void refresh_material_3d_templates()
{
    // set_shader_parameter() does not emit `changed`; compare parameter values
    // with the last snapshot and announce real changes.
    static std::map<std::uint64_t, Dictionary> snapshots;
    for (const auto& entry : templates()) {
        ShaderMaterial* material = live_material(entry.first);
        if (material == nullptr || material->get_shader().is_null()) {
            continue;
        }
        Dictionary values;
        const Array uniforms = material->get_shader()->get_shader_uniform_list(true);
        for (int64_t i = 0; i < uniforms.size(); ++i) {
            const Dictionary uniform = uniforms[i];
            const StringName name = uniform.get("name", StringName());
            values[name] = material->get_shader_parameter(name);
        }
        auto previous = snapshots.find(entry.first);
        if (previous == snapshots.end()) {
            snapshots[entry.first] = values;
        } else if (!previous->second.recursive_equal(values, 1)) {
            previous->second = values;
            material->emit_changed();
        }
    }
    for (auto it = snapshots.begin(); it != snapshots.end();) {
        it = templates().count(it->first) == 0 ? snapshots.erase(it) : std::next(it);
    }
}

std::size_t live_material_3d_count()
{
    std::size_t count = 0;
    for (const auto& entry : materials()) {
        if (live_material(entry.second) != nullptr) {
            ++count;
        }
    }
    return count;
}

void clear_material_3d_cache()
{
    // Disconnect from templates that outlive the extension.
    for (const auto& entry : templates()) {
        if (Resource* resource = Object::cast_to<Resource>(ObjectDB::get_instance(entry.first))) {
            const Callable callable = callable_mp_static(&on_template_changed).bind(entry.first);
            if (resource->is_connected("changed", callable)) {
                resource->disconnect("changed", callable);
            }
        }
    }
    templates().clear();
    materials().clear();
    shaders().clear();
}

} // namespace spriteloop_godot

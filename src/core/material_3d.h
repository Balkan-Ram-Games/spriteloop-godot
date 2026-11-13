#pragma once

#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cstddef>
#include <cstdint>

namespace spriteloop_godot {

// Options of a SpriteLoop3D material, packed into a cache key.
struct Material3DOptions {
    enum Billboard : std::uint8_t { billboard_disabled = 0, billboard_enabled = 1, billboard_fixed_y = 2 };
    enum AlphaCut : std::uint8_t { alpha_blend = 0, alpha_discard = 1, alpha_opaque_prepass = 2, alpha_hash = 3 };

    Billboard billboard = billboard_disabled;
    // Opaque pre-pass by default: correct depth against the world and shadows,
    // with soft blended edges.
    AlphaCut alpha_cut = alpha_opaque_prepass;
    bool shaded = false;
    bool double_sided = true;
    bool no_depth_test = false;
    bool nearest = false;
    bool flip_h = false;
    bool flip_v = false;
    int render_priority = 0;
    float alpha_scissor_threshold = 0.5f;
    godot::Color modulate = godot::Color(1, 1, 1, 1);
    // Optional user material whose shader includes spriteloop_3d.gdshaderinc.
    // Replaces the built-in shader; shaded, double_sided, no_depth_test and
    // alpha_cut then come from that shader's render modes.
    godot::Ref<godot::ShaderMaterial> custom;

    // Options that select a built-in shader variant.
    std::uint32_t shader_key() const;
};

// Returns a material for a package atlas, cached weakly per texture and option
// values, so nodes that look the same share one material.
//
// Built-in: one shader variant per shader_key(), from the embedded
// spriteloop_3d.gdshader. Custom: a copy of the user's material per look, with
// the SpriteLoop uniforms filled in; copies are rebuilt after the user's
// material emits `changed`.
//
// Per-node looks are plain material parameters, not instance uniforms: those
// live in a fixed global buffer that large crowds overflow.
godot::Ref<godot::ShaderMaterial> material_3d(const godot::Ref<godot::Texture2D>& texture,
                                              const Material3DOptions& options);

// True if the material is used as a custom SpriteLoop3D material template.
bool is_material_3d_template(const godot::Object* material);

// Emits `changed` on templates whose shader parameters changed since the last
// call, so their copies are rebuilt. The editor calls this after every
// undo/redo step (inspector edits do not emit `changed`).
void refresh_material_3d_templates();

// Number of live SpriteLoop3D materials (built-in and custom copies).
std::size_t live_material_3d_count();

// Releases cached shaders and materials. Called when the extension unloads,
// while the rendering server still exists.
void clear_material_3d_cache();

} // namespace spriteloop_godot

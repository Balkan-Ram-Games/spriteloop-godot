#pragma once

#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cstdint>

namespace spriteloop_godot {

// Options of the generated SpriteLoop3D material, packed into a cache key.
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

    // Options that change the generated shader code.
    std::uint32_t shader_key() const;
};

// Returns a material for a package atlas, cached weakly per texture and option
// values, so nodes that look the same share one material. Shaders are cached
// per shader_key(). Per-node looks are plain material parameters, not
// instance uniforms: those live in a fixed global buffer that large crowds
// overflow.
godot::Ref<godot::ShaderMaterial> material_3d(const godot::Ref<godot::Texture2D>& texture,
                                              const Material3DOptions& options);

// Releases cached shaders and materials. Called when the extension unloads,
// while the rendering server still exists.
void clear_material_3d_cache();

} // namespace spriteloop_godot

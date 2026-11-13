#include "core/material_3d.h"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/core/object.hpp>

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
using MaterialKey = std::tuple<std::uint64_t, std::uint32_t, int, float, float, float, float, float>;

std::map<MaterialKey, std::uint64_t>& materials()
{
    static std::map<MaterialKey, std::uint64_t> cache;
    return cache;
}

ShaderMaterial* live_material(std::uint64_t id)
{
    return Object::cast_to<ShaderMaterial>(ObjectDB::get_instance(id));
}

// Only options that need different render modes or fragment outputs create
// separate shaders. Billboard mode, filtering, flips and tint are uniforms:
// every extra shader is compiled on first use, which stalls the frame
// (hundreds of ms in the Compatibility renderer, every run).
String shader_code(const Material3DOptions& options)
{
    // Vertices are transformed here so draw layers can be offset in view space.
    String modes = "blend_mix, depth_draw_opaque, skip_vertex_transform";
    // Mirroring reverses triangle winding; a mirrored single-sided character
    // must cull the other side to stay visible.
    const bool mirrored = options.flip_h != options.flip_v;
    modes += options.double_sided ? ", cull_disabled" : (mirrored ? ", cull_front" : ", cull_back");
    if (!options.shaded) {
        modes += ", unshaded";
    }
    if (options.no_depth_test) {
        modes += ", depth_test_disabled";
    }
    if (options.alpha_cut == Material3DOptions::alpha_opaque_prepass) {
        modes += ", depth_prepass_alpha";
    }

    String code = "shader_type spatial;\nrender_mode " + modes + ";\n\n";
    code += "uniform sampler2D atlas_linear : source_color, filter_linear, repeat_disable;\n";
    code += "uniform sampler2D atlas_nearest : source_color, filter_nearest, repeat_disable;\n";
    code += "uniform bool nearest = false;\n";
    code += "uniform int billboard = 0;\n";
    code += "uniform vec4 modulate : source_color = vec4(1.0);\n";
    code += "uniform vec2 flip = vec2(1.0);\n";
    if (options.alpha_cut == Material3DOptions::alpha_discard) {
        code += "uniform float alpha_scissor_threshold = 0.5;\n";
    }
    code += "\nvoid vertex() {\n";
    code += "\tVERTEX.xy *= flip;\n";
    // Mirrored geometry shows its back face to the viewer. Invert the normal so,
    // after Godot's back-face normal flip, it is lit like the unflipped side.
    code += "\tNORMAL *= sign(flip.x * flip.y);\n";
    // Billboard code follows BaseMaterial3D (scale kept, as Sprite3D does).
    code += "\tif (billboard == 1) {\n";
    code += "\t\tMODELVIEW_MATRIX = VIEW_MATRIX * mat4(MAIN_CAM_INV_VIEW_MATRIX[0], MAIN_CAM_INV_VIEW_MATRIX[1], "
            "MAIN_CAM_INV_VIEW_MATRIX[2], MODEL_MATRIX[3]);\n";
    code += "\t} else if (billboard == 2) {\n";
    code += "\t\tMODELVIEW_MATRIX = VIEW_MATRIX * mat4(vec4(normalize(cross(vec3(0.0, 1.0, 0.0), "
            "MAIN_CAM_INV_VIEW_MATRIX[2].xyz)), 0.0), vec4(0.0, 1.0, 0.0, 0.0), "
            "vec4(normalize(cross(MAIN_CAM_INV_VIEW_MATRIX[0].xyz, vec3(0.0, 1.0, 0.0))), 0.0), "
            "MODEL_MATRIX[3]);\n";
    code += "\t}\n";
    code += "\tif (billboard != 0) {\n";
    code += "\t\tMODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0), "
            "vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0), vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0), "
            "vec4(0.0, 0.0, 0.0, 1.0));\n";
    code += "\t\tMODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);\n";
    code += "\t}\n";
    code += "\tVERTEX = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;\n";
    code += "\tNORMAL = normalize(MODELVIEW_NORMAL_MATRIX * NORMAL);\n";
    // Later draws move towards the viewer, from whichever side the character is
    // seen, so depth-writing modes keep painter's order. Scaling along the view
    // ray keeps the screen position; orthographic views (directional shadow
    // maps, orthogonal cameras) step along view Z by the baked world distance.
    code += "\tif (PROJECTION_MATRIX[3][3] != 0.0) {\n";
    code += "\t\tVERTEX.z += UV2.y;\n";
    code += "\t} else {\n";
    code += "\t\tVERTEX *= 1.0 - UV2.x * 0.0002;\n";
    code += "\t}\n";
    // Vertex colors carry authored tint (sRGB) and opacity; light in linear.
    code += "\tCOLOR.rgb = mix(pow((COLOR.rgb + vec3(0.055)) * (1.0 / 1.055), vec3(2.4)), "
            "COLOR.rgb * (1.0 / 12.92), lessThan(COLOR.rgb, vec3(0.04045)));\n";
    code += "}\n\nvoid fragment() {\n";
    code += "\tvec4 texel = nearest ? texture(atlas_nearest, UV) : texture(atlas_linear, UV);\n";
    code += "\tvec4 color = texel * COLOR * modulate;\n";
    code += "\tALBEDO = color.rgb;\n";
    code += "\tALPHA = color.a;\n";
    if (options.alpha_cut == Material3DOptions::alpha_discard) {
        code += "\tALPHA_SCISSOR_THRESHOLD = alpha_scissor_threshold;\n";
    } else if (options.alpha_cut == Material3DOptions::alpha_hash) {
        code += "\tALPHA_HASH_SCALE = 1.0;\n";
    }
    code += "}\n";
    return code;
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
    const std::uint32_t shader_key = options.shader_key();
    const bool discard = options.alpha_cut == Material3DOptions::alpha_discard;
    // Uniform-driven options: flips, filter and billboard mode.
    const std::uint32_t uniforms = (options.flip_h ? 1u : 0u) | (options.flip_v ? 2u : 0u) |
                                   (options.nearest ? 4u : 0u) | (static_cast<std::uint32_t>(options.billboard) << 3);
    auto& cache = materials();
    const MaterialKey material_key(texture->get_instance_id(), shader_key | (uniforms << 8),
                                   options.render_priority, options.modulate.r, options.modulate.g,
                                   options.modulate.b, options.modulate.a,
                                   discard ? options.alpha_scissor_threshold : 0.0f);
    auto existing = cache.find(material_key);
    if (existing != cache.end()) {
        if (ShaderMaterial* live = live_material(existing->second)) {
            return Ref<ShaderMaterial>(live);
        }
    }

    Ref<Shader>& shader = shaders()[shader_key];
    if (shader.is_null()) {
        shader.instantiate();
        shader->set_code(shader_code(options));
    }

    for (auto it = cache.begin(); it != cache.end();) {
        it = live_material(it->second) == nullptr ? cache.erase(it) : std::next(it);
    }

    Ref<ShaderMaterial> material;
    material.instantiate();
    material->set_shader(shader);
    material->set_shader_parameter("atlas_linear", texture);
    material->set_shader_parameter("atlas_nearest", texture);
    material->set_shader_parameter("nearest", options.nearest);
    material->set_shader_parameter("billboard", static_cast<int>(options.billboard));
    material->set_shader_parameter("modulate", options.modulate);
    material->set_shader_parameter("flip", Vector2(options.flip_h ? -1.0f : 1.0f, options.flip_v ? -1.0f : 1.0f));
    if (discard) {
        material->set_shader_parameter("alpha_scissor_threshold", options.alpha_scissor_threshold);
    }
    material->set_render_priority(options.render_priority);
    cache[material_key] = material->get_instance_id();
    return material;
}

void clear_material_3d_cache()
{
    materials().clear();
    shaders().clear();
}

} // namespace spriteloop_godot

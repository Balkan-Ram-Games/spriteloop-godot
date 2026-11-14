// SpriteLoop3D shader core: uniforms, the vertex step and the base color.
//
// Used by the built-in spriteloop_3d.gdshader. Custom shaders for
// SpriteLoop3D.material include it to keep part layering, flips, billboarding
// and authored tint, and add their own look in fragment():
//
//     shader_type spatial;
//     render_mode skip_vertex_transform, cull_disabled, depth_prepass_alpha;  // skip_vertex_transform is required
//     #include "res://addons/spriteloop/shaders/spriteloop_3d.gdshaderinc"
//
//     void vertex() {
//         SPRITELOOP_VERTEX;
//     }
//
//     void fragment() {
//         vec4 color = spriteloop_color(UV, COLOR);
//         ALBEDO = color.rgb;
//         ALPHA = color.a;
//     }
//
// The uniforms below are set by SpriteLoop3D from its properties (atlas,
// texture_filter, billboard, modulate, flip_h/flip_v, offset);
// they appear under "Spriteloop" in the material inspector. Do not set them
// yourself. Assign the material to SpriteLoop3D.material, not to Material
// Override, so the node fills them in.

group_uniforms spriteloop;
uniform sampler2D atlas_linear : source_color, filter_linear, repeat_disable;
uniform sampler2D atlas_nearest : source_color, filter_nearest, repeat_disable;
// texture_filter: true for TEXTURE_FILTER_NEAREST.
uniform bool nearest = false;
// billboard: 0 disabled, 1 enabled, 2 Y-billboard.
uniform int billboard = 0;
uniform vec4 modulate : source_color = vec4(1.0);
// flip_h / flip_v: -1 mirrors that axis.
uniform vec2 flip = vec2(1.0);
// offset, in world units (pixels * pixel_size). Applied before flipping, so a
// flipped character mirrors around its node origin.
uniform vec2 sprite_offset = vec2(0.0);
group_uniforms;
// SpriteLoop3D also sets alpha_scissor_threshold. Declare it in a shader that
// uses alpha scissor:
//     uniform float alpha_scissor_threshold = 0.5;
//     ... ALPHA_SCISSOR_THRESHOLD = alpha_scissor_threshold;

// Relative step towards the camera per draw layer (perspective views).
const float SPRITELOOP_LAYER_STEP = 0.0002;

// Atlas texel times the part's vertex color (tint and opacity) and modulate.
vec4 spriteloop_color(vec2 uv, vec4 vertex_color) {
	vec4 texel = nearest ? texture(atlas_nearest, uv) : texture(atlas_linear, uv);
	return texel * vertex_color * modulate;
}

// Vertex step for vertex(). Needs render_mode skip_vertex_transform: it
// transforms VERTEX and NORMAL to view space itself.
//
// 1. The offset moves the artwork; flips then mirror it around the node origin. Mirrored geometry shows its back face, so the
//    normal is inverted to keep the same lighting after Godot's back-face flip.
// 2. Billboard modes follow BaseMaterial3D and keep the node's scale, as
//    Sprite3D does.
// 3. All parts of a character share one plane. UV2.x holds each vertex's draw
//    layer; later layers move towards the viewer along the view ray, so
//    depth-writing modes and shadows keep the authored painter's order from
//    the front, from behind and when flipped. Scaling along the ray keeps the
//    screen position; orthographic views (directional shadow maps) step by
//    the world distance baked in UV2.y.
// 4. Vertex colors carry authored tint in sRGB; lighting works in linear.
#define SPRITELOOP_VERTEX \
	VERTEX.xy = (VERTEX.xy + sprite_offset) * flip; \
	NORMAL *= sign(flip.x * flip.y); \
	if (billboard == 1) { \
		MODELVIEW_MATRIX = VIEW_MATRIX * mat4(MAIN_CAM_INV_VIEW_MATRIX[0], MAIN_CAM_INV_VIEW_MATRIX[1], MAIN_CAM_INV_VIEW_MATRIX[2], MODEL_MATRIX[3]); \
	} else if (billboard == 2) { \
		MODELVIEW_MATRIX = VIEW_MATRIX * mat4(vec4(normalize(cross(vec3(0.0, 1.0, 0.0), MAIN_CAM_INV_VIEW_MATRIX[2].xyz)), 0.0), vec4(0.0, 1.0, 0.0, 0.0), vec4(normalize(cross(MAIN_CAM_INV_VIEW_MATRIX[0].xyz, vec3(0.0, 1.0, 0.0))), 0.0), MODEL_MATRIX[3]); \
	} \
	if (billboard != 0) { \
		MODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0), vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0), vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0), vec4(0.0, 0.0, 0.0, 1.0)); \
		MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX); \
	} \
	VERTEX = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz; \
	NORMAL = normalize(MODELVIEW_NORMAL_MATRIX * NORMAL); \
	if (PROJECTION_MATRIX[3][3] > 0.5) { \
		VERTEX.z += UV2.y; \
	} else { \
		VERTEX *= 1.0 - UV2.x * SPRITELOOP_LAYER_STEP; \
	} \
	COLOR.rgb = mix(pow((COLOR.rgb + vec3(0.055)) * (1.0 / 1.055), vec3(2.4)), COLOR.rgb * (1.0 / 12.92), lessThan(COLOR.rgb, vec3(0.04045)))

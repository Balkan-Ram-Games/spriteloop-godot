// SpriteLoop3D built-in shader.
//
// SpriteLoop3D compiles one variant of this file per combination of its
// render-mode options, selected by defines it prepends:
//
//   SPRITELOOP_SHADED          react to lights (otherwise unshaded)
//   SPRITELOOP_CULL_BACK       single-sided (otherwise double-sided)
//   SPRITELOOP_CULL_FRONT      single-sided and mirrored (one flip axis)
//   SPRITELOOP_NO_DEPTH_TEST   draw on top of everything
//   SPRITELOOP_ALPHA_DISCARD   alpha cut: discard below a threshold
//   SPRITELOOP_ALPHA_PREPASS   alpha cut: opaque depth pre-pass
//   SPRITELOOP_ALPHA_HASH      alpha cut: hashed transparency
//
// Without defines it is the default look minus the pre-pass. The build embeds
// this file, with the include inlined, into the extension. Billboard mode,
// filtering, flips and tint are uniforms, so they do not create variants;
// every variant is compiled on first use, which costs a frame hitch.

shader_type spatial;

#if defined(SPRITELOOP_CULL_FRONT)
#define SPRITELOOP_CULL_MODE cull_front
#elif defined(SPRITELOOP_CULL_BACK)
#define SPRITELOOP_CULL_MODE cull_back
#else
#define SPRITELOOP_CULL_MODE cull_disabled
#endif

#ifdef SPRITELOOP_SHADED
#define SPRITELOOP_SHADING_MODE
#else
#define SPRITELOOP_SHADING_MODE , unshaded
#endif

#ifdef SPRITELOOP_NO_DEPTH_TEST
#define SPRITELOOP_DEPTH_TEST_MODE , depth_test_disabled
#else
#define SPRITELOOP_DEPTH_TEST_MODE
#endif

#ifdef SPRITELOOP_ALPHA_PREPASS
#define SPRITELOOP_PREPASS_MODE , depth_prepass_alpha
#else
#define SPRITELOOP_PREPASS_MODE
#endif

// skip_vertex_transform: SPRITELOOP_VERTEX transforms to view space itself.
render_mode blend_mix, depth_draw_opaque, skip_vertex_transform, SPRITELOOP_CULL_MODE SPRITELOOP_SHADING_MODE SPRITELOOP_DEPTH_TEST_MODE SPRITELOOP_PREPASS_MODE;

#include "res://addons/spriteloop/shaders/spriteloop_3d.gdshaderinc"

#ifdef SPRITELOOP_ALPHA_DISCARD
group_uniforms spriteloop;
uniform float alpha_scissor_threshold = 0.5;
group_uniforms;
#endif

void vertex() {
	SPRITELOOP_VERTEX;
}

void fragment() {
	vec4 color = spriteloop_color(UV, COLOR);
	ALBEDO = color.rgb;
	ALPHA = color.a;
#if defined(SPRITELOOP_ALPHA_DISCARD)
	ALPHA_SCISSOR_THRESHOLD = alpha_scissor_threshold;
#elif defined(SPRITELOOP_ALPHA_HASH)
	ALPHA_HASH_SCALE = 1.0;
#endif
}

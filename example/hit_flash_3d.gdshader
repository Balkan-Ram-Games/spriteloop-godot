// Example custom look for SpriteLoop3D.material: a hit flash.
//
// The include keeps SpriteLoop layering, flips, billboarding and authored tint;
// this shader only changes the color in fragment().
shader_type spatial;
render_mode skip_vertex_transform, cull_disabled, depth_prepass_alpha, unshaded;

#include "res://addons/spriteloop/shaders/spriteloop_3d.gdshaderinc"

// Shared by every character that uses this material.
uniform vec4 flash_color : source_color = vec4(1.0, 0.25, 0.2, 1.0);
// Per character: node.set_instance_shader_parameter("flash", amount).
instance uniform float flash = 0.0;

void vertex() {
	SPRITELOOP_VERTEX;
}

void fragment() {
	vec4 color = spriteloop_color(UV, COLOR);
	ALBEDO = mix(color.rgb, flash_color.rgb, flash * flash_color.a);
	ALPHA = color.a;
}

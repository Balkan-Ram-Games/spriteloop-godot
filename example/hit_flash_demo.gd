extends SpriteLoop3D
## Flashes this character every second through the per-character `flash`
## instance uniform of example/hit_flash_3d.gdshader.

const INTERVAL := 1.2
const FADE := 0.25

var _time := 0.0


func _process(delta: float) -> void:
	_time = fmod(_time + delta, INTERVAL)
	set_instance_shader_parameter(&"flash", clampf(1.0 - _time / FADE, 0.0, 1.0))

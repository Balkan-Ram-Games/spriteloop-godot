extends RefCounted
## Bullet patterns. The characters decide when to fire: on the `shoot` events
## of their animations.

const BULLET := preload("res://example/arena/bullet.tscn")
const PATTERNS := {
	&"red": {
		"angles": [0.0], "speed": 420.0, "scale": 0.055, "team": &"player", "pitch": 1.0,
		"texture": preload("res://example/arena/assets/images/red_bullet.png"),
	},
	&"blue": {
		"angles": [-14.0, -7.0, 0.0, 7.0, 14.0], "speed": 460.0, "scale": 0.05, "team": &"player",
		"pitch": 0.85, "texture": preload("res://example/arena/assets/images/blue_bullet.png"),
	},
	&"turret": {
		"angles": [0.0], "speed": 260.0, "scale": 0.22, "team": &"turret", "pitch": 0.0,
		"texture": preload("res://example/arena/assets/images/turret_bullet.png"),
	},
}


## Fires `pattern` from the muzzle attachment of `shooter` (a character whose
## position is its spot on the ground). `facing` is 1 for right, -1 for left.
static func fire(shooter: Node2D, muzzle: Node2D, pattern_name: StringName, facing: float) -> void:
	var pattern: Dictionary = PATTERNS[pattern_name]
	var muzzle_position := muzzle.global_position
	for degrees: float in pattern.angles:
		var angle := deg_to_rad(degrees)
		var bullet: Node2D = BULLET.instantiate()
		bullet.arena = shooter.arena
		bullet.team = pattern.team
		# The bullet travels along the shooter's ground lane, drawn at muzzle height.
		bullet.position = Vector2(muzzle_position.x, shooter.position.y)
		bullet.height = shooter.position.y - muzzle_position.y
		bullet.velocity = Vector2(facing * cos(angle), -sin(angle)) * float(pattern.speed)
		var sprite: Sprite2D = bullet.get_node(^"Sprite")
		sprite.texture = pattern.texture
		sprite.scale = Vector2.ONE * float(pattern.scale)
		shooter.get_parent().add_child(bullet)
	# One sound per firing event.
	var sound: AudioStreamPlayer = shooter.get_node(^"Shot")
	sound.pitch_scale = pattern.pitch if pattern.pitch > 0.0 else randf_range(0.94, 1.06)
	sound.play()

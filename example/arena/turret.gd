extends Node2D
## A turret: shoots at the player, hops somewhere nearby, rests, repeats.
## The `shoot` event fires from the gun attachment; `air_start` and
## `air_end` say when the hop leaves and touches the ground.

const Shooting := preload("res://example/arena/shooting.gd")
const HOP_SECONDS := 1.0  # 30 frames at 30 fps.
const AIR_SECONDS := 10.0 / 30.0  # air_start at frame 6, air_end at frame 16.
const SHOOT_SECONDS := 1.0  # 24 frames at 24 fps.

var arena: Node
var target: Node2D

var _timer := 0.0
var _shooting := false
var _hopping := false
var _airborne := false
var _dead := false
var _stopped := false
var _hop_from := Vector2.ZERO
var _hop_to := Vector2.ZERO
var _air_time := 0.0

@onready var sprite: SpriteLoop2D = $Sprite
@onready var gun: SpriteLoopAttachment2D = $Sprite/Gun


func _ready() -> void:
	_timer = randf_range(0.5, 2.0)
	sprite.event_triggered.connect(_on_event)


func _process(delta: float) -> void:
	if _update_end_state():
		return
	if _airborne:
		_air_time += delta
		position = arena.clamp_to_ground(_hop_from.lerp(_hop_to, minf(1.0, _air_time / AIR_SECONDS)))
	_timer -= delta
	if _timer > 0.0:
		return
	if _shooting:
		_shooting = false
		_begin_hop()
	elif _hopping:
		_begin_idle()
	else:
		_begin_shoot()


func _on_event(event_name: StringName, _data: String, _animation_name: StringName,
		_frame: int, _source_frame: int) -> void:
	if _dead or _stopped:
		return
	if event_name == &"shoot" and _shooting:
		# The artwork faces left at positive scale.
		Shooting.fire(self, gun, &"turret", -signf(sprite.scale.x))
	elif event_name == &"air_start" and _hopping:
		_airborne = true
	elif event_name == &"air_end" and _hopping:
		_land()
		var steps: AudioStreamPlayer = $Steps
		steps.volume_db = linear_to_db(0.65)
		steps.pitch_scale = 0.9
		steps.play()


func _begin_shoot() -> void:
	_face(target.position.x - position.x)
	_shooting = true
	_timer = SHOOT_SECONDS
	_play(&"shoot", false)


func _begin_hop() -> void:
	_hop_from = position
	var angle := randf() * TAU
	_hop_to = arena.clamp_to_ground(_hop_from + Vector2.from_angle(angle) * randf_range(90.0, 180.0))
	_face(_hop_to.x - _hop_from.x)
	_air_time = 0.0
	_hopping = true
	_airborne = false
	_timer = HOP_SECONDS
	_play(&"jump", false)


func _begin_idle() -> void:
	_land()
	_hopping = false
	_timer = randf_range(0.8, 2.2)
	_play(&"idle", true)


func _land() -> void:
	_airborne = false
	position = _hop_to


## Dead turrets play "die" once; after the player loses, the rest jump for joy.
func _update_end_state() -> bool:
	if not arena.combat.alive(self):
		if not _dead:
			_dead = true
			_shooting = false
			_hopping = false
			_airborne = false
			_play(&"die", false)
		return true
	if arena.combat.ended:
		if not _stopped:
			_stopped = true
			_play(&"jump", true)
		return true
	return false


func _face(direction_x: float) -> void:
	if direction_x != 0.0:
		sprite.scale.x = absf(sprite.scale.x) * -signf(direction_x)


func _play(animation: StringName, loop: bool) -> void:
	sprite.loop_mode = SpriteLoop2D.LOOP_MODE_LOOP if loop else SpriteLoop2D.LOOP_MODE_ONCE
	sprite.stop()
	sprite.play(animation)

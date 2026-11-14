extends Node2D
## The player. Shows animation selection, `shoot` and `step` events, a part
## attachment (the gun point) and skins as weapons.
##
## Animations: idle/run unarmed, idle_armed/run_armed with a weapon, shoot
## while standing, hit/hit_armed when damaged, die/die_armed.

const Shooting := preload("res://example/arena/shooting.gd")
## Gameplay firing cycle, independent of the exported frame count.
const SHOOT_SECONDS := 0.6
const HIT_SECONDS := 18.0 / 24.0

@export var speed := 300.0

var arena: Node
## Equipped weapon skin: &"red", &"blue", or empty when unarmed.
var weapon := &""

var _animation := &""
var _shooting := false
var _shoot_time := 0.0
var _hit_time := 0.0
var _hit_animation := &""
var _wants_move := false
var _moving := false
var _dead := false
var _mouse_held := false
var _fire_tapped := false

@onready var sprite: SpriteLoop2D = $Sprite
@onready var gun_point: SpriteLoopAttachment2D = $Sprite/GunPoint


func _ready() -> void:
	sprite.event_triggered.connect(_on_event)


func _process(delta: float) -> void:
	if not arena.combat.alive(self):
		_die()
		return
	var direction := Vector2(Input.get_axis(&"arena_left", &"arena_right"),
		Input.get_axis(&"arena_up", &"arena_down"))
	_wants_move = direction != Vector2.ZERO
	_hit_time = maxf(0.0, _hit_time - delta)
	var reacting := _hit_time > 0.0
	# Moving cancels shooting before the next shoot event arrives.
	if _wants_move:
		_shooting = false
	_update_shooting(delta, reacting)
	_moving = _move(direction, delta, _shooting or reacting)
	_update_animation(reacting)


func _unhandled_input(event: InputEvent) -> void:
	var mouse := event as InputEventMouseButton
	if mouse != null and mouse.button_index == MOUSE_BUTTON_LEFT:
		_mouse_held = mouse.pressed
		_fire_tapped = _fire_tapped or mouse.pressed


## Called by the arena when the player walks over a pickup.
func equip(skin: StringName) -> void:
	# A skin swaps the gun parts in every animation at once.
	sprite.skin = skin
	weapon = skin
	$Pickup.play()


## Called by combat when the player takes damage and survives.
func hit() -> void:
	_shooting = false
	_hit_time = HIT_SECONDS
	_hit_animation = &"hit_armed" if weapon else &"hit"
	_play(_hit_animation, false)


func _on_event(event_name: StringName, _data: String, _animation_name: StringName,
		_frame: int, _source_frame: int) -> void:
	if _dead:
		return
	if event_name == &"shoot" and _shooting:
		Shooting.fire(self, gun_point, weapon, signf(sprite.scale.x))
	elif event_name == &"step" and _wants_move and _moving and _hit_time == 0.0:
		_footstep()


func _update_shooting(delta: float, reacting: bool) -> void:
	if _shooting:
		_shoot_time -= delta
		if _shoot_time <= 0.0:
			_shooting = false
	var wants_fire := _fire_tapped or _mouse_held or Input.is_action_pressed(&"arena_fire") \
		or Input.is_action_just_pressed(&"arena_fire")
	if weapon and not reacting and not _shooting and not _wants_move and wants_fire:
		_shooting = true
		_shoot_time = SHOOT_SECONDS
		_play(&"shoot", false)
	_fire_tapped = false


func _move(direction: Vector2, delta: float, blocked: bool) -> bool:
	var previous := position
	if direction != Vector2.ZERO and not blocked:
		position += direction.normalized() * speed * delta
		if direction.x != 0.0:
			sprite.scale.x = absf(sprite.scale.x) * signf(direction.x)
	position = arena.clamp_to_ground(position)
	return position.distance_squared_to(previous) > 0.000001


func _update_animation(reacting: bool) -> void:
	var animation: StringName
	if reacting:
		animation = _hit_animation
	elif _shooting:
		animation = &"shoot"
	elif weapon:
		animation = &"run_armed" if _moving else &"idle_armed"
	else:
		animation = &"run" if _moving else &"idle"
	if animation != _animation:
		_play(animation, true)


func _die() -> void:
	if _dead:
		return
	_dead = true
	_shooting = false
	_play(&"die_armed" if weapon else &"die", false)


## Starts `animation` from its first frame, even when it is already current.
func _play(animation: StringName, loop: bool) -> void:
	sprite.loop_mode = SpriteLoop2D.LOOP_MODE_LOOP if loop else SpriteLoop2D.LOOP_MODE_ONCE
	sprite.stop()
	sprite.play(animation)
	_animation = animation


func _footstep() -> void:
	var steps: AudioStreamPlayer = $Steps
	steps.volume_db = linear_to_db(0.45)
	steps.pitch_scale = 1.0
	steps.play()

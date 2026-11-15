extends Control
## Health bars over the characters, the player's health, controls, round
## result and the mute button. Mute is the audio bus state, so it survives
## restarts.

const Combat := preload("res://example/arena/combat.gd")
const DARK := Color(0.1, 0.14, 0.18)
const MUTED := Color(0.65, 0.25, 0.16)
const CONTROLS := "WASD / Arrows: move    Space / Click: fire    R: restart"
const TOUCH_CONTROLS := "Left side: move    Tap: fire"

@export var arena: Node

@onready var health_label: Label = $Health
@onready var status_label: Label = $Status
@onready var mute_button: Button = $Mute
@onready var touch_controls: Control = $TouchControls


func _ready() -> void:
	mute_button.pressed.connect(_toggle_mute)
	_refresh_mute()
	# The touch buttons replace the instant victory hint.
	$Win.visible = not touch_controls.enabled


func _process(_delta: float) -> void:
	var combat: Combat = arena.combat
	for actor in combat.actors:
		if actor.team == Combat.PLAYER:
			health_label.text = "HEALTH %d / %d" % [actor.hp, actor.max_hp]
	if combat.ended:
		status_label.text = "%s    -    Press R to restart" % combat.result
	else:
		status_label.text = TOUCH_CONTROLS if touch_controls.enabled else CONTROLS
	queue_redraw()


func _draw() -> void:
	var combat: Combat = arena.combat
	if combat.ended:
		return
	for actor in combat.actors:
		if actor.hp <= 0:
			continue
		var top_center := actor.node.position + Vector2(0, -192)
		draw_rect(Rect2(top_center - Vector2(38, 5), Vector2(76, 10)), DARK)
		var fill := Color(0.15, 0.7, 0.4) if actor.team == Combat.PLAYER else Color(0.9, 0.3, 0.2)
		draw_rect(Rect2(top_center - Vector2(36, 3), Vector2(72.0 * actor.hp / actor.max_hp, 6)), fill)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed(&"arena_mute"):
		_toggle_mute()


func _toggle_mute() -> void:
	AudioServer.set_bus_mute(0, not AudioServer.is_bus_mute(0))
	_refresh_mute()


func _refresh_mute() -> void:
	var muted := AudioServer.is_bus_mute(0)
	mute_button.text = "UNMUTE [M]" if muted else "MUTE [M]"
	for state in [&"normal", &"hover", &"pressed"]:
		var style := StyleBoxFlat.new()
		style.bg_color = MUTED if muted else DARK
		if state == &"hover":
			style.bg_color = style.bg_color.lightened(0.1)
		mute_button.add_theme_stylebox_override(state, style)

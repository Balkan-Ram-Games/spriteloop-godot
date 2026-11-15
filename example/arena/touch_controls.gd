extends Control
## On-screen controls for touch screens: a floating joystick on the left side
## of the screen moves, a tap or hold anywhere else fires, and the R and V
## buttons restart and win. They press the same input actions as the keyboard,
## so the game code does not know the difference. Hidden without a touch
## screen.

const JOYSTICK_REST := Vector2(180, 520)
const JOYSTICK_RADIUS := 90.0
const KNOB_RADIUS := 38.0
## Touches that start left of this line drive the joystick.
const JOYSTICK_ZONE_RIGHT := 520.0
## Stick deflection (0-1) below which an axis counts as released.
const DEAD_ZONE := 0.25
const BASE_COLOR := Color(0.1, 0.14, 0.18, 0.18)
const KNOB_COLOR := Color(0.1, 0.14, 0.18, 0.45)

## Shows the controls without a touch screen (tests).
@export var force_enabled := false

## Buttons and the input action each one sends.
@onready var buttons := {$Restart: &"arena_restart", $Win: &"arena_win"}
## Controls that touches pass through to (they handle the emulated mouse).
@onready var passthrough: Array[Control] = [$"../Mute"]

var enabled := false
## Joystick deflection, length 0-1.
var stick := Vector2.ZERO

var _stick_touch := -1
var _stick_center := JOYSTICK_REST
var _fire_touches := {}


func _ready() -> void:
	enabled = force_enabled or DisplayServer.is_touchscreen_available()
	visible = enabled
	set_process_input(enabled)


func _exit_tree() -> void:
	# Restart frees this node mid-touch; leave no action held.
	for action: StringName in [&"arena_left", &"arena_right", &"arena_up", &"arena_down", &"arena_fire"]:
		Input.action_release(action)


func _input(event: InputEvent) -> void:
	var touch := event as InputEventScreenTouch
	if touch != null:
		if touch.pressed:
			touch_down(touch.index, touch.position)
		else:
			touch_up(touch.index)
		return
	var drag := event as InputEventScreenDrag
	if drag != null and drag.index == _stick_touch:
		_move_stick(drag.position)


## A finger went down at `point` (viewport coordinates).
func touch_down(index: int, point: Vector2) -> void:
	for button: Control in buttons:
		if button.get_global_rect().has_point(point):
			var event := InputEventAction.new()
			event.action = buttons[button]
			event.pressed = true
			Input.parse_input_event(event)
			return
	for control in passthrough:
		if control.get_global_rect().has_point(point):
			return
	if point.x < JOYSTICK_ZONE_RIGHT and _stick_touch == -1:
		_stick_touch = index
		# The stick appears under the finger, kept fully on screen.
		_stick_center = point.clamp(Vector2.ONE * JOYSTICK_RADIUS, size - Vector2.ONE * JOYSTICK_RADIUS)
		_move_stick(point)
	else:
		_fire_touches[index] = true
		Input.action_press(&"arena_fire")


func touch_up(index: int) -> void:
	if index == _stick_touch:
		_stick_touch = -1
		_stick_center = JOYSTICK_REST
		_move_stick(_stick_center)
	elif _fire_touches.erase(index) and _fire_touches.is_empty():
		Input.action_release(&"arena_fire")


func _move_stick(point: Vector2) -> void:
	stick = (point - _stick_center).limit_length(JOYSTICK_RADIUS) / JOYSTICK_RADIUS
	_set_axis(&"arena_left", &"arena_right", stick.x)
	_set_axis(&"arena_up", &"arena_down", stick.y)
	queue_redraw()


static func _set_axis(negative: StringName, positive: StringName, value: float) -> void:
	if value <= -DEAD_ZONE:
		Input.action_press(negative, -value)
	else:
		Input.action_release(negative)
	if value >= DEAD_ZONE:
		Input.action_press(positive, value)
	else:
		Input.action_release(positive)


func _draw() -> void:
	draw_circle(_stick_center, JOYSTICK_RADIUS, BASE_COLOR)
	draw_circle(_stick_center + stick * JOYSTICK_RADIUS, KNOB_RADIUS, KNOB_COLOR)

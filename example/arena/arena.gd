extends Node2D
## The arena: combat, pickups, the victory celebration and the keys for
## restart, instant victory and mute.
##
## Character positions are their spot on the ground; the artwork is offset
## above it. The Actors node y-sorts characters and bullets by that spot.

const Combat := preload("res://example/arena/combat.gd")
const VICTORY := preload("res://example/arena/victory.tscn")
## Ground area the characters stay in (artwork extends above it).
const GROUND := Rect2(110, 220, 1060, 400)
const PICKUP_RADIUS := 42.0
const PICKUP_RESPAWN_SECONDS := 6.0
const ACTIONS := {
	&"arena_left": [KEY_A, KEY_LEFT],
	&"arena_right": [KEY_D, KEY_RIGHT],
	&"arena_up": [KEY_W, KEY_UP],
	&"arena_down": [KEY_S, KEY_DOWN],
	&"arena_fire": [KEY_SPACE],
	&"arena_restart": [KEY_R],
	&"arena_win": [KEY_V],
	&"arena_mute": [KEY_M],
}

var combat := Combat.new()

var _victory: Node2D

@onready var player: Node2D = $Actors/Player
@onready var turrets: Array[Node] = get_tree().get_nodes_in_group(&"arena_turrets")
@onready var pickups: Array[Node] = $Pickups.get_children()


func _enter_tree() -> void:
	# The example brings its own input actions, so the project needs no setup.
	for action: StringName in ACTIONS:
		if InputMap.has_action(action):
			continue
		InputMap.add_action(action)
		for key: Key in ACTIONS[action]:
			var event := InputEventKey.new()
			event.physical_keycode = key
			InputMap.action_add_event(action, event)


func _ready() -> void:
	player.arena = self
	combat.register(player, Combat.PLAYER, 5, Vector2(32, 28))
	for turret in turrets:
		turret.arena = self
		turret.target = player
		combat.register(turret, Combat.TURRET, 4, Vector2(32, 28))


# Runs after the characters (process_priority 1), like Defold's late_update.
func _process(delta: float) -> void:
	combat.update(delta)
	if combat.result == Combat.CLEARED and _victory == null:
		_victory = VICTORY.instantiate()
		add_child(_victory)
	_update_pickups(delta)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed(&"arena_restart"):
		get_tree().reload_current_scene()
	elif event.is_action_pressed(&"arena_win"):
		win()


func clamp_to_ground(point: Vector2) -> Vector2:
	return point.clamp(GROUND.position, GROUND.end)


## Defeats every turret through the normal damage path (V key).
func win() -> void:
	if combat.ended or not combat.alive(player):
		return
	for actor in combat.actors:
		if actor.team == Combat.TURRET and actor.hp > 0:
			combat.damage(actor, actor.hp)
	combat.update(0.0)


func _update_pickups(delta: float) -> void:
	for pickup: Sprite2D in pickups:
		var nearby := player.position.distance_to(pickup.position) <= PICKUP_RADIUS
		if pickup.visible and nearby and combat.alive(player) and player.weapon != pickup.weapon:
			player.equip(pickup.weapon)
			pickup.visible = false
			pickup.cooldown = PICKUP_RESPAWN_SECONDS
		elif not pickup.visible:
			pickup.cooldown = maxf(0.0, pickup.cooldown - delta)
			# Wait for the player to step away before it comes back.
			if pickup.cooldown == 0.0 and not nearby:
				pickup.visible = true

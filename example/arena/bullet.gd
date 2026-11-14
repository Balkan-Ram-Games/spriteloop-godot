extends Node2D
## A bullet. The node moves along the ground plane, so it is y-sorted with the
## characters; the sprite floats `height` pixels above it.

const Combat := preload("res://example/arena/combat.gd")
const BOUNDS := Rect2(-80, -80, 1440, 880)

var arena: Node
var team := Combat.TURRET
var velocity := Vector2.ZERO
var height := 0.0
var damage := 1
var lifetime := 4.0

@onready var sprite: Sprite2D = $Sprite


func _ready() -> void:
	sprite.position.y = -height
	sprite.flip_h = team == Combat.PLAYER and velocity.x < 0.0


func _process(delta: float) -> void:
	var combat: Combat = arena.combat
	lifetime -= delta
	var next := position + velocity * delta
	var target: Combat.Actor = null if combat.ended else combat.trace(team, position, next)
	if target != null:
		combat.damage(target, damage)
	var round_over := combat.ended and (combat.result != Combat.CLEARED or team != Combat.PLAYER)
	if target != null or round_over or lifetime <= 0.0 or not BOUNDS.has_point(next + sprite.position):
		queue_free()
		return
	position = next

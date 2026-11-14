extends RefCounted
## Health, damage, hit flash and the round result.

const PLAYER := &"player"
const TURRET := &"turret"
const CLEARED := "ARENA CLEARED"
const DEFEAT := "DEFEAT"
const FLASH_SECONDS := 0.12
## A brief grace period keeps overlapping enemy shots from draining all HP.
const PLAYER_IMMUNITY_SECONDS := 0.65
const HIT_COLOR := Color(1.0, 0.3, 0.3)


class Actor:
	var node: Node2D
	var team: StringName
	var hp: int
	var max_hp: int
	## Hitbox half extents on the ground plane.
	var half_size: Vector2
	var flash := 0.0
	var immunity := 0.0


var actors: Array[Actor] = []
var ended := false
var result := ""


func register(node: Node2D, team: StringName, health: int, half_size: Vector2) -> void:
	var actor := Actor.new()
	actor.node = node
	actor.team = team
	actor.hp = health
	actor.max_hp = health
	actor.half_size = half_size
	actors.append(actor)


func find(node: Node2D) -> Actor:
	for actor in actors:
		if actor.node == node:
			return actor
	return null


func alive(node: Node2D) -> bool:
	var actor := find(node)
	return actor != null and actor.hp > 0


## The first living enemy of `team` hit by a bullet moving from `from` to `to`
## on the ground plane. Sweeping the whole step keeps fast bullets from
## passing through.
func trace(team: StringName, from: Vector2, to: Vector2) -> Actor:
	var nearest: Actor = null
	var contact := INF
	for actor in actors:
		if actor.hp <= 0 or actor.team == team:
			continue
		var t := _sweep(from, to, actor.node.position, actor.half_size + Vector2(3, 3))
		if t >= 0.0 and t < contact:
			nearest = actor
			contact = t
	return nearest


func damage(actor: Actor, amount: int) -> bool:
	if ended or actor.hp <= 0 or actor.immunity > 0.0:
		return false
	actor.hp = maxi(0, actor.hp - amount)
	actor.flash = FLASH_SECONDS
	actor.immunity = PLAYER_IMMUNITY_SECONDS if actor.team == PLAYER else 0.0
	actor.node.sprite.self_modulate = HIT_COLOR
	if actor.team == PLAYER and actor.hp > 0:
		actor.node.hit()
	return true


func update(delta: float) -> void:
	var enemies := 0
	for actor in actors:
		actor.immunity = maxf(0.0, actor.immunity - delta)
		if actor.flash > 0.0:
			actor.flash = maxf(0.0, actor.flash - delta)
			if actor.flash == 0.0:
				actor.node.sprite.self_modulate = Color.WHITE
		if actor.team == TURRET:
			# Dead turrets stay as scenery; only living ones count.
			if actor.hp > 0:
				enemies += 1
		elif actor.hp <= 0:
			ended = true
			result = DEFEAT
	if not ended and enemies == 0:
		ended = true
		result = CLEARED


## Fraction of the segment where it enters the box, or -1 when it misses.
static func _sweep(from: Vector2, to: Vector2, center: Vector2, half_size: Vector2) -> float:
	var enter := 0.0
	var leave := 1.0
	for axis in 2:
		var delta := to[axis] - from[axis]
		var low := center[axis] - half_size[axis]
		var high := center[axis] + half_size[axis]
		if absf(delta) < 0.000001:
			if from[axis] < low or from[axis] > high:
				return -1.0
			continue
		var a := (low - from[axis]) / delta
		var b := (high - from[axis]) / delta
		enter = maxf(enter, minf(a, b))
		leave = minf(leave, maxf(a, b))
		if enter > leave:
			return -1.0
	return enter

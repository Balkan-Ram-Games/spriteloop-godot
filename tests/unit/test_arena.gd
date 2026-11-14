extends "res://tests/test_case.gd"
## Plays the example arena with simulated input. Any script error in the
## example fails the run (the runner counts script errors).

const Combat := preload("res://example/arena/combat.gd")


func _wait(seconds: float) -> void:
	var end := Time.get_ticks_msec() + int(seconds * 1000.0)
	while Time.get_ticks_msec() < end:
		await runner.process_frame


func _arena() -> Node2D:
	var arena: Node2D = load("res://example/arena/arena.tscn").instantiate()
	_owned.append(arena)
	runner.root.add_child(arena)
	return arena


static func _anim(node: Node2D) -> String:
	return String(node.sprite.animation).to_lower()


func _hold(action: StringName, seconds: float) -> void:
	Input.action_press(action)
	await _wait(seconds)
	Input.action_release(action)


func test_arena_plays() -> void:
	seed(7)
	var arena := _arena()
	var player: Node2D = arena.player
	var combat: Combat = arena.combat
	await _wait(0.1)
	check_eq(_anim(player), "idle", "starts unarmed in idle")
	check_eq(combat.actors.size(), 4, "player and three turrets registered")
	# Keep turret shots from interrupting the player during the test.
	combat.find(player).immunity = 1000.0

	var start := player.position
	Input.action_press(&"arena_right")
	Input.action_press(&"arena_up")
	await _wait(0.3)
	check(player.position.x > start.x + 40.0 and player.position.y < start.y - 40.0, "moves up and right")
	check_eq(_anim(player), "run", "run while moving")
	Input.action_release(&"arena_right")
	Input.action_release(&"arena_up")
	await _hold(&"arena_left", 0.05)
	check(player.sprite.scale.x < 0.0, "faces left")
	await _wait(0.1)
	check_eq(_anim(player), "idle", "idle when stopped")

	# Shooting needs a weapon.
	await _hold(&"arena_fire", 0.2)
	check(_anim(player) != "shoot", "no shooting unarmed")

	var red: Sprite2D = arena.get_node("Pickups/RedGun")
	player.position = red.position
	await _wait(0.1)
	check_eq(player.weapon, &"red", "red pickup equips the red skin")
	check(not red.visible, "pickup disappears")
	check_eq(_anim(player), "idle_armed", "armed idle")

	# A turret right in front of the player.
	var turret: Node2D = arena.turrets[0]
	player.position = Vector2(300, 440)
	await _hold(&"arena_right", 0.05)
	turret.position = player.position + Vector2(160, 0)
	var bullets := []
	arena.get_node("Actors").child_entered_tree.connect(func(node: Node):
		if node.get(&"team") == Combat.PLAYER:
			bullets.append(node))
	Input.action_press(&"arena_fire")
	var shoot_seen := false
	var end := Time.get_ticks_msec() + 2000
	while Time.get_ticks_msec() < end and combat.find(turret).hp == 4:
		await runner.process_frame
		shoot_seen = shoot_seen or _anim(player) == "shoot"
	Input.action_release(&"arena_fire")
	check(shoot_seen, "shoot animation while firing")
	check(bullets.size() > 0, "shoot events fire bullets from the gun point")
	check(combat.find(turret).hp < 4, "a bullet hits the turret")

	var blue: Sprite2D = arena.get_node("Pickups/BlueGun")
	player.position = blue.position
	await _wait(0.1)
	check_eq(player.weapon, &"blue", "blue pickup equips the blue skin")
	check_eq(String(player.sprite.skin), "blue", "skin switched")
	await _wait(0.7)
	bullets.clear()
	await _hold(&"arena_fire", 0.3)
	check(bullets.size() >= 5, "blue gun fires a five-bullet spread (%d)" % bullets.size())

	# Red respawns after the cooldown once the player is away.
	red.cooldown = 0.01
	await _wait(0.1)
	check(red.visible, "pickup respawns")

	arena.win()
	await _wait(0.2)
	check_eq(combat.result, Combat.CLEARED, "instant victory")
	check(arena.get_node_or_null("Victory") != null, "victory celebration")
	check_eq(_anim(turret), "die", "turrets die")


func test_player_hit_and_defeat() -> void:
	var arena := _arena()
	var player: Node2D = arena.player
	var combat: Combat = arena.combat
	await _wait(0.05)
	var actor := combat.find(player)
	combat.damage(actor, 1)
	await _wait(0.05)
	check_eq(_anim(player), "hit", "hit reaction")
	check(player.sprite.self_modulate != Color.WHITE, "hit flash")
	await _wait(0.9)
	check_eq(_anim(player), "idle", "recovers after the hit")
	check_eq(player.sprite.self_modulate, Color.WHITE, "flash cleared")

	actor.immunity = 0.0
	combat.damage(actor, actor.hp)
	await _wait(0.1)
	check_eq(combat.result, Combat.DEFEAT, "defeat")
	check_eq(_anim(player), "die", "player dies")
	check_eq(_anim(arena.turrets[0]), "jump", "turrets celebrate")

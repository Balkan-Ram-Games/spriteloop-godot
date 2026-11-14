extends "res://tests/test_case.gd"
## Driving SpriteLoop nodes from an AnimationPlayer timeline (cutscenes):
## value tracks for animation, frame, skin and modulate, plus a method track.
## Clockheart: "idle" 24 frames, "jump" 24 frames, "run" 33 frames.


## Scene: Cutscene (Node) with the character "Robot" and an AnimationPlayer,
## stepped manually so the test controls time.
func make_cutscene(robot: Node, animation: Animation) -> AnimationPlayer:
	var scene := Node.new()
	_owned.append(scene)
	robot.name = "Robot"
	scene.add_child(robot)
	var library := AnimationLibrary.new()
	library.add_animation(&"cutscene", animation)
	var player := AnimationPlayer.new()
	player.callback_mode_process = AnimationMixer.ANIMATION_CALLBACK_MODE_PROCESS_MANUAL
	player.add_animation_library(&"", library)
	scene.add_child(player)
	runner.root.add_child(scene)
	return player


func add_discrete_track(animation: Animation, path: String, keys: Array) -> void:
	var track := animation.add_track(Animation.TYPE_VALUE)
	animation.track_set_path(track, path)
	animation.value_track_set_update_mode(track, Animation.UPDATE_DISCRETE)
	for key in keys:
		animation.track_insert_key(track, key[0], key[1])


func make_timeline() -> Animation:
	var animation := Animation.new()
	animation.length = 1.0
	add_discrete_track(animation, "Robot:animation", [[0.0, &"idle"], [0.5, &"jump"]])
	add_discrete_track(animation, "Robot:frame", [[0.0, 2], [0.25, 5], [0.75, 10]])
	add_discrete_track(animation, "Robot:skin", [[0.0, &""], [0.5, &"red"]])
	var fade := animation.add_track(Animation.TYPE_VALUE)
	animation.track_set_path(fade, "Robot:self_modulate")
	animation.track_insert_key(fade, 0.0, Color(1, 1, 1, 0))
	animation.track_insert_key(fade, 0.5, Color(1, 1, 1, 1))
	var calls := animation.add_track(Animation.TYPE_METHOD)
	animation.track_set_path(calls, "Robot")
	animation.track_insert_key(calls, 0.9, {"method": &"play", "args": [&"run"]})
	return animation


func test_timeline_drives_2d_character() -> void:
	var robot := SpriteLoop2D.new()
	robot.process_callback = SpriteLoop2D.PROCESS_CALLBACK_MANUAL
	robot.data = load(GUN)
	var attachment := SpriteLoopAttachment2D.new()
	attachment.part = &"5_head"
	robot.add_child(attachment)
	var player := make_cutscene(robot, make_timeline())
	var log := record_signals(robot)

	player.play(&"cutscene")
	player.seek(0.3, true)
	check_eq(robot.animation, &"Idle", "animation key")
	check_eq(robot.frame, 5, "frame key")
	check(not robot.is_playing(), "keyed frames do not start playback")
	check_near(robot.self_modulate.a, 0.6, "interpolated fade", 0.001)
	check_eq(attachment.transform, robot.get_part_transform("5_head"), "attachment follows keyed frame")

	player.seek(0.8, true)
	check_eq(robot.animation, &"Jump", "animation switched by the timeline")
	check_eq(robot.frame, 10, "frame key after the switch")
	check_eq(robot.skin, &"red", "skin key")
	check_eq(attachment.transform, robot.get_part_transform("5_head"), "attachment follows")
	check_eq(only(log, "event").size(), 0, "keyed frames never emit animation events")

	# Method tracks are called deferred by default (end of the frame).
	player.advance(0.15)
	await runner.process_frame
	check(robot.is_playing(), "method track started playback")
	check_eq(robot.animation, &"Run", "method track played run")


func test_scrubbing_matches_stock_nodes() -> void:
	# How AnimationPlayer applies discrete keys when jumping around the timeline
	# is Godot's business (it applies the keys it crosses). What matters is that
	# SpriteLoop2D ends up exactly where a built-in node does, so a stock
	# Sprite2D shares the timeline: the same frame keys, and a z_index key at the
	# moment the animation and skin switch.
	var robot := SpriteLoop2D.new()
	robot.process_callback = SpriteLoop2D.PROCESS_CALLBACK_MANUAL
	robot.data = load(GUN)
	var animation := make_timeline()
	# Paths are relative to the AnimationPlayer's root (the cutscene node).
	add_discrete_track(animation, "Stock:frame", [[0.0, 2], [0.25, 5], [0.75, 10]])
	add_discrete_track(animation, "Stock:z_index", [[0.0, 1], [0.5, 7]])
	var player := make_cutscene(robot, animation)
	var stock := Sprite2D.new()
	stock.name = "Stock"
	stock.texture = load("res://addons/spriteloop/icons/spriteloop.svg")
	stock.hframes = 16
	player.get_parent().add_child(stock)
	player.play(&"cutscene")
	for times in [[0.8, 0.1], [0.3, 0.9], [0.95, 0.4], [0.6, 0.0]]:
		player.seek(times[0], true)
		player.seek(times[1], true, true)
		var label := "%.2f -> %.2f" % times
		check_eq(robot.frame, stock.frame, "frame matches a stock node (%s)" % label)
		var switched := stock.z_index == 7
		check_eq(robot.animation == &"Jump", switched, "animation key applied like a stock key (%s)" % label)
		check_eq(robot.skin == &"red", switched, "skin key applied like a stock key (%s)" % label)


func test_timeline_drives_3d_character() -> void:
	var robot := SpriteLoop3D.new()
	robot.process_callback = SpriteLoop3D.PROCESS_CALLBACK_MANUAL
	robot.data = load(GUN)
	var animation := Animation.new()
	animation.length = 1.0
	add_discrete_track(animation, "Robot:animation", [[0.0, &"idle"], [0.5, &"jump"]])
	add_discrete_track(animation, "Robot:frame", [[0.0, 3], [0.6, 12]])
	add_discrete_track(animation, "Robot:flip_h", [[0.0, false], [0.5, true]])
	var player := make_cutscene(robot, animation)
	player.play(&"cutscene")
	player.seek(0.2, true)
	check_eq(robot.animation, &"Idle", "3D animation key")
	check_eq(robot.frame, 3, "3D frame key")
	player.seek(0.7, true)
	check_eq(robot.animation, &"Jump", "3D animation switched")
	check_eq(robot.frame, 12, "3D frame key after the switch")
	check(robot.flip_h, "3D flip key")

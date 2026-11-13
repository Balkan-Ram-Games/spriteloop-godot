extends "res://tests/test_case.gd"
## Playback state: play/pause/stop/seek, loop modes, speed and finish.
## Clockheart "run" is 33 frames at 60 FPS and loops; "jump" is 24 frames at
## 24 FPS and does not.


func test_initial_state_shows_first_animation() -> void:
	var node := make_2d()
	check_eq(node.animation, &"Run", "first animation selected")
	check_eq(node.frame, 0, "frame 0")
	check(not node.is_playing(), "not playing until play()")
	check_eq(node.get_frame_count(), 33, "frame count")


func test_animation_property_before_data() -> void:
	var node := SpriteLoop2D.new()
	node.animation = &"jump"
	node.frame = 5
	node.data = load(GUN)
	check_eq(node.animation, &"Jump", "animation resolved by ID to display name")
	check_eq(node.frame, 5, "pending frame applied")
	node.free()


func test_play_and_advance() -> void:
	var node := make_2d()
	check(node.play(&"run"), "play by ID")
	check(node.is_playing(), "playing")
	node.advance(10.5 / 60.0)
	check_eq(node.frame, 10, "frame after advance")
	check_near(node.get_playback_position(), 10.5 / 60.0, "playback position", 0.001)


func test_unknown_animation() -> void:
	var node := make_2d()
	check(not node.play(&"does_not_exist"), "unknown animation returns false")
	check_eq(node.animation, &"Run", "animation unchanged")


func test_play_same_animation_continues() -> void:
	var node := make_2d()
	node.play(&"run")
	node.advance(5.5 / 60.0)
	node.play(&"Run")
	check_eq(node.frame, 5, "play() of the current animation does not restart")


func test_pause_resume_stop() -> void:
	var node := make_2d()
	node.play(&"run")
	node.advance(6.5 / 60.0)
	node.pause()
	check(not node.is_playing(), "paused")
	node.advance(1.0)
	check_eq(node.frame, 6, "paused frame holds")
	node.play()
	check(node.is_playing(), "resumed")
	node.advance(1.0 / 60.0)
	check_eq(node.frame, 7, "resumed from the paused frame")
	node.stop()
	check(not node.is_playing(), "stopped")
	check_eq(node.frame, 0, "stop rewinds")


func test_non_looping_finishes_on_last_frame() -> void:
	var node := make_2d()
	var log := record_signals(node)
	node.play(&"jump")
	node.advance(0.5)
	check(node.is_playing(), "still playing mid-animation")
	node.advance(0.6)
	check(not node.is_playing(), "stops at the end")
	check_eq(node.frame, 23, "holds the last frame")
	check_eq(only(log, "finished").size(), 1, "animation_finished once")
	node.advance(1.0)
	check_eq(only(log, "finished").size(), 1, "no repeat after finishing")
	node.play()
	check(node.is_playing(), "play() after finishing restarts")
	check_eq(node.frame, 0, "restart from frame 0")


func test_loop_signal() -> void:
	var node := make_2d()
	var log := record_signals(node)
	node.play(&"run")
	node.advance(32.5 / 60.0)
	check_eq(only(log, "looped").size(), 0, "no loop before the end")
	node.advance(1.0 / 60.0)
	check_eq(only(log, "looped").size(), 1, "animation_looped at the wrap")
	check_eq(node.frame, 0, "wrapped to frame 0")


func test_loop_mode_overrides() -> void:
	var node := make_2d()
	node.loop_mode = SpriteLoop2D.LOOP_MODE_ONCE
	node.play(&"run")
	node.advance(2.0)
	check(not node.is_playing(), "LOOP_MODE_ONCE stops a looping animation")
	check_eq(node.frame, 32, "holds last frame")

	node.loop_mode = SpriteLoop2D.LOOP_MODE_LOOP
	node.play(&"jump")
	node.advance(25.5 / 24.0)
	check(node.is_playing(), "LOOP_MODE_LOOP loops a one-shot animation")
	check_eq(node.frame, 1, "wrapped")


func test_speed_scale() -> void:
	var node := make_2d()
	node.speed_scale = 2.0
	node.play(&"run")
	node.advance(5.5 / 60.0)
	check_eq(node.frame, 11, "double speed")
	node.speed_scale = 0.0
	node.advance(1.0)
	check_eq(node.frame, 11, "zero speed holds")
	node.speed_scale = -1.0
	check_eq(node.speed_scale, 0.0, "negative speed clamps to zero")


func test_seek() -> void:
	var node := make_2d()
	node.play(&"run")
	check(node.seek(20.5 / 60.0), "seek")
	check_eq(node.frame, 20, "seek by time")
	check(node.seek_frame(3), "seek_frame")
	check_eq(node.frame, 3, "seek by frame")
	node.seek_frame(500)
	check_eq(node.frame, 32, "seek_frame clamps")
	node.frame = 12
	check_eq(node.frame, 12, "frame property")


func test_animation_property_switches() -> void:
	var node := make_2d()
	node.animation = &"idle"
	check_eq(node.animation, &"Idle", "selected")
	check(not node.is_playing(), "selecting does not start playback")
	node.play()
	node.animation = &"jump"
	check(node.is_playing(), "changing animation while playing keeps playing")
	check_eq(node.animation, &"Jump", "switched")
	check_eq(node.frame, 0, "new animation starts at 0")


func test_play_before_data_starts_when_assigned() -> void:
	var node := SpriteLoop2D.new()
	node.process_callback = SpriteLoop2D.PROCESS_CALLBACK_MANUAL
	node.play(&"jump")
	node.data = load(GUN)
	check(node.is_playing(), "deferred play starts once data is set")
	check_eq(node.animation, &"Jump", "requested animation")
	node.free()


func test_autoplay_and_process() -> void:
	var node := SpriteLoop2D.new()
	node.data = load(GUN)
	node.autoplay = &"run"
	runner.root.add_child(node)
	_owned.append(node)
	check(node.is_playing(), "autoplay on ready")
	var before := node.get_playback_position()
	await runner.process_frame
	await runner.process_frame
	check(node.get_playback_position() > before, "advances with idle processing")
	node.process_callback = SpriteLoop2D.PROCESS_CALLBACK_MANUAL
	var held := node.get_playback_position()
	await runner.process_frame
	check_eq(node.get_playback_position(), held, "manual callback does not advance")

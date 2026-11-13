extends "res://tests/test_case.gd"
## Event semantics, matching the Defold extension:
## - starting an animation emits frame 0 events,
## - playback emits events of every crossed frame,
## - seeking emits only the destination frame, unless disabled,
## - events are delivered by signals on the next advance().
## Clockheart "run": "step" on frames 0 and 16. "shoot": frames 5 and 8.


func test_start_emits_frame_zero_on_next_advance() -> void:
	var node := make_2d()
	var log := record_signals(node)
	node.play(&"run")
	check_eq(only(log, "event").size(), 0, "nothing emitted inside play()")
	node.advance(0.0)
	var events := only(log, "event")
	check_eq(events.size(), 1, "one frame 0 event")
	if events.size() == 1:
		check_eq(events[0].name, &"step", "event name")
		check_eq(events[0].data, "That's one small step for a man...", "event data")
		check_eq(events[0].animation, &"Run", "animation display name")
		check_eq(events[0].frame, 0, "frame")
		check_eq(events[0].source_frame, 0, "source frame")


func test_crossed_frames_emit_in_order() -> void:
	var node := make_2d()
	var log := record_signals(node)
	node.play(&"shoot")
	node.advance(0.0)
	node.advance(9.5 / 24.0)
	var events := only(log, "event")
	check_eq(events.size(), 2, "both shoot events in one large step")
	if events.size() == 2:
		check_eq([events[0].frame, events[1].frame], [5, 8], "crossed frames in order")


func test_loop_emits_frame_zero_again() -> void:
	var node := make_2d()
	var log := record_signals(node)
	node.play(&"run")
	node.advance(16.5 / 60.0)
	node.advance(17.0 / 60.0)
	var frames := only(log, "event").map(func(entry): return entry.frame)
	check_eq(frames, [0, 16, 0], "events across a loop")
	check_eq(only(log, "looped").size(), 1, "looped")


func test_events_precede_frame_and_loop_signals() -> void:
	var node := make_2d()
	node.play(&"run")
	node.advance(0.0)
	var log := record_signals(node)
	# Frame 0 -> raw frame 34 (frame 1): crosses frame 16 and the wrap to 0.
	node.advance(34.5 / 60.0)
	var order := log.map(func(entry): return entry.signal)
	check_eq(order, ["event", "event", "frame_changed", "looped"], "signal order")


func test_seek_emits_destination_only() -> void:
	var node := make_2d()
	node.play(&"run")
	node.advance(0.0)
	var log := record_signals(node)
	node.seek_frame(16)
	node.advance(0.0)
	check_eq(only(log, "event").map(func(entry): return entry.frame), [16], "destination event")
	log.clear()
	node.seek_frame(16)
	node.advance(0.0)
	check_eq(only(log, "event").size(), 1, "re-selecting the current frame repeats its events")
	log.clear()
	node.seek_frame(0, false)
	node.advance(0.0)
	check_eq(only(log, "event").size(), 0, "seek with emit_events = false")
	node.frame = 16
	node.advance(0.0)
	check_eq(only(log, "event").size(), 0, "frame property never emits")


func test_emit_events_property() -> void:
	var node := make_2d()
	var log := record_signals(node)
	node.emit_events = false
	node.play(&"run")
	node.advance(20.0 / 60.0)
	check_eq(only(log, "event").size(), 0, "events disabled")
	check(only(log, "frame_changed").size() > 0, "other signals still emitted")


func test_play_inside_handler_stops_stale_signals() -> void:
	var node := make_2d()
	var finished := [0]
	node.play(&"jump")
	node.advance(0.0)
	node.event_triggered.connect(func(event_name, _data, _animation, _frame, _source):
		if event_name == &"air_start":
			node.play(&"idle"))
	node.animation_finished.connect(func(): finished[0] += 1)
	node.advance(2.0)
	check_eq(node.animation, &"Idle", "handler switched animation")
	check(node.is_playing(), "new animation playing")
	check_eq(finished[0], 0, "no stale animation_finished for the replaced animation")


func test_queued_events_survive_play() -> void:
	var node := make_2d()
	var log := record_signals(node)
	node.play(&"run")
	node.play(&"shoot")
	node.advance(0.0)
	var animations := only(log, "event").map(func(entry): return entry.animation)
	check_eq(animations, [&"Run"], "run frame 0 event is still delivered; shoot has no frame 0 event")

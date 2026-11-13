extends SceneTree
## Quick crowd probe (not the benchmark suite): spawns N playing characters of
## one package and reports draw calls and average frame time with vsync off.
##
##   godot --path . -s res://tests/render/crowd_probe.gd -- <package> <count>[,<count>...] [2d|3d]
##
## Stops immediately on the first engine error or when a single frame takes
## longer than FRAME_BUDGET_MS, so a broken or overloaded run never keeps the
## desktop busy.

const WARMUP_FRAMES := 60
const MEASURE_FRAMES := 240
const FRAME_BUDGET_MS := 250.0
## First frames after spawning compile shaders and build meshes.
const GRACE_FRAMES := 5

var guard


func _initialize() -> void:
	guard = load("res://tests/render/error_guard.gd").install()
	_run.call_deferred()


func _run() -> void:
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	Engine.max_fps = 0
	var args := OS.get_cmdline_user_args()
	var data: SpriteLoopData = load(args[0])
	var use_3d := args.size() > 2 and args[2] == "3d"
	var animation: StringName = data.get_animation_names()[0]
	if use_3d:
		var camera := Camera3D.new()
		camera.position = Vector3(0, 0, 60)
		root.add_child(camera)
	else:
		var camera := Camera2D.new()
		camera.zoom = Vector2(0.25, 0.25)
		root.add_child(camera)
	for count_text in args[1].split(","):
		var count := int(count_text)
		var crowd: Node = Node3D.new() if use_3d else Node2D.new()
		root.add_child(crowd)
		var rng := RandomNumberGenerator.new()
		rng.seed = 7
		for i in count:
			var node: Node = SpriteLoop3D.new() if use_3d else SpriteLoop2D.new()
			node.data = data
			if use_3d:
				node.position = Vector3(rng.randf_range(-40, 40), rng.randf_range(-22, 22), 0)
			else:
				node.position = Vector2(rng.randf_range(-2400, 2400), rng.randf_range(-1300, 1300))
				node.scale = Vector2(0.25, 0.25)
			crowd.add_child(node)
			node.play(animation)
			node.seek(rng.randf() * 2.0, false)
		if not await _frames(WARMUP_FRAMES, count, GRACE_FRAMES):
			return
		var draw_calls := 0
		var start := Time.get_ticks_usec()
		for i in MEASURE_FRAMES:
			if not await _frames(1, count):
				return
			draw_calls = max(draw_calls, int(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)))
		var frame_ms := float(Time.get_ticks_usec() - start) / MEASURE_FRAMES / 1000.0
		print("%s %s count=%d frame=%.2fms fps=%.0f draw_calls=%d" % [
			"3d" if use_3d else "2d", RenderingServer.get_current_rendering_method(), count,
			frame_ms, 1000.0 / frame_ms, draw_calls])
		crowd.free()
		await process_frame
	quit(0)


## Waits for frames; aborts the probe on an engine error or an over-budget frame.
func _frames(frames: int, count: int, grace := 0) -> bool:
	for i in frames:
		var start := Time.get_ticks_usec()
		await process_frame
		var frame_ms := float(Time.get_ticks_usec() - start) / 1000.0
		if guard.error_count > 0:
			printerr("ABORT count=%d: engine error: %s" % [count, guard.first_error])
			quit(1)
			return false
		if i >= grace and frame_ms > FRAME_BUDGET_MS:
			printerr("ABORT count=%d: frame took %.0f ms (budget %.0f ms)" % [count, frame_ms, FRAME_BUDGET_MS])
			quit(1)
			return false
	return true

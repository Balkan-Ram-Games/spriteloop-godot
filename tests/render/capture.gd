extends SceneTree
## Renders a scene for a few frames and saves the viewport as PNG.
##
##   godot --path . -s res://tests/render/capture.gd -- <scene> <output.png> [frames]
##
## Needs a real renderer (not --headless). Use --rendering-method to pick
## forward_plus, mobile or gl_compatibility.


var guard


func _initialize() -> void:
	guard = load("res://tests/render/error_guard.gd").install()
	_capture.call_deferred()


func _capture() -> void:
	var args := OS.get_cmdline_user_args()
	if args.size() < 2:
		printerr("usage: -- <scene> <output.png> [frames]")
		quit(2)
		return
	var frames := int(args[2]) if args.size() > 2 else 30
	var started := Time.get_ticks_usec()
	root.add_child(load(args[0]).instantiate())
	print("scene added in %.1f ms" % ((Time.get_ticks_usec() - started) / 1000.0))
	for i in frames:
		var start := Time.get_ticks_usec()
		await process_frame
		var frame_ms := (Time.get_ticks_usec() - start) / 1000.0
		if frame_ms > 50.0:
			print("slow frame %d: %.1f ms" % [i, frame_ms])
	await RenderingServer.frame_post_draw
	var image := root.get_texture().get_image()
	if guard.error_count > 0:
		printerr("engine errors during capture (%d), first: %s" % [guard.error_count, guard.first_error])
	var error := image.save_png(args[1])
	print("saved %s (%dx%d, %s)" % [args[1], image.get_width(), image.get_height(),
		RenderingServer.get_current_rendering_method()])
	quit(0 if error == OK and guard.error_count == 0 else 1)

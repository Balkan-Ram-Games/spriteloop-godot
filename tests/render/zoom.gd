extends SceneTree
## Crops a PNG region and enlarges it with nearest filtering, for inspecting
## fine detail in captures.
##   godot --headless --path . -s res://tests/render/zoom.gd -- <in.png> <out.png> <x> <y> <w> <h> <scale>

func _initialize() -> void:
	var args := OS.get_cmdline_user_args()
	var image := Image.load_from_file(args[0])
	var region := image.get_region(Rect2i(int(args[2]), int(args[3]), int(args[4]), int(args[5])))
	region.resize(region.get_width() * int(args[6]), region.get_height() * int(args[6]), Image.INTERPOLATE_NEAREST)
	region.save_png(args[1])
	quit()

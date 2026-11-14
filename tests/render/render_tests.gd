extends SceneTree
## Image-compared render tests (needs a GPU; not --headless).
##
##   godot --path . --resolution 1280x720 --rendering-method forward_plus \
##       -s res://tests/render/render_tests.gd [-- [--update] [case ...]]
##
## Renders each scene with time frozen (Engine.time_scale = 0), so animations
## and effects stay on their initial frame, and compares it with the approved
## reference in tests/render/reference/<rendering method>/<case>.png. A missing
## reference is created and reported as NEW (review it before committing);
## --update replaces references after an intentional visual change.
## Failures write the actual image and a difference map to tests/output/render/.
## Exits with code 1 on any failure or engine error.

const CASES := {
	"arena": "res://example/arena/arena.tscn",
	"example_2d": "res://example/example.tscn",
	"example_3d": "res://example/example_3d.tscn",
	"alpha_cut_3d": "res://tests/render/scenes/alpha_cut_3d.tscn",
	"material_variants_3d": "res://tests/render/scenes/material_variants_3d.tscn",
	"single_sided_3d": "res://tests/render/scenes/single_sided_3d.tscn",
	"attachments": "res://tests/render/scenes/attachments.tscn",
	"attachments_billboard": "res://tests/render/scenes/attachments_billboard.tscn",
	"mipmaps": "res://tests/render/scenes/mipmaps.tscn",
}
const SIZE := Vector2i(1280, 720)
## Largest per-channel difference (of 255) that counts as equal; absorbs
## rounding differences between runs and driver updates.
const CHANNEL_TOLERANCE := 8
## Fraction of pixels allowed to exceed the tolerance.
const MAX_DIFFERENT_FRACTION := 0.0005
## Frames before capturing: shader compilation, first-frame setup.
const SETTLE_FRAMES := 10

var guard


func _initialize() -> void:
	guard = load("res://tests/render/error_guard.gd").install()
	_run.call_deferred()


func _run() -> void:
	var args := OS.get_cmdline_user_args()
	var update := args.has("--update")
	var selected: Array[String] = []
	for arg in args:
		if not arg.begins_with("--"):
			selected.append(arg)
	var method := RenderingServer.get_current_rendering_method()
	var reference_dir := "res://tests/render/reference/%s" % method
	var output_dir := "res://tests/output/render/%s" % method
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(reference_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(output_dir))
	if Vector2i(root.get_texture().get_size()) != SIZE:
		printerr("Run with --resolution %dx%d (got %s)." % [SIZE.x, SIZE.y, root.get_texture().get_size()])
		quit(2)
		return

	Engine.time_scale = 0.0
	var failures: Array[String] = []
	var created: Array[String] = []
	for case_name: String in CASES:
		if not selected.is_empty() and not selected.has(case_name):
			continue
		var errors_before: int = guard.error_count
		var scene: Node = load(CASES[case_name]).instantiate()
		root.add_child(scene)
		for i in SETTLE_FRAMES:
			await process_frame
		await RenderingServer.frame_post_draw
		var image := root.get_texture().get_image()
		image.convert(Image.FORMAT_RGBA8)
		scene.free()
		await process_frame

		if guard.error_count != errors_before:
			failures.append("%s: engine error: %s" % [case_name, guard.first_error])
			continue
		var reference_path := "%s/%s.png" % [reference_dir, case_name]
		if update or not FileAccess.file_exists(reference_path):
			image.save_png(reference_path)
			created.append(case_name)
			print("%-24s %s" % [case_name, "UPDATED" if update else "NEW (review before committing)"])
			continue
		var reference := Image.load_from_file(ProjectSettings.globalize_path(reference_path))
		reference.convert(Image.FORMAT_RGBA8)
		var result := _compare(image, reference)
		if result.fraction > MAX_DIFFERENT_FRACTION:
			failures.append("%s: %.3f%% of pixels differ (max channel difference %d)" % [
				case_name, result.fraction * 100.0, result.max_difference])
			image.save_png("%s/%s.actual.png" % [output_dir, case_name])
			result.map.save_png("%s/%s.diff.png" % [output_dir, case_name])
		print("%-24s %s (%.3f%% different, max %d)" % [case_name,
			"FAIL" if result.fraction > MAX_DIFFERENT_FRACTION else "ok", result.fraction * 100.0,
			result.max_difference])

	Engine.time_scale = 1.0
	print("%s: %d failed, %d new/updated" % [method, failures.size(), created.size()])
	for failure in failures:
		printerr("FAIL ", failure)
	quit(1 if failures.size() > 0 else 0)


## Per-pixel comparison. Returns the fraction of pixels whose largest channel
## difference exceeds CHANNEL_TOLERANCE, the largest difference, and a
## grayscale map of the differences (amplified 4x).
func _compare(actual: Image, reference: Image) -> Dictionary:
	if actual.get_size() != reference.get_size():
		var blank := Image.create(actual.get_width(), actual.get_height(), false, Image.FORMAT_L8)
		return {"fraction": 1.0, "max_difference": 255, "map": blank}
	var a := actual.get_data()
	var b := reference.get_data()
	var pixel_count := actual.get_width() * actual.get_height()
	var map := PackedByteArray()
	map.resize(pixel_count)
	var different := 0
	var largest := 0
	for pixel in pixel_count:
		var offset := pixel * 4
		var difference := maxi(maxi(absi(a[offset] - b[offset]), absi(a[offset + 1] - b[offset + 1])),
			maxi(absi(a[offset + 2] - b[offset + 2]), absi(a[offset + 3] - b[offset + 3])))
		map[pixel] = mini(difference * 4, 255)
		largest = maxi(largest, difference)
		if difference > CHANNEL_TOLERANCE:
			different += 1
	return {
		"fraction": float(different) / pixel_count,
		"max_difference": largest,
		"map": Image.create_from_data(actual.get_width(), actual.get_height(), false, Image.FORMAT_L8, map),
	}

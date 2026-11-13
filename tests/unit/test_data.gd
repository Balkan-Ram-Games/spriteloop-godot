extends "res://tests/test_case.gd"


func test_import_produces_spriteloop_data() -> void:
	var data = load(GUN)
	check(data is SpriteLoopData, "imported resource type")
	check(data.is_valid(), "package parses")
	check_eq(data.get_load_error(), "", "no load error")
	check_eq(data.get_canvas_size(), Vector2i(495, 512), "canvas size")


func test_animation_info() -> void:
	var data: SpriteLoopData = load(GUN)
	var names := data.get_animation_names()
	check_eq(names.size(), 12, "animation count")
	check_eq(names[0], "Run", "first animation display name")
	check(data.has_animation(&"run"), "lookup by ID")
	check(data.has_animation(&"Run"), "lookup by name")
	check(not data.has_animation(&"missing"), "unknown animation")
	check_eq(data.get_animation_frame_count(&"run"), 33, "frame count")
	check_near(data.get_animation_fps(&"run"), 60.0, "fps")
	check_near(data.get_animation_length(&"run"), 33.0 / 60.0, "length")
	check(data.is_animation_looping(&"run"), "run loops")
	check(not data.is_animation_looping(&"jump"), "jump does not loop")


func test_skin_part_variant_info() -> void:
	var data: SpriteLoopData = load(GUN)
	check_eq(data.get_skin_names(), PackedStringArray(["Default", "red", "blue"]), "skins")
	check_eq(data.get_part_names().size(), 18, "part count")
	check(data.get_variant_names(&"gun0").has("gun1"), "gun0 variants include gun1")


func test_invalid_package_is_rejected() -> void:
	# A failed import leaves an .import file but no loadable resource.
	check(ResourceLoader.load(INVALID, "", ResourceLoader.CACHE_MODE_IGNORE) == null,
		"invalid package has no imported resource")
	var data := SpriteLoopData.new()
	data.package_bytes = FileAccess.get_file_as_bytes(INVALID)
	check(not data.is_valid(), "invalid bytes do not parse")
	check(data.get_load_error().contains("manifest.json"), "error names the missing manifest")


func test_load_from_file() -> void:
	var data := SpriteLoopData.load_from_file(TURRET)
	check(data != null and data.is_valid(), "runtime load of a .spla file")
	check(data.has_animation(&"shoot"), "runtime-loaded animations")

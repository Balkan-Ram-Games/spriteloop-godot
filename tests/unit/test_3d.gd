extends "res://tests/test_case.gd"
## SpriteLoop3D: same playback core as 2D, plus 3D geometry, flips and AABB.


func make_3d(path := GUN, in_tree := false) -> SpriteLoop3D:
	var node := SpriteLoop3D.new()
	node.process_callback = SpriteLoop3D.PROCESS_CALLBACK_MANUAL
	node.data = load(path)
	_owned.append(node)
	if in_tree:
		runner.root.add_child(node)
	return node


func test_shared_playback_api() -> void:
	var node := make_3d()
	var log := record_signals(node)
	check_eq(node.animation, &"Run", "first animation selected")
	check(node.play(&"jump"), "play")
	node.advance(0.0)
	node.advance(2.0)
	check(not node.is_playing(), "finished")
	check_eq(node.frame, 23, "last frame")
	check_eq(only(log, "finished").size(), 1, "animation_finished")
	check_eq(only(log, "event").map(func(entry): return entry.name), [&"air_start"], "events")


func test_part_transform_matches_2d() -> void:
	var node_2d := make_2d(ROBOT_SKINS)
	var node_3d := make_3d(ROBOT_SKINS)
	node_3d.pixel_size = 0.01
	var flat := node_2d.get_part_transform("arm_back", SpriteLoop2D.PART_ORIGIN_CENTER)
	var spatial := node_3d.get_part_transform("arm_back", SpriteLoop3D.PART_ORIGIN_CENTER)
	check_near(spatial.origin.x, flat.origin.x * 0.01, "x in world units", 0.00001)
	check_near(spatial.origin.y, -flat.origin.y * 0.01, "y up", 0.00001)
	check_near(spatial.origin.z, 0.0, "in the XY plane", 0.00001)
	check_near(spatial.basis.get_euler().z, -flat.get_rotation(), "rotation mirrors the Y flip", 0.0001)


func test_flip_mirrors_parts_and_bounds() -> void:
	var node := make_3d(ROBOT_SKINS)
	var normal := node.get_part_transform("arm_back")
	var aabb := node.get_aabb()
	node.flip_h = true
	var flipped := node.get_part_transform("arm_back")
	check_near(flipped.origin.x, -normal.origin.x, "flip_h mirrors X", 0.00001)
	check_near(flipped.origin.y, normal.origin.y, "flip_h keeps Y", 0.00001)
	check_near(node.get_aabb().position.x, -aabb.end.x, "flip_h mirrors bounds", 0.00001)


func test_aabb_follows_pixel_size_and_billboard() -> void:
	var node := make_3d()
	var rect := make_2d().get_rect()
	node.pixel_size = 0.02
	var aabb := node.get_aabb()
	check_near(aabb.size.x, rect.size.x * 0.02, "width", 0.0001)
	check_near(aabb.size.y, rect.size.y * 0.02, "height", 0.0001)
	# Billboard: the tightest cube covering every orientation reaches as far as
	# the farthest bounds corner from the node origin.
	var reach := 0.0
	for corner in [aabb.position, aabb.end, Vector3(aabb.position.x, aabb.end.y, 0), Vector3(aabb.end.x, aabb.position.y, 0)]:
		reach = maxf(reach, Vector2(corner.x, corner.y).length())
	node.billboard = SpriteLoop3D.BILLBOARD_ENABLED
	var cube := node.get_aabb()
	check_near(cube.size.x, reach * 2.0, "billboard cube width", 0.0001)
	check_near(cube.size.z, reach * 2.0, "billboard cube depth", 0.0001)
	check_near(cube.position.y, -reach, "billboard cube is centered on the origin", 0.0001)
	# Y-billboard: keeps the real height, widened on X and Z only.
	node.billboard = SpriteLoop3D.BILLBOARD_FIXED_Y
	var y_box := node.get_aabb()
	var horizontal := maxf(absf(aabb.position.x), absf(aabb.end.x))
	check_near(y_box.position.y, aabb.position.y, "Y-billboard keeps the bottom", 0.0001)
	check_near(y_box.size.y, aabb.size.y, "Y-billboard keeps the height", 0.0001)
	check_near(y_box.size.x, horizontal * 2.0, "Y-billboard width", 0.0001)
	check_near(y_box.size.z, horizontal * 2.0, "Y-billboard depth", 0.0001)
	check(y_box.size.y < cube.size.y, "Y-billboard box is lower than the full billboard cube")


func test_meshes_cached_per_pixel_size() -> void:
	var a := make_3d(TURRET, true)
	var meshes: int = SpriteLoopServer.get_stats().meshes
	var b := make_3d(TURRET, true)
	check_eq(SpriteLoopServer.get_stats().meshes, meshes, "same pixel size shares the frame mesh")
	b.pixel_size = 0.05
	check_eq(SpriteLoopServer.get_stats().meshes, meshes + 1, "another pixel size builds its own")
	check(a.get_aabb().size.x < b.get_aabb().size.x, "larger pixel size, larger bounds")


func test_properties_round_trip() -> void:
	var node := make_3d()
	node.modulate = Color(1, 0, 0, 0.5)
	node.billboard = SpriteLoop3D.BILLBOARD_FIXED_Y
	node.shaded = true
	node.double_sided = false
	node.alpha_cut = SpriteLoop3D.ALPHA_CUT_DISCARD
	node.alpha_scissor_threshold = 2.0
	node.texture_filter = SpriteLoop3D.TEXTURE_FILTER_NEAREST
	node.render_priority = 500
	check_eq(node.modulate, Color(1, 0, 0, 0.5), "modulate")
	check_eq(node.billboard, SpriteLoop3D.BILLBOARD_FIXED_Y, "billboard")
	check(node.shaded and not node.double_sided, "flags")
	check_eq(node.alpha_cut, SpriteLoop3D.ALPHA_CUT_DISCARD, "alpha cut")
	check_eq(node.alpha_scissor_threshold, 1.0, "threshold clamps")
	check_eq(node.texture_filter, SpriteLoop3D.TEXTURE_FILTER_NEAREST, "filter")
	check_eq(node.render_priority, 127, "render priority clamps")


func test_3d_released() -> void:
	var before: Dictionary = SpriteLoopServer.get_stats()
	var data: SpriteLoopData = ResourceLoader.load(TURRET, "", ResourceLoader.CACHE_MODE_IGNORE)
	var nodes: Array[SpriteLoop3D] = []
	for i in 3:
		var node := SpriteLoop3D.new()
		node.data = data
		node.skin = &"Default"
		node.billboard = i
		runner.root.add_child(node)
		node.seek_frame(i, false)
		nodes.append(node)
	await runner.process_frame
	for node in nodes:
		node.free()
	nodes.clear()
	data = null
	var after: Dictionary = SpriteLoopServer.get_stats()
	for key in ["packages", "atlas_bytes", "baked_sets", "meshes", "nodes"]:
		check_eq(after[key], before[key], "%s released" % key)

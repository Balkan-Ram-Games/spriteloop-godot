extends "res://tests/test_case.gd"
## Resource lifetime: packages, baked sets and meshes are released when the
## last node and resource reference go away, and reimport restores state.


func test_everything_released() -> void:
	var before: Dictionary = SpriteLoopServer.get_stats()
	var data: SpriteLoopData = ResourceLoader.load(TURRET, "", ResourceLoader.CACHE_MODE_IGNORE)
	var nodes: Array[SpriteLoop2D] = []
	for i in 4:
		var node := SpriteLoop2D.new()
		node.data = data
		node.skin = &"Default"
		runner.root.add_child(node)
		node.seek_frame(i, false)
		nodes.append(node)
	await runner.process_frame
	var during: Dictionary = SpriteLoopServer.get_stats()
	check(during.packages > before.packages, "package loaded")
	check(during.meshes >= before.meshes + 4, "frame meshes built")
	for node in nodes:
		node.free()
	nodes.clear()
	data = null
	var after: Dictionary = SpriteLoopServer.get_stats()
	for key in ["packages", "atlas_bytes", "baked_sets", "meshes", "mesh_bytes", "nodes"]:
		check_eq(after[key], before[key], "%s released" % key)


func test_reload_restores_state() -> void:
	var data: SpriteLoopData = ResourceLoader.load(GUN, "", ResourceLoader.CACHE_MODE_IGNORE)
	var node := SpriteLoop2D.new()
	node.process_callback = SpriteLoop2D.PROCESS_CALLBACK_MANUAL
	node.data = data
	_owned.append(node)
	node.skin = &"red"
	node.set_variant("gun0", "gun2")
	node.play(&"run")
	node.advance(10.5 / 60.0)
	var log := record_signals(node)
	# Simulates a reimport: same resource, new bytes.
	data.package_bytes = data.package_bytes.duplicate()
	check_eq(node.animation, &"Run", "animation kept")
	check_eq(node.frame, 10, "frame kept")
	check(node.is_playing(), "still playing")
	check_eq(node.skin, &"red", "skin kept")
	node.advance(0.0)
	check_eq(only(log, "event").size(), 0, "reload does not emit events")

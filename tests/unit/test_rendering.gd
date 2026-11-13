extends "res://tests/test_case.gd"
## Draw-path behaviour that is observable without a GPU: frame meshes are
## shared, commands are re-recorded only when the displayed frame changes, and
## culling bounds cover the character.


func test_rect_covers_canvas_content() -> void:
	var node := make_2d()
	var rect := node.get_rect()
	check(rect.size.x > 0.0 and rect.size.y > 0.0, "non-empty bounds")
	check(rect.has_point(Vector2.ZERO), "bounds contain the node origin")


func test_redraw_only_when_frame_changes() -> void:
	var node := make_2d(GUN, true)
	node.play(&"idle")
	await runner.process_frame
	var redraws: int = SpriteLoopServer.get_stats().redraws
	node.advance(0.001)
	await runner.process_frame
	check_eq(SpriteLoopServer.get_stats().redraws, redraws, "same frame: no redraw")
	node.advance(1.0 / 24.0)
	await runner.process_frame
	check_eq(SpriteLoopServer.get_stats().redraws, redraws + 1, "new frame: one redraw")


func test_meshes_shared_between_nodes() -> void:
	var a := make_2d(TURRET, true)
	await runner.process_frame
	var meshes: int = SpriteLoopServer.get_stats().meshes
	var b := make_2d(TURRET, true)
	await runner.process_frame
	check_eq(SpriteLoopServer.get_stats().meshes, meshes, "second node reuses the frame mesh")
	b.seek_frame(3, false)
	await runner.process_frame
	check_eq(SpriteLoopServer.get_stats().meshes, meshes + 1, "a new frame builds one mesh")
	a.seek_frame(3, false)
	await runner.process_frame
	check_eq(SpriteLoopServer.get_stats().meshes, meshes + 1, "and shares it")


func test_mesh_packages_render() -> void:
	for path in [DRAGON, WAVE]:
		var node := make_2d(path, true)
		node.play()
		node.advance(0.25)
	await runner.process_frame
	check(SpriteLoopServer.get_stats().meshes > 0, "mesh and deformed-mesh packages build meshes")

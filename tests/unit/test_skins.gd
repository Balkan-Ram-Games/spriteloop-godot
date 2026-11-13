extends "res://tests/test_case.gd"
## Skins and part variants. Clockheart skins: Default, red, blue; part gun0
## has variants gun1 and gun2.


func test_default_skin_selected() -> void:
	var node := make_2d()
	check_eq(node.skin, &"", "property stays empty (package default)")


func test_set_skin() -> void:
	var node := make_2d()
	node.skin = &"red"
	check_eq(node.skin, &"red", "skin by name")
	node.skin = &"blue"
	check_eq(node.skin, &"blue", "switch skin")


func test_set_variant() -> void:
	var node := make_2d()
	check(node.set_variant("gun0", "gun1"), "variant by key")
	check(node.set_variant("gun0", "default"), "\"default\" clears the override")
	check(node.clear_variant("gun0"), "clear_variant")
	node.clear_variants()
	check(not node.set_variant("gun0", "missing"), "unknown variant")
	check(not node.set_variant("missing", "gun1"), "unknown part")
	check(not node.clear_variant("missing"), "clear unknown part")


func test_same_skin_state_shares_baked_geometry() -> void:
	var before: int = SpriteLoopServer.get_stats().bakes
	var a := make_2d()
	var b := make_2d()
	a.skin = &"red"
	b.skin = &"red"
	a.set_variant("gun0", "gun2")
	b.set_variant("gun0", "gun2")
	var bakes: int = SpriteLoopServer.get_stats().bakes - before
	# Default state (maybe already cached), red, red + gun2: at most 3 new bakes
	# for two nodes, because equal skin states share one baked set.
	check(bakes <= 3, "equal skin states share baked sets (%d bakes)" % bakes)


func test_variant_changes_geometry() -> void:
	var node := make_2d(GUN, true)
	node.animation = &"idle_armed"
	await runner.process_frame
	var redraws: int = SpriteLoopServer.get_stats().redraws
	node.set_variant("gun0", "gun2")
	await runner.process_frame
	check(SpriteLoopServer.get_stats().redraws > redraws, "variant change redraws")

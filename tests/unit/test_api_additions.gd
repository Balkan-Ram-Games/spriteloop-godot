extends "res://tests/test_case.gd"
## animation_changed, frame_progress, offset and attachment nodes.
## Clockheart "run": 33 frames at 60 FPS, loops; "jump": 24 frames at 24 FPS.

const RANGER := "res://tests/fixtures/ranger_idle.spla"


func make_3d(path := GUN, in_tree := false) -> SpriteLoop3D:
	var node := SpriteLoop3D.new()
	node.process_callback = SpriteLoop3D.PROCESS_CALLBACK_MANUAL
	node.data = load(path)
	_owned.append(node)
	if in_tree:
		runner.root.add_child(node)
	return node


# --- animation_changed ---------------------------------------------------------

func test_animation_changed() -> void:
	var node := make_2d()
	var changes := [0]
	node.animation_changed.connect(func(): changes[0] += 1)
	node.play(&"run")
	check_eq(changes[0], 0, "playing the already selected animation")
	node.play(&"jump")
	check_eq(changes[0], 1, "play() of another animation")
	node.play(&"Jump")
	check_eq(changes[0], 1, "same animation by display name")
	node.animation = &"idle"
	check_eq(changes[0], 2, "setting the animation property")
	node.animation = &"Idle"
	check_eq(changes[0], 2, "same animation, other reference")
	node.stop()
	node.animation = &"run"
	check_eq(changes[0], 3, "selecting while stopped")


func test_animation_changed_not_emitted_on_load() -> void:
	var node := SpriteLoop2D.new()
	var changes := [0]
	node.animation_changed.connect(func(): changes[0] += 1)
	node.animation = &"jump"
	node.data = load(GUN)
	check_eq(changes[0], 0, "no signal while a package loads")
	node.free()


# --- frame_progress --------------------------------------------------------------

func test_frame_progress() -> void:
	var node := make_2d()
	node.play(&"run")
	node.advance(10.25 / 60.0)
	check_eq(node.frame, 10, "frame")
	check_near(node.frame_progress, 0.25, "progress inside the frame", 0.001)

	check(node.set_frame_and_progress(5, 0.5), "set_frame_and_progress")
	check_eq(node.frame, 5, "frame set")
	check_near(node.frame_progress, 0.5, "progress set", 0.001)
	check_near(node.get_playback_position(), 5.5 / 60.0, "playback position", 0.0001)

	node.frame_progress = 0.75
	check_eq(node.frame, 5, "progress keeps the frame")
	check_near(node.frame_progress, 0.75, "progress property", 0.001)

	node.set_frame_and_progress(3, 2.0)
	check_eq(node.frame, 3, "progress above 1 stays in the frame")

	node.frame = 7
	check_near(node.frame_progress, 0.0, "setting frame resets progress", 0.001)


func test_frame_progress_wraps_and_finishes() -> void:
	var node := make_2d()
	node.play(&"run")
	node.advance(33.5 / 60.0)
	check_eq(node.frame, 0, "wrapped")
	check_near(node.frame_progress, 0.5, "progress after a loop", 0.001)
	node.play(&"jump")
	node.advance(2.0)
	check(not node.is_playing(), "finished")
	check_near(node.frame_progress, 1.0, "finished animation reports 1", 0.0001)


func test_frame_progress_seek_has_no_events() -> void:
	var node := make_2d()
	node.play(&"run")
	node.advance(0.0)
	var log := record_signals(node)
	node.set_frame_and_progress(16, 0.5)
	node.advance(0.0)
	check_eq(only(log, "event").size(), 0, "set_frame_and_progress emits no events")
	check_eq(only(log, "frame_changed").size(), 1, "but frame_changed")


# --- offset ----------------------------------------------------------------------

func test_offset_2d() -> void:
	var node := make_2d(ROBOT_SKINS)
	var rect := node.get_rect()
	var part := node.get_part_transform("body_top")
	node.offset = Vector2(10, -20)
	check_eq(node.offset, Vector2(10, -20), "offset property")
	check_eq(node.get_rect().position, rect.position + Vector2(10, -20), "bounds move")
	check_eq(node.get_rect().size, rect.size, "bounds keep their size")
	check_eq(node.get_part_transform("body_top").origin, part.origin + Vector2(10, -20), "parts move")


func test_offset_3d() -> void:
	var node := make_3d(ROBOT_SKINS)
	node.pixel_size = 0.01
	var aabb := node.get_aabb()
	var part := node.get_part_transform("body_top")
	node.offset = Vector2(50, 100)
	check_near(node.get_aabb().position.y, aabb.position.y + 1.0, "bounds move up (pixels * pixel_size)", 0.00001)
	var moved := node.get_part_transform("body_top")
	check_near(moved.origin.x, part.origin.x + 0.5, "part x", 0.00001)
	check_near(moved.origin.y, part.origin.y + 1.0, "part y", 0.00001)
	# Flipping mirrors around the node origin, after the offset.
	node.flip_h = true
	check_near(node.get_part_transform("body_top").origin.x, -moved.origin.x, "flip mirrors around the origin", 0.00001)
	check_near(node.get_aabb().end.x, -(aabb.position.x + 0.5), "flipped bounds", 0.00001)


# --- attachments -----------------------------------------------------------------

func test_attachment_2d_follows_part() -> void:
	var node := make_2d(ROBOT_SKINS, true)
	var attachment := SpriteLoopAttachment2D.new()
	attachment.part = &"body_top"
	node.add_child(attachment)
	check_eq(attachment.transform, node.get_part_transform("body_top"), "placed on entering the tree")
	node.seek_frame(1, false)
	check_eq(attachment.transform, node.get_part_transform("body_top"), "follows a new frame immediately")
	node.offset = Vector2(5, 7)
	check_eq(attachment.transform, node.get_part_transform("body_top"), "follows the offset")
	attachment.origin = SpriteLoopAttachment2D.ORIGIN_CENTER
	check_eq(attachment.transform, node.get_part_transform("body_top", SpriteLoop2D.PART_ORIGIN_CENTER), "center origin")


func test_attachment_2d_follow_flags() -> void:
	var node := make_2d(RANGER, true)
	var attachment := SpriteLoopAttachment2D.new()
	attachment.part = &"right_hand"
	node.add_child(attachment)
	var full := node.get_part_transform("right_hand")
	check(absf(attachment.rotation) > 0.01, "follows rotation by default")
	attachment.follow_rotation = false
	check_near(attachment.rotation, 0.0, "rotation not followed", 0.00001)
	check_eq(attachment.position, full.origin, "position still followed")
	attachment.follow_scale = false
	check_eq(attachment.scale, Vector2(1, 1), "scale not followed")
	check_eq(attachment.position, full.origin, "position kept")


func test_attachment_2d_missing_part_keeps_transform() -> void:
	var node := make_2d(ROBOT_SKINS, true)
	var attachment := SpriteLoopAttachment2D.new()
	attachment.part = &"body_top"
	node.add_child(attachment)
	var kept := attachment.transform
	attachment.part = &"__missing__"
	check_eq(attachment.transform, kept, "unknown part keeps the last transform")


func test_attachment_3d_follows_part() -> void:
	var node := make_3d(ROBOT_SKINS, true)
	var attachment := SpriteLoopAttachment3D.new()
	attachment.part = &"body_top"
	node.add_child(attachment)
	check(attachment.transform.is_equal_approx(node.get_part_transform("body_top")), "placed on entering the tree")
	node.seek_frame(1, false)
	check(attachment.transform.is_equal_approx(node.get_part_transform("body_top")), "follows a new frame")
	node.flip_h = true
	check(attachment.transform.is_equal_approx(node.get_part_transform("body_top")), "follows flips")
	node.pixel_size = 0.02
	check(attachment.transform.is_equal_approx(node.get_part_transform("body_top")), "follows pixel size")
	node.offset = Vector2(0, 100)
	check(attachment.transform.is_equal_approx(node.get_part_transform("body_top")), "follows the offset")


func test_attachment_3d_follow_flags() -> void:
	var node := make_3d(RANGER, true)
	var attachment := SpriteLoopAttachment3D.new()
	attachment.part = &"right_hand"
	attachment.follow_rotation = false
	attachment.follow_scale = false
	node.add_child(attachment)
	check(attachment.transform.basis.is_equal_approx(Basis()), "neither rotation nor scale followed")
	check(attachment.position.is_equal_approx(node.get_part_transform("right_hand").origin), "position followed")


func test_attachment_3d_follows_billboard() -> void:
	# A camera on +X looking at the origin: its right axis is world -Z.
	var camera := Camera3D.new()
	camera.transform = Transform3D(Basis(Vector3.UP, PI / 2.0), Vector3(10, 0, 0))
	runner.root.add_child(camera)
	_owned.append(camera)
	camera.make_current()
	var node := make_3d(ROBOT_SKINS, true)
	var attachment := SpriteLoopAttachment3D.new()
	attachment.part = &"body_top"
	node.add_child(attachment)
	var local := node.get_part_transform("body_top").origin
	check(attachment.global_position.is_equal_approx(local), "not billboarded: the node's own plane")
	node.billboard = SpriteLoop3D.BILLBOARD_ENABLED
	check(attachment.global_position.is_equal_approx(Vector3(0, local.y, -local.x)),
		"billboarded: turned to face the camera (%s)" % attachment.global_position)
	node.billboard = SpriteLoop3D.BILLBOARD_FIXED_Y
	check(attachment.global_position.is_equal_approx(Vector3(0, local.y, -local.x)),
		"Y-billboard: turned around Y (%s)" % attachment.global_position)
	node.billboard = SpriteLoop3D.BILLBOARD_DISABLED
	check(attachment.global_position.is_equal_approx(local), "back to the node's plane")


# --- editor preview --------------------------------------------------------------

func test_preview_is_editor_only_and_not_saved() -> void:
	var node := make_2d()
	node.frame = 4
	node.preview = true
	check(node.preview, "property set")
	check(not node.is_playing(), "does nothing outside the editor")
	check_eq(node.frame, 4, "frame untouched outside the editor")
	for property in node.get_property_list():
		if property.name == "preview":
			check(property.usage & PROPERTY_USAGE_STORAGE == 0, "never saved with the scene")
			check(property.usage & PROPERTY_USAGE_EDITOR != 0, "shown in the inspector")

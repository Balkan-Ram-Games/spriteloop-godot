extends "res://tests/test_case.gd"
## Part transforms. Expected values come from the Defold extension's API test
## (spriteloop-defold/example/tests/lua_api_test.script), which uses centered
## Y-up coordinates; Godot 2D is Y down, so Y and rotation signs flip.

const RANGER := "res://tests/fixtures/ranger_idle.spla"


func test_pivot_and_center() -> void:
	var node := make_2d(ROBOT_SKINS)
	node.seek_frame(0, false)
	var body := node.get_part_transform("body_top")
	check_near(body.origin.x, 59.0, "body pivot x", 0.001)
	check_near(body.origin.y, -117.833, "body pivot y", 0.001)
	check_near(body.get_rotation(), 0.0, "body rotation", 0.001)
	check_near(body.get_scale().x, 1.0, "body scale x", 0.001)
	check_near(body.get_scale().y, 1.0, "body scale y", 0.001)

	var by_id := node.get_part_transform("4_group_38_4")
	check_near(by_id.origin.y, body.origin.y, "lookup by ID", 0.001)
	var body_center := node.get_part_transform("body_top", SpriteLoop2D.PART_ORIGIN_CENTER)
	check_near(body_center.origin.x, body.origin.x, "centered image: center equals pivot x", 0.001)
	check_near(body_center.origin.y, body.origin.y, "centered image: center equals pivot y", 0.001)

	var arm_pivot := node.get_part_transform("arm_back")
	var arm_center := node.get_part_transform("arm_back", SpriteLoop2D.PART_ORIGIN_CENTER)
	check(absf(arm_center.origin.x - arm_pivot.origin.x) > 0.001, "off-center image: center differs")
	check_near(arm_center.origin.x, -115.323, "arm center x", 0.001)
	check_near(arm_center.origin.y, -4.528, "arm center y", 0.001)

	node.seek_frame(1, false)
	check_near(node.get_part_transform("body_top").origin.y, -116.606362, "follows the current frame", 0.001)


func test_missing_part() -> void:
	var node := make_2d(ROBOT_SKINS)
	check(node.has_part("body_top"), "has_part")
	check(not node.has_part("__missing_part__"), "missing part")
	check_eq(node.get_part_transform("__missing_part__"), Transform2D(), "missing part returns identity")


func test_rotation_skew_scale_and_empty_parts() -> void:
	var node := make_2d(RANGER)
	node.seek_frame(0, false)
	var hand := node.get_part_transform("right_hand")
	# Defold reports -349.807 degrees (Y up), which is +10.193; Y down negates it.
	check_near(wrapf(rad_to_deg(hand.get_rotation()), -180.0, 180.0), -10.193, "rotation sign", 0.001)

	var empty_pivot := node.get_part_transform("empty")
	var empty_center := node.get_part_transform("empty", SpriteLoop2D.PART_ORIGIN_CENTER)
	check_near(empty_pivot.origin.x, 109.053, "empty part x", 0.001)
	check_near(empty_pivot.origin.y, -24.751, "empty part y", 0.001)
	check_eq(empty_center.origin, empty_pivot.origin, "empty part: center equals pivot")

	node.seek_frame(1, false)
	var arrow := node.get_part_transform("arrow")
	# Defold reports the authored scale (0.976091 on both axes). This part is
	# skewed on both axes, which a Transform2D decomposes into one skew, so only
	# the X scale and the skew angle are comparable. The full matrix is covered
	# by the off-center "arm_back" check above.
	check_near(arrow.get_scale().x, 0.976091, "scale x", 0.001)
	check_near(absf(rad_to_deg(arrow.get_skew())), 2.691364, "skew magnitude", 0.01)

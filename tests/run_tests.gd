extends SceneTree
## Headless test runner.
##
##   godot --headless --path . --import   (twice on a fresh checkout)
##   godot --headless --path . -s res://tests/run_tests.gd
##
## Exits with code 1 when any check fails.

const SUITES := [
	"res://tests/unit/test_data.gd",
	"res://tests/unit/test_playback.gd",
	"res://tests/unit/test_events.gd",
	"res://tests/unit/test_skins.gd",
	"res://tests/unit/test_parts.gd",
	"res://tests/unit/test_rendering.gd",
	"res://tests/unit/test_lifetime.gd",
	"res://tests/unit/test_3d.gd",
	"res://tests/unit/test_api_additions.gd",
	"res://tests/unit/test_animation_player.gd",
	"res://tests/unit/test_arena.gd",
	"res://tests/unit/test_mipmaps.gd",
]

var checks := 0
var failures: Array[String] = []
var current_test := ""
var guard
## A test still running after this long fails and ends the run (e.g. waiting
## for a signal the headless renderer never emits).
const TEST_TIMEOUT_SECONDS := 30.0
## Time.get_ticks_msec() deadline of the running test; 0 between tests.
var _deadline_msec := 0


func _initialize() -> void:
	# Script errors abort a test function silently; count them as failures.
	guard = load("res://tests/render/error_guard.gd").install()
	_run.call_deferred()


func _run() -> void:
	print("SpriteLoop tests on Godot %s, extension %s" % [
		Engine.get_version_info().string, SpriteLoopServer.get_version()])
	for path in SUITES:
		var script: GDScript = load(path)
		if script == null or not script.can_instantiate():
			failures.append("%s: suite failed to load (parse error?)" % path)
			continue
		var suite = script.new()
		suite.runner = self
		for method in suite.get_method_list():
			var test_name: String = method.name
			if not test_name.begins_with("test_"):
				continue
			current_test = "%s::%s" % [path.get_file().get_basename(), test_name]
			var script_errors: int = guard.script_error_count
			_deadline_msec = Time.get_ticks_msec() + int(TEST_TIMEOUT_SECONDS * 1000.0)
			await suite.call(test_name)
			_deadline_msec = 0
			if guard.script_error_count != script_errors:
				failures.append("%s: script error: %s" % [current_test, guard.last_script_error])
			suite.cleanup()
	print("%d checks, %d failed" % [checks, failures.size()])
	for failure in failures:
		printerr("FAIL ", failure)
	quit(1 if failures.size() > 0 else 0)


func record(ok: bool, message: String) -> void:
	checks += 1
	if not ok:
		failures.append("%s: %s" % [current_test, message])


# One deadline checked every frame, instead of a timer per test: pending
# SceneTreeTimers are reported as leaked when the run quits.
func _process(_delta: float) -> bool:
	if _deadline_msec != 0 and Time.get_ticks_msec() > _deadline_msec:
		_deadline_msec = 0
		printerr("FAIL %s: did not finish within %d s" % [current_test, TEST_TIMEOUT_SECONDS])
		quit(1)
	return false

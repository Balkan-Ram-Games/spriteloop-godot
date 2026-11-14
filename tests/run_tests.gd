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
]

var checks := 0
var failures: Array[String] = []
var current_test := ""
var guard
## A test still running after this long fails and ends the run (e.g. waiting
## for a signal the headless renderer never emits).
const TEST_TIMEOUT_SECONDS := 30.0
var _test_serial := 0


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
			_test_serial += 1
			_watch(_test_serial, current_test)
			await suite.call(test_name)
			_test_serial += 1
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


func _watch(serial: int, test_name: String) -> void:
	await create_timer(TEST_TIMEOUT_SECONDS).timeout
	if serial == _test_serial:
		printerr("FAIL %s: did not finish within %d s" % [test_name, TEST_TIMEOUT_SECONDS])
		quit(1)

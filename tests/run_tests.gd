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
]

var checks := 0
var failures: Array[String] = []
var current_test := ""
var guard


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
			await suite.call(test_name)
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

extends Logger
## Records engine errors so windowed tools can stop on the first one instead
## of running on while the log floods. Requires Godot 4.5 (OS.add_logger).

var first_error := ""
var error_count := 0


func _log_error(function: String, file: String, line: int, code: String, rationale: String,
		_editor_notify: bool, _error_type: int, _script_backtraces: Array[ScriptBacktrace]) -> void:
	error_count += 1
	if first_error.is_empty():
		first_error = "%s (%s:%d in %s)" % [rationale if not rationale.is_empty() else code, file, line, function]


func _log_message(_message: String, _error: bool) -> void:
	pass


static func install() -> Logger:
	var guard = load("res://tests/render/error_guard.gd").new()
	OS.add_logger(guard)
	return guard

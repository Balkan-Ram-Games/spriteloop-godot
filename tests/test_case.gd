extends RefCounted
## Base class for test suites: assertions, node helpers and signal recording.

const GUN := "res://tests/fixtures/clockheart_gun.spla"
const TURRET := "res://tests/fixtures/turret.spla"
const DRAGON := "res://tests/fixtures/mesh_dragon_idle.spla"
const WAVE := "res://tests/fixtures/animated-wave.spla"
const ROBOT_SKINS := "res://tests/fixtures/robot_idle_skins.spla"
const INVALID := "res://tests/fixtures/invalid.spla"

var runner
var _owned: Array[Node] = []


func check(condition: bool, message: String) -> void:
	runner.record(condition, message)


func check_eq(actual, expected, message: String) -> void:
	runner.record(actual == expected, "%s: expected <%s>, got <%s>" % [message, expected, actual])


func check_near(actual: float, expected: float, message: String, tolerance := 0.0001) -> void:
	runner.record(absf(actual - expected) <= tolerance,
		"%s: expected %f, got %f" % [message, expected, actual])


## A SpriteLoop2D driven by advance() only. Freed after the test.
func make_2d(path := GUN, in_tree := false) -> SpriteLoop2D:
	var node := SpriteLoop2D.new()
	node.process_callback = SpriteLoop2D.PROCESS_CALLBACK_MANUAL
	node.data = load(path)
	_owned.append(node)
	if in_tree:
		runner.root.add_child(node)
	return node


## Records every signal of interest in order, as small dictionaries.
func record_signals(node: Node) -> Array:
	var log := []
	node.event_triggered.connect(func(event_name, data, animation, frame, source_frame):
		log.append({"signal": "event", "name": event_name, "data": data,
			"animation": animation, "frame": frame, "source_frame": source_frame}))
	node.frame_changed.connect(func(): log.append({"signal": "frame_changed", "frame": node.frame}))
	node.animation_looped.connect(func(): log.append({"signal": "looped"}))
	node.animation_finished.connect(func(): log.append({"signal": "finished"}))
	return log


static func only(log: Array, signal_name: String) -> Array:
	return log.filter(func(entry): return entry.signal == signal_name)


func cleanup() -> void:
	for node in _owned:
		if is_instance_valid(node):
			node.free()
	_owned.clear()

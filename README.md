# SpriteLoop for Godot

> Status: in development (`0.1.0-dev`). Windows builds only so far; other
> platforms, 3D and masks are planned. Not released yet.

> For SpriteLoop desktop app bugs and feature requests, use the
> [SpriteLoop app issue tracker](https://github.com/Balkan-Ram-Games/spriteloop-app/issues).

SpriteLoop for Godot plays `.spla` animation packages exported from SpriteLoop.
It is a native GDExtension: the SpriteLoop runtime loads and bakes animations in
C++, and Godot renders them. It supports part meshes and mesh deformation,
sprite-state crossfades, skins, part variants, events and part transforms.

## Install

Copy `addons/spriteloop/` into your project's `addons/` folder. The addon
contains prebuilt libraries; you do not need to compile anything.

Godot 4.5 or newer is required.

After adding the addon to an open project, restart the editor once. Godot
registers the `.spla` importer when the editor starts, so packages added
before the restart stay unimported until then.

## Use

1. Add `.spla` files to the project. They import as `SpriteLoopData` resources.
2. Add a `SpriteLoop2D` node and assign the package to **Data**.
3. Pick an **Animation** and set **Autoplay** to start it when the scene runs.

Control it from scripts with the usual Godot playback methods:

```gdscript
@onready var robot: SpriteLoop2D = $Robot

func _ready() -> void:
	robot.play(&"run")
	robot.animation_finished.connect(_on_animation_finished)

func _on_animation_finished() -> void:
	robot.play(&"idle")
```

Animation, skin, part and variant arguments accept the exported ID or the
display name. IDs are matched first.

### Playback

| Method / property | Behaviour |
|---|---|
| `play(animation = &"")` | Starts an animation from frame 0. Calling it for the animation that is already playing or paused continues from the current frame. With no argument it plays `animation`. |
| `pause()` | Keeps the current frame; `play()` resumes. |
| `stop()` | Stops and rewinds to frame 0. |
| `seek(seconds, emit_events = true)`, `seek_frame(frame, emit_events = true)` | Jump to a time or frame. |
| `frame` | Current frame. Setting it seeks to the start of that frame without emitting events. |
| `frame_progress`, `set_frame_and_progress(frame, progress)` | Position inside the current frame, 0 to 1 (1 once a non-looping animation has finished). Setting it seeks without emitting events. |
| `speed_scale` | Playback speed multiplier, 0 or more. |
| `loop_mode` | `LOOP_MODE_PACKAGE` (as exported), `LOOP_MODE_LOOP` or `LOOP_MODE_ONCE`. |
| `process_callback` | `PROCESS_CALLBACK_IDLE`, `PROCESS_CALLBACK_PHYSICS`, or `PROCESS_CALLBACK_MANUAL` with `advance(delta)`. |

A non-looping animation stops on its last frame and emits `animation_finished`.
A looping animation emits `animation_looped` each time it wraps. `frame_changed`
is emitted whenever the displayed frame changes, and `animation_changed` when the
current animation switches to another one (not when a package loads).

### Events

Animation events are delivered through the `event_triggered` signal:

```gdscript
func _ready() -> void:
	$Robot.event_triggered.connect(_on_event)

func _on_event(event_name: StringName, data: String, animation: StringName,
		frame: int, source_frame: int) -> void:
	if event_name == &"step":
		$Footstep.play()
```

- Starting an animation emits the events of frame 0.
- Playback emits the events of every frame it crosses, in order, including
  frames crossed in one long update and across a loop.
- `seek()` and `seek_frame()` emit only the destination frame's events. Pass
  `emit_events = false` to skip them. Seeking to the current frame emits its
  events again.
- Setting `frame` and reimporting a package never emit events.
- Events are emitted during the node's next update, before `frame_changed`,
  `animation_looped` and `animation_finished`.
- Set `emit_events = false` to turn events off for the node.

`data` is the exported event payload, or an empty string. `frame` is the
zero-based frame in the exported animation; `source_frame` is the frame number
in the SpriteLoop editor.

### AnimationPlayer and cutscenes

SpriteLoop nodes can be driven from an `AnimationPlayer` timeline like any other
node:

- Key `animation`, `skin` or `flip_h` with discrete keys to switch them at a
  moment in the timeline.
- Key `frame` to lock the character's pose to the timeline (scrubbing the
  timeline then scrubs the character). Do this on a character that is not
  playing; otherwise its own playback and the keys compete.
- Use a method track calling `play()` or `seek()` to start an animation at a
  precise moment and let it run on its own.
- Key `modulate`/`self_modulate` or `visible` for fades and appearances.

Keyed `frame` changes never emit `event_triggered`, and attachments follow
keyed frames.

### Skins and part variants

The package's `default` skin is used when **Skin** is empty.

```gdscript
robot.skin = &"blue_robot"                 # Applies the skin's variants and visibility.
robot.set_variant("head", "head_helmet")   # Overrides one part after the skin.
robot.set_variant("head", "default")       # Clears that part's override.
robot.clear_variant("head")                # Same.
robot.clear_variants()                     # Clears every override.
```

`set_variant()` and `clear_variant()` return `false` and log a warning when the
part or variant does not exist. Manual variants take precedence over the skin,
and changing the skin keeps them. Characters with the same package, skin and
variant overrides share their baked geometry.

### Part transforms

```gdscript
var hand: Transform2D = robot.get_part_transform("right_hand")
var muzzle := robot.to_global(robot.get_part_transform("gun_point").origin)
var center := robot.get_part_transform("arm", SpriteLoop2D.PART_ORIGIN_CENTER)
```

The transform is in the node's local space (pixels, Y down, `offset` included)
for the frame that is currently displayed. The default origin is the part pivot;
`PART_ORIGIN_CENTER` uses the center of the part's base image. Empty Parts work
as attachment points. `has_part(part)` tells whether a part is in the current
frame; for a missing part `get_part_transform()` warns and returns the identity
transform.

### Attachments

To make other nodes follow a part without code, add a `SpriteLoopAttachment2D`
(or `SpriteLoopAttachment3D` under a `SpriteLoop3D`) as a direct child and
choose its **Part**. Everything under it moves with that part, like Godot's
`BoneAttachment2D`/`BoneAttachment3D`:

```text
Robot (SpriteLoop2D)
└─ Muzzle (SpriteLoopAttachment2D, part = "gun_point")
   ├─ Flash (Sprite2D)
   └─ Hitbox (Area2D)
```

- **Origin** is the part pivot or the center of its base image.
- Turn off **Follow Rotation** or **Follow Scale** to follow only the position
  (and rotation).
- The attachment updates whenever the displayed pose changes (new frame,
  animation, seek, `offset`, flips), in the same update, so it never lags a
  frame behind. It also follows in the editor.
- If the part is not in the current frame, the attachment keeps its last
  transform.
- The 3D attachment follows the part in the node's XY plane. When the
  `SpriteLoop3D` is billboarded, it also turns with the character towards the
  camera, updated just before each frame is drawn. In the editor it faces the
  first 3D viewport's camera.

### Offset

`offset` moves the drawn character relative to the node origin, in package
pixels (Y down in 2D, Y up in 3D), like `Sprite2D`/`Sprite3D`. Use it to put the
origin at the character's feet. Placement, billboarding and flipping then pivot
on the feet. Bounds, culling, editor picking, part transforms and attachments
include it. In 3D it is applied before flipping, so a flipped character mirrors
around its origin.

### Tint, filtering and materials

`SpriteLoop2D` is a regular `CanvasItem`:

- Tint with `modulate` or `self_modulate`. This does not rebuild geometry.
- Use **Texture → Filter → Nearest** for pixel art.
- Assign a `CanvasItemMaterial` or `ShaderMaterial` to **Material**.
- `z_index`, Y-sort, visibility layers and light masks work as usual.

`get_rect()` returns the bounds of every frame for the active skin.

### 3D

`SpriteLoop3D` plays the same packages in 3D, as a flat character in the
node's XY plane facing +Z, like `Sprite3D`. It has the same playback, event,
skin, variant and signal API as `SpriteLoop2D`, plus:

| Property | Meaning |
|---|---|
| `pixel_size` | World size of one package pixel. Default 0.01 (100 px = 1 m); Godot 3D units are meters. |
| `offset` | Moves the drawn character, in package pixels, Y up. See [Offset](#offset). |
| `modulate`, `flip_h`, `flip_v` | Tint and mirroring. |
| `billboard` | `BILLBOARD_DISABLED`, `BILLBOARD_ENABLED` or `BILLBOARD_FIXED_Y`. |
| `shaded` | React to scene lights. Off by default. |
| `double_sided`, `no_depth_test`, `render_priority` | As in `Sprite3D`. |
| `alpha_cut` | `ALPHA_CUT_OPAQUE_PREPASS` (default: depth against the world, shadows, soft edges), `ALPHA_CUT_DISABLED` (blended only), `ALPHA_CUT_DISCARD` or `ALPHA_CUT_HASH`. |
| `texture_filter` | `TEXTURE_FILTER_LINEAR` or `TEXTURE_FILTER_NEAREST`. |

`get_part_transform()` returns a `Transform3D` in the node's local space
(world units, Y up). It does not include billboard rotation.

The built-in material comes from `addons/spriteloop/shaders/spriteloop_3d.gdshader`
and its reusable core `spriteloop_3d.gdshaderinc`. Nodes with the same settings,
tint and flips share one material. Like any `GeometryInstance3D`, the node also
supports shadow casting, transparency, visibility ranges and render layers.

#### Custom 3D shaders

To give characters your own look (hit flashes, dissolves, toon lighting), set
**Material** to a `ShaderMaterial` whose spatial shader includes the SpriteLoop
core:

```glsl
shader_type spatial;
render_mode skip_vertex_transform, cull_disabled, depth_prepass_alpha;  // skip_vertex_transform is required
#include "res://addons/spriteloop/shaders/spriteloop_3d.gdshaderinc"

uniform vec4 flash_color : source_color = vec4(1.0, 0.25, 0.2, 1.0);  // shared
instance uniform float flash = 0.0;                                   // per character

void vertex() {
	SPRITELOOP_VERTEX;  // part layering, flips, billboard, authored tint
}

void fragment() {
	vec4 color = spriteloop_color(UV, COLOR);
	ALBEDO = mix(color.rgb, flash_color.rgb, flash);
	ALPHA = color.a;
}
```

- The node fills in the uniforms in the **Spriteloop** group (atlas, flips,
  billboard mode, `modulate`, filter, alpha threshold) from its own
  properties; do not set them yourself.
- Your shader's `render_mode` decides shading, culling and transparency, so
  `shaded`, `double_sided`, `no_depth_test` and `alpha_cut` are read-only while
  a custom material is set.
- One material can be shared by many characters. Nodes that look the same
  share one internal copy; editing the material in the inspector updates them.
  After changing its parameters from a script, call `material.emit_changed()`.
- For per-character values, use an `instance uniform` and
  `node.set_instance_shader_parameter("flash", 1.0)`. This needs no extra
  materials, but instance uniforms share a global buffer
  (`rendering/limits/global_shader_variables/buffer_size`), so avoid them on
  thousands of characters.
- See `example/hit_flash_3d.gdshader` and the flashing robot in
  `example/example_3d.tscn`.

How the material slots combine:

| Set | Renders |
|---|---|
| nothing | the built-in material |
| **Material** | your shader, with SpriteLoop layering, flips and billboarding |
| **Material Override** | the override only; it replaces everything, including SpriteLoop layering, flips and billboarding. The node warns when it hides **Material** |
| **Material Overlay** | an extra pass on top of the above. The overlay does not run the SpriteLoop vertex step, so it only lines up on characters without billboard and flips; prefer **Material** for effects |

### Editor

`.spla` files show up as `SpriteLoopData` resources. Check **Preview** in the
inspector to play the selected animation in the editor; one-shot animations
repeat, and changing the animation or skin updates the preview. Previewing is
editor-only and never saved: turning it off returns to the frame set before.

In the 2D and 3D editors, click a SpriteLoop character's artwork to select it
and drag it to move it. A selected `SpriteLoop2D` shows its bounds with handles like
`Sprite2D`: drag an edge or corner handle to scale it with the opposite side
fixed, and hold Shift to keep the aspect ratio. A `SpriteLoop2D` that is the
root of the open scene can only be selected in the Scene dock.

On a brand-new project (no `.godot/` folder yet), Godot can crash while closing
after the first scan that discovers the extension. This happens in Godot's
shutdown with any extension that registers classes; the next start is
normal. In CI, run `--import` twice and ignore the first exit code.

### Package information

`SpriteLoopData` exposes `get_animation_names()`, `get_skin_names()`,
`get_part_names()`, `get_variant_names(part)`, `get_animation_frame_count()`,
`get_animation_fps()`, `get_animation_length()`, `is_animation_looping()` and
`get_canvas_size()`. Load a package at runtime, for example downloaded content,
with `SpriteLoopData.load_from_file(path)`.

### Diagnostics

`SpriteLoopServer.get_stats()` returns counts of live packages, baked skin
states, frame meshes and their memory, and how often nodes re-recorded draw
commands.

## Performance notes

- Frame meshes are built once per package, skin state and frame, and shared by
  every node showing that frame. A node does no per-frame vertex work; it only
  re-records one draw command when its displayed frame changes.
- Each visible character is one draw call. Godot does not batch mesh draws
  across nodes, so large crowds cost one draw call per character.

## Known limitations

- Masks are not supported yet. Masked parts are drawn without their mask.
- `SpriteLoop3D` has no texture filtering with mipmaps yet, so distant
  characters can shimmer.
- `ALPHA_CUT_HASH` does not work in the Compatibility renderer (transparent
  areas render white). This is a Godot limitation that also affects `Sprite3D`.
- Like `Sprite3D` and other materials, the first `SpriteLoop3D` drawn with a new
  combination of `shaded`, `double_sided`, `no_depth_test` and `alpha_cut`
  compiles a shader, which stalls that frame. Forward+ and Mobile cache the
  result on disk. The Compatibility renderer compiles again on every run (about
  0.2–0.4 s per combination on a desktop GPU). Show such characters once behind
  a loading screen to avoid the hitch. Billboard mode, filtering, flips and tint
  do not create new shaders.
- Parts of a `SpriteLoop3D` share one plane. To keep the authored layering in
  depth-writing modes and shadows, the shader moves each later part a tiny
  distance towards the viewer along the view ray (0.02% of the camera distance
  per part). The screen position does not change, and the order holds from the
  front, from behind and when flipped.
- Only Windows x86_64 libraries are included so far.
- Playback runs forward only; negative `speed_scale` is clamped to 0.

## Build from source

The SpriteLoop SDK is included as headers and prebuilt static libraries in
`sdk/`. Building the extension needs Python with SCons and a C++17 compiler.

```sh
git submodule update --init
scons platform=windows target=template_debug
scons platform=windows target=template_release
```

Output goes to `addons/spriteloop/bin/`.

## Tests

The repository is also the example and test project. With the Godot console
executable:

```sh
godot --headless --path . --import
godot --headless --path . --import
godot --headless --path . -s res://tests/run_tests.gd
```

The first import registers the extension; the second imports the `.spla`
fixtures. The test runner exits with code 1 when a check fails.

To render a scene to a PNG (needs a GPU):

```sh
godot --path . --rendering-method gl_compatibility -s res://tests/render/capture.gd -- res://example/example.tscn out.png
```

## Layout

```text
project.godot          # Example and test project
addons/spriteloop/     # The addon: .gdextension, icons, bin/
example/               # Example scene
tests/                 # Headless tests, fixtures, render capture tools
src/                   # Extension source (C++)
sdk/                   # SpriteLoop SDK headers and prebuilt libraries
godot-cpp/             # godot-cpp submodule (4.5 branch)
SConstruct             # Build script
```

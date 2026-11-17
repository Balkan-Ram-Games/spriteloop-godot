# SpriteLoop for Godot

[![Godot extension version](https://img.shields.io/badge/Godot%20extension-v0.1.0--alpha-478cbf?style=for-the-badge&logo=godotengine&logoColor=white)](https://github.com/Balkan-Ram-Games/spriteloop-godot/releases/tag/v0.1.0-alpha)
[![Godot 4.5+](https://img.shields.io/badge/Godot-4.5%2B-478cbf?style=for-the-badge&logo=godotengine&logoColor=white)](https://godotengine.org/)
[![Support SpriteLoop on itch.io](https://img.shields.io/badge/Support-SpriteLoop%20on%20itch.io-fa5c5c?style=for-the-badge&logo=itchdotio&logoColor=white)](https://balkanramgames.itch.io/spriteloop/purchase)

> For SpriteLoop desktop app bugs and feature requests, use the
> [SpriteLoop app issue tracker](https://github.com/Balkan-Ram-Games/spriteloop-app/issues).

SpriteLoop for Godot adds native playback for `.spla` animation packages. It
includes the `SpriteLoop2D` and `SpriteLoop3D` nodes, part attachments, a
`.spla` importer, editor preview and a small example project.

## Install

Download `spriteloop-godot-v0.1.0-alpha.zip` from the
[release page](https://github.com/Balkan-Ram-Games/spriteloop-godot/releases/tag/v0.1.0-alpha)
and extract it into your project folder, so the addon ends up in
`addons/spriteloop/`. The addon contains prebuilt libraries; nothing needs to be
compiled. Godot 4.5 or newer is required.

After adding the addon to an open project, restart the editor once so it
imports the `.spla` files.

## Use

Add `.spla` files to the project; they import as `SpriteLoopData` resources.
Add a `SpriteLoop2D` (or `SpriteLoop3D`) node, assign the package to **Data**,
pick an **Animation** and enable **Autoplay**, or control it from a script:

```gdscript
@onready var robot: SpriteLoop2D = $Robot

func _ready() -> void:
	robot.play(&"run")
	robot.animation_finished.connect(func(): robot.play(&"idle"))
```

Animation, skin, part and variant arguments accept the exported ID or the
display name. IDs are matched first.

Check **Preview** in the inspector to play the animation in the editor.

The example project (`example/`) shows 2D and 3D characters, attachments, a
custom 3D shader and a small arena game.

## Playback

| Method / property | Behaviour |
|---|---|
| `play(animation = &"")` | Starts an animation from frame 0, or resumes the current one. |
| `pause()`, `stop()` | Pause on the current frame, or stop and rewind. |
| `seek(seconds, emit_events = true)`, `seek_frame(frame, emit_events = true)` | Jump to a time or frame. |
| `frame`, `frame_progress` | Current frame and position inside it (0 to 1). Setting them never emits events. |
| `speed_scale` | Playback speed, 0 or more. |
| `loop_mode` | `LOOP_MODE_PACKAGE` (as exported), `LOOP_MODE_LOOP` or `LOOP_MODE_ONCE`. |
| `process_callback` | `PROCESS_CALLBACK_IDLE`, `PROCESS_CALLBACK_PHYSICS`, or `PROCESS_CALLBACK_MANUAL` with `advance(delta)`. |

Signals: `animation_finished`, `animation_looped`, `animation_changed`,
`frame_changed` and `event_triggered`.

The nodes can also be driven from an `AnimationPlayer`: key `animation`,
`skin` or `frame`, or call `play()` from a method track. Keyed frames never
emit events.

## Animation Events

```gdscript
func _ready() -> void:
	$Robot.event_triggered.connect(_on_event)

func _on_event(event_name: StringName, data: String, animation: StringName,
		frame: int, source_frame: int) -> void:
	if event_name == &"step":
		$Footstep.play()
```

Starting an animation emits frame 0's events, playback emits the events of
every frame it crosses (also in long updates and across loops), and seeking
emits only the destination frame's events. Set `emit_events = false` on the
node to turn events off. `source_frame` is the frame number in the SpriteLoop
editor.

## Skins and Part Variants

```gdscript
robot.skin = &"blue_robot"                 # Empty uses the package's default skin.
robot.set_variant("head", "head_helmet")   # Overrides one part after the skin.
robot.clear_variant("head")
robot.clear_variants()
```

Manual variants take precedence over the skin and survive skin changes.
`set_variant()` returns `false` and warns when the part or variant does not
exist.

## Part Transforms and Attachments

```gdscript
var muzzle := robot.to_global(robot.get_part_transform("gun_point").origin)
```

`get_part_transform(part, origin)` returns the part's transform for the
displayed frame in the node's local space (`Transform3D` for `SpriteLoop3D`).
The origin is the part pivot, or `PART_ORIGIN_CENTER` for the center of its
image. `has_part()` tells whether a part is in the current frame.

To make nodes follow a part without code, add a `SpriteLoopAttachment2D` (or
`SpriteLoopAttachment3D`) as a direct child and set its **Part**, like
`BoneAttachment2D`:

```text
Robot (SpriteLoop2D)
└─ Muzzle (SpriteLoopAttachment2D, part = "gun_point")
   ├─ Flash (Sprite2D)
   └─ Hitbox (Area2D)
```

## Tint, Filtering and Materials

`SpriteLoop2D` works like `Sprite2D`: `modulate`, `self_modulate`,
**Texture → Filter**, **Material** and lights apply to the whole character,
including masked parts. `offset` moves the drawn character relative to the
node origin, in package pixels.

For characters drawn much smaller than their art, turn on **Mipmaps** in the
`.spla` file's Import dock and use a mipmap texture filter.

## 3D

`SpriteLoop3D` plays the same packages as a flat character in the node's XY
plane, like `Sprite3D`, with the same API as `SpriteLoop2D` plus `pixel_size`
(default 0.01: 100 px = 1 m), `billboard`, `shaded`, `double_sided`,
`no_depth_test`, `alpha_cut` and `texture_filter`.

For custom looks (hit flashes, dissolves, toon lighting), set **Material** to
a `ShaderMaterial` that includes the SpriteLoop shader core:

```glsl
shader_type spatial;
render_mode skip_vertex_transform, cull_disabled, depth_prepass_alpha;  // skip_vertex_transform is required
#include "res://addons/spriteloop/shaders/spriteloop_3d.gdshaderinc"

instance uniform float flash = 0.0;

void vertex() {
	SPRITELOOP_VERTEX;
}

void fragment() {
	vec4 color = spriteloop_color(UV, COLOR);
	ALBEDO = mix(color.rgb, vec3(1.0, 0.25, 0.2), flash);
	ALPHA = color.a;
}
```

The node fills in the uniforms in the **Spriteloop** group; your shader's
`render_mode` replaces `shaded`, `double_sided`, `no_depth_test` and
`alpha_cut`. **Material Override** skips the SpriteLoop vertex step (layering,
flips, billboard), so prefer **Material**. See `example/hit_flash_3d.gdshader`.

## Masks

Masks exported from SpriteLoop work automatically in 2D and 3D. Packages
without masks need no setup and cost nothing extra.

Masked frames are composed once and shared by every node showing the same
frame, so repeated masked animations stay cheap. Composed frames keep the
package's resolution; export at a higher resolution for crisp close-ups.

Set `spriteloop/masks/cache_budget_mb` in Project Settings to choose how much
memory unused composed frames may keep (see [Project Settings](#project-settings)).

## Package Information

`SpriteLoopData` exposes `get_animation_names()`, `get_skin_names()`,
`get_part_names()`, `get_variant_names(part)`, `get_animation_frame_count()`,
`get_animation_fps()`, `get_animation_length()`, `is_animation_looping()` and
`get_canvas_size()`. Load a package at runtime with
`SpriteLoopData.load_from_file(path)`.

`SpriteLoopServer.get_stats()` returns cache and memory counters for debugging.

## Project Settings

| Setting | Default | Meaning |
|---|---|---|
| `spriteloop/masks/cache_budget_mb` | 32 | Memory for composed mask frames no longer on screen, in MiB (0–128). Also `SpriteLoopServer.mask_cache_budget_mb` at runtime. |

## Known Limitations

- Characters from different `.spla` exports do not batch together; large
  crowds cost about one draw call per visible character. On Android, the
  Compatibility renderer handles large 2D crowds best.
- Playback runs forward only; negative `speed_scale` is clamped to 0.

## Supported Platforms

Windows x86_64, Linux x86_64, macOS (universal), iOS arm64, Android (arm64,
arm32, x86_64) and Web (with and without threads).

## Build from Source

The SpriteLoop SDK is included as headers and prebuilt libraries in `sdk/`.
Building needs Python with SCons and a C++17 compiler:

```sh
git submodule update --init
scons platform=windows target=template_debug
scons platform=windows target=template_release
```

Output goes to `addons/spriteloop/bin/`.

## Layout

```text
project.godot          # Example project
addons/spriteloop/     # The addon: .gdextension, icons, shaders, bin/
example/               # Example scenes and the arena game
src/                   # Extension source (C++)
sdk/                   # SpriteLoop SDK headers and prebuilt libraries
godot-cpp/             # godot-cpp submodule
SConstruct             # Build script
```

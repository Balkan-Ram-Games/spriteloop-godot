#pragma once

#include "core/package_asset.h"

#include "spriteloop/spla_player.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace spriteloop_godot {

// Implemented by SpriteLoop2D / SpriteLoop3D.
class PlaybackHost {
public:
    // The displayed frame, skin or package changed: re-record draw commands.
    virtual void playback_visual_changed() = 0;
    // The baked set changed (package or skin state): update culling bounds.
    virtual void playback_bounds_changed() = 0;
    // Object that owns the signals.
    virtual godot::Object* playback_owner() = 0;

protected:
    ~PlaybackHost() = default;
};

enum class LoopMode { package = 0, loop = 1, once = 2 };

// Node-independent playback state shared by the 2D and 3D nodes: player,
// skin state, pause/finish handling and signal emission.
//
// Semantics follow AnimatedSprite2D where Godot users expect them:
// play() restarts (or resumes a paused animation), pause() keeps the frame,
// stop() rewinds, a finished non-looping animation stops on its last frame.
// Event semantics match the Defold extension: starting an animation emits
// frame 0, playback emits every crossed frame, seeking emits only the
// destination frame.
class Playback {
public:
    explicit Playback(PlaybackHost& host);
    ~Playback();

    // Package --------------------------------------------------------------
    // Replaces the package. With restore_state, the current animation, frame,
    // play state, skin and variant overrides are re-applied by stable IDs
    // (used when the resource is reimported).
    void set_asset(std::shared_ptr<PackageAsset> asset, bool restore_state);
    const std::shared_ptr<PackageAsset>& asset() const { return asset_; }

    // Properties -----------------------------------------------------------
    void set_animation(const godot::StringName& animation);
    godot::StringName get_animation() const { return animation_; }
    void set_skin(const godot::StringName& skin);
    godot::StringName get_skin() const { return skin_; }
    void set_frame(int frame);
    int get_frame() const;
    int get_frame_count() const;
    void set_speed_scale(double speed_scale);
    double get_speed_scale() const { return speed_scale_; }
    void set_loop_mode(LoopMode mode);
    LoopMode get_loop_mode() const { return loop_mode_; }
    void set_emit_events(bool enabled) { emit_events_ = enabled; }
    bool get_emit_events() const { return emit_events_; }

    // Methods --------------------------------------------------------------
    bool play(const godot::StringName& animation);
    void pause();
    void stop();
    bool is_playing() const { return playing_; }
    bool seek(double seconds, bool emit_events);
    bool seek_frame(int frame, bool emit_events);
    double get_playback_position() const;
    void advance(double delta);

    bool set_variant(const godot::String& part, const godot::String& variant);
    bool clear_variant(const godot::String& part);
    void clear_variants();

    // Part transform in Godot 2D node space (pixels, Y down). Returns false
    // when the part does not exist or is not in the current frame.
    bool get_part_transform_2d(const godot::String& part, bool center,
                               godot::Transform2D& transform, bool warn) const;
    // Part transform in SpriteLoop space (pixels, Y up, centered).
    bool get_part_transform_y_up(const godot::String& part, bool center,
                                 godot::Transform2D& transform, bool warn) const;

    // Rendering ------------------------------------------------------------
    godot::RID current_mesh_2d();
    godot::RID current_mesh_3d(float pixel_size);
    godot::RID texture_rid() const;
    godot::Rect2 bounds_2d() const;
    const spriteloop::SplaBounds* bounds() const;

private:
    void rebuild_player();
    void apply_loop_mode();
    void apply_skin_state();
    void select_animation_without_events(int animation_index);
    int current_animation_index() const;
    int resolve_default_skin() const;
    void stash_pending_events();
    void dispatch_events();
    void emit(const char* signal);
    void visual_changed();

    PlaybackHost& host_;
    // Declared before player_: the player references the asset's package.
    std::shared_ptr<PackageAsset> asset_;
    std::unique_ptr<spriteloop::SplaPlayer> player_;
    std::shared_ptr<BakedSet> baked_;
    spriteloop::SplaSkinState skin_state_;

    godot::StringName animation_;
    godot::StringName skin_;
    double speed_scale_ = 1.0;
    LoopMode loop_mode_ = LoopMode::package;
    bool emit_events_ = true;

    bool playing_ = false;
    bool paused_ = false;   // Paused by pause(); play() resumes.
    bool finished_ = false; // A non-looping animation reached its end.
    bool play_on_load_ = false; // play() was called before a package was set.
    int pending_frame_ = 0; // Frame requested before a package was available.
    std::uint64_t generation_ = 0; // Changes on play/stop/package change.
    // Events taken from the player before an operation that would discard
    // its queue; emitted on the next advance(), before newer events.
    std::vector<spriteloop::SplaPlaybackEvent> backlog_;
};

} // namespace spriteloop_godot

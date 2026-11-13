#include "core/playback.h"

#include "core/names.h"

#include "spriteloop/spla_baked.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <utility>

using namespace godot;

namespace spriteloop_godot {

namespace {

constexpr double pi = 3.14159265358979323846;

bool equals_ignore_case(const std::string& value, const char* expected)
{
    const std::size_t length = std::char_traits<char>::length(expected);
    if (value.size() != length) {
        return false;
    }
    for (std::size_t i = 0; i < length; ++i) {
        if (std::tolower(static_cast<unsigned char>(value[i])) !=
            std::tolower(static_cast<unsigned char>(expected[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

Playback::Playback(PlaybackHost& host)
    : host_(host)
{
}

Playback::~Playback() = default;

// ---------------------------------------------------------------------------
// Package

void Playback::set_asset(std::shared_ptr<PackageAsset> asset, bool restore_state)
{
    // Remember choices by stable IDs so a reimported package can restore them.
    struct Saved {
        std::string animation_id;
        int frame = 0;
        bool playing = false;
        bool paused = false;
        bool finished = false;
        std::vector<std::pair<std::string, std::string>> variants;
    } saved;
    const bool restore = restore_state && asset_ && player_;
    if (restore) {
        const spriteloop::SplaPackage& old_package = asset_->package();
        if (const spriteloop::SplaAnimation* animation = player_->current_animation()) {
            saved.animation_id = animation->id;
        }
        saved.frame = player_->current_frame_index();
        saved.playing = playing_;
        saved.paused = paused_;
        saved.finished = finished_;
        const auto& overrides = skin_state_.variant_overrides_by_part;
        for (std::size_t part = 0; part < overrides.size() && part < old_package.parts.size(); ++part) {
            const int variant = overrides[part];
            if (variant >= 0 && variant < static_cast<int>(old_package.variants.size())) {
                saved.variants.emplace_back(old_package.parts[part].id,
                                            old_package.variants[static_cast<std::size_t>(variant)].id);
            }
        }
    }

    // Keep the previous geometry alive until the host has switched to the new
    // meshes: a 3D instance whose mesh is freed loses its base.
    const std::shared_ptr<BakedSet> previous_baked = std::move(baked_);
    player_.reset();
    backlog_.clear();
    skin_state_ = {};
    playing_ = false;
    paused_ = false;
    finished_ = false;
    ++generation_;
    asset_ = std::move(asset);

    if (!asset_) {
        host_.playback_bounds_changed();
        visual_changed();
        return;
    }

    const spriteloop::SplaPackage& package = asset_->package();
    rebuild_player();

    // Skin and variant overrides.
    int skin_index = skin_.is_empty()
        ? resolve_default_skin()
        : spriteloop::find_skin_index_by_id_or_name(package, to_std(skin_));
    if (skin_index < 0 && !skin_.is_empty()) {
        UtilityFunctions::push_warning("SpriteLoop: unknown skin '", skin_, "'.");
        skin_index = resolve_default_skin();
    }
    skin_state_.skin_index = skin_index;
    for (const auto& entry : saved.variants) {
        const int part = spriteloop::find_part_index_by_id(package, entry.first);
        const int variant = spriteloop::find_variant_index_by_id(package, entry.second);
        if (part >= 0 && variant >= 0 && package.variants[static_cast<std::size_t>(variant)].part_index == part) {
            skin_state_.variant_overrides_by_part.resize(package.parts.size(), -1);
            skin_state_.variant_overrides_by_part[static_cast<std::size_t>(part)] = variant;
        }
    }
    baked_ = asset_->baked_set(skin_state_);

    // Animation.
    int animation_index = -1;
    if (!saved.animation_id.empty()) {
        animation_index = find_animation_index(package, saved.animation_id);
    }
    const bool same_animation = animation_index >= 0;
    if (animation_index < 0 && !animation_.is_empty()) {
        animation_index = find_animation_index(package, to_std(animation_));
    }
    if (animation_index < 0 && !package.animations.empty()) {
        animation_index = 0;
    }
    if (animation_index >= 0) {
        animation_ = display_name(package.animations[static_cast<std::size_t>(animation_index)]);
        select_animation_without_events(animation_index);
        if (same_animation) {
            player_->set_frame(saved.frame, false);
            playing_ = saved.playing;
            paused_ = saved.paused;
            finished_ = saved.finished;
        } else {
            player_->set_frame(pending_frame_, false);
        }
    }
    visual_changed();
    host_.playback_bounds_changed();

    if (play_on_load_) {
        play_on_load_ = false;
        play(StringName());
    }
}

void Playback::rebuild_player()
{
    player_ = std::make_unique<spriteloop::SplaPlayer>(asset_->package());
    apply_loop_mode();
}

void Playback::apply_loop_mode()
{
    if (!player_) {
        return;
    }
    switch (loop_mode_) {
    case LoopMode::package:
        player_->clear_loop_override();
        break;
    case LoopMode::loop:
        player_->set_loop_override(true);
        break;
    case LoopMode::once:
        player_->set_loop_override(false);
        break;
    }
}

int Playback::resolve_default_skin() const
{
    // Same as the Defold component: the exported "default" skin when present.
    return spriteloop::find_skin_index_by_id_or_name(asset_->package(), "default");
}

int Playback::current_animation_index() const
{
    if (!player_ || !asset_) {
        return -1;
    }
    const spriteloop::SplaAnimation* animation = player_->current_animation();
    if (animation == nullptr) {
        return -1;
    }
    return static_cast<int>(animation - asset_->package().animations.data());
}

void Playback::stash_pending_events()
{
    if (player_ && player_->pending_event_count() > 0) {
        std::vector<spriteloop::SplaPlaybackEvent> events = player_->consume_events();
        for (auto& event : events) {
            backlog_.push_back(std::move(event));
        }
    }
}

void Playback::select_animation_without_events(int animation_index)
{
    // play() with the session's event setting, then drop the frame 0 events a
    // selection must not emit. Events already queued are kept.
    stash_pending_events();
    (void)player_->play(asset_->package().animations[static_cast<std::size_t>(animation_index)].id,
                        emit_events_);
    (void)player_->consume_events();
}

// ---------------------------------------------------------------------------
// Properties

void Playback::set_animation(const StringName& animation)
{
    if (animation == animation_ && current_animation_index() >= 0) {
        return;
    }
    if (!asset_ || !player_) {
        animation_ = animation;
        return;
    }
    const int index = find_animation_index(asset_->package(), to_std(animation));
    if (index < 0) {
        // Keep the value so it can resolve after a package change.
        animation_ = animation;
        return;
    }
    if (playing_) {
        play(animation);
        return;
    }
    animation_ = display_name(asset_->package().animations[static_cast<std::size_t>(index)]);
    select_animation_without_events(index);
    paused_ = false;
    finished_ = false;
    ++generation_;
    visual_changed();
}

void Playback::set_skin(const StringName& skin)
{
    skin_ = skin;
    if (!asset_) {
        return;
    }
    const int index = skin.is_empty()
        ? resolve_default_skin()
        : spriteloop::find_skin_index_by_id_or_name(asset_->package(), to_std(skin));
    if (index < 0 && !skin.is_empty()) {
        UtilityFunctions::push_warning("SpriteLoop: unknown skin '", skin, "'.");
        return;
    }
    skin_state_.skin_index = index;
    apply_skin_state();
}

void Playback::set_frame(int frame)
{
    if (!player_ || current_animation_index() < 0) {
        pending_frame_ = std::max(0, frame);
        return;
    }
    seek_frame(frame, false);
}

int Playback::get_frame() const
{
    if (!player_ || current_animation_index() < 0) {
        return pending_frame_;
    }
    return player_->current_frame_index();
}

int Playback::get_frame_count() const
{
    const int index = current_animation_index();
    return index < 0 ? 0 : static_cast<int>(asset_->package().animations[static_cast<std::size_t>(index)].frames.size());
}

void Playback::set_speed_scale(double speed_scale)
{
    // The SDK player only moves forward.
    speed_scale_ = std::max(0.0, speed_scale);
}

void Playback::set_loop_mode(LoopMode mode)
{
    loop_mode_ = mode;
    apply_loop_mode();
}

// ---------------------------------------------------------------------------
// Methods

bool Playback::play(const StringName& animation)
{
    if (!asset_ || !player_) {
        if (!animation.is_empty()) {
            animation_ = animation;
        }
        play_on_load_ = true;
        return false;
    }
    const String target = animation.is_empty() ? String(animation_) : String(animation);
    const int index = find_animation_index(asset_->package(), to_std(target));
    if (index < 0) {
        UtilityFunctions::push_warning("SpriteLoop: unknown animation '", target, "'.");
        return false;
    }

    // Like AnimatedSprite2D: the current animation continues or resumes; a
    // finished, stopped or different animation starts from frame 0.
    if (index == current_animation_index() && (playing_ || paused_) && !finished_) {
        playing_ = true;
        paused_ = false;
        return true;
    }

    stash_pending_events();
    const spriteloop::SplaAnimation& entry = asset_->package().animations[static_cast<std::size_t>(index)];
    (void)player_->play(entry.id, emit_events_);
    animation_ = display_name(entry);
    playing_ = true;
    paused_ = false;
    finished_ = false;
    ++generation_;
    visual_changed();
    return true;
}

void Playback::pause()
{
    if (playing_) {
        playing_ = false;
        paused_ = true;
    }
}

void Playback::stop()
{
    playing_ = false;
    paused_ = false;
    finished_ = false;
    ++generation_;
    if (player_ && current_animation_index() >= 0) {
        const int previous = player_->current_frame_index();
        stash_pending_events();
        player_->set_frame(0, false);
        if (previous != 0) {
            visual_changed();
        }
    }
}

bool Playback::seek(double seconds, bool emit_events)
{
    if (!player_ || current_animation_index() < 0) {
        return false;
    }
    const int previous = player_->current_frame_index();
    stash_pending_events();
    player_->set_time(static_cast<float>(std::max(0.0, seconds)), emit_events && emit_events_);
    finished_ = false;
    if (player_->current_frame_index() != previous) {
        visual_changed();
        emit("frame_changed");
    }
    return true;
}

bool Playback::seek_frame(int frame, bool emit_events)
{
    if (!player_ || current_animation_index() < 0) {
        return false;
    }
    const int previous = player_->current_frame_index();
    stash_pending_events();
    player_->set_frame(frame, emit_events && emit_events_);
    finished_ = false;
    if (player_->current_frame_index() != previous) {
        visual_changed();
        emit("frame_changed");
    }
    return true;
}

double Playback::get_playback_position() const
{
    return player_ ? static_cast<double>(player_->time()) : 0.0;
}

void Playback::advance(double delta)
{
    if (!player_) {
        return;
    }
    // Events queued by play()/seek() since the last update come first.
    const std::uint64_t generation = generation_;
    dispatch_events();
    if (generation != generation_ || !playing_) {
        return;
    }
    const spriteloop::SplaAnimation* animation = player_->current_animation();
    const double scaled = delta * speed_scale_;
    if (animation == nullptr || scaled <= 0.0) {
        return;
    }

    const int previous_frame = player_->current_frame_index();
    const double previous_time = player_->time();
    player_->update(static_cast<float>(scaled));

    bool looped = false;
    bool finished = false;
    const int frame_count = static_cast<int>(animation->frames.size());
    if (animation->fps > 0.0f && frame_count > 0) {
        const double previous_position = previous_time * animation->fps;
        const double position = static_cast<double>(player_->time()) * animation->fps;
        if (player_->effective_loop()) {
            looped = std::floor(position / frame_count) > std::floor(previous_position / frame_count);
        } else {
            finished = position >= frame_count;
        }
    }
    if (finished) {
        playing_ = false;
        finished_ = true;
    }

    const bool frame_changed = player_->current_frame_index() != previous_frame;
    if (frame_changed) {
        visual_changed();
    }

    // Signal handlers may call play()/stop(); stop emitting stale signals then.
    dispatch_events();
    if (generation != generation_) {
        return;
    }
    if (frame_changed) {
        emit("frame_changed");
        if (generation != generation_) {
            return;
        }
    }
    if (looped) {
        emit("animation_looped");
    } else if (finished) {
        emit("animation_finished");
    }
}

void Playback::dispatch_events()
{
    stash_pending_events();
    if (backlog_.empty()) {
        return;
    }
    std::vector<spriteloop::SplaPlaybackEvent> events;
    events.swap(backlog_);
    if (!emit_events_) {
        return;
    }
    Object* owner = host_.playback_owner();
    const StringName signal("event_triggered");
    for (const spriteloop::SplaPlaybackEvent& event : events) {
        const std::string& animation = event.animation_name.empty() ? event.animation_id
                                                                    : event.animation_name;
        owner->emit_signal(signal, StringName(to_godot(event.name)), to_godot(event.data),
                           StringName(to_godot(animation)), event.frame_index, event.source_frame);
    }
}

void Playback::emit(const char* signal)
{
    host_.playback_owner()->emit_signal(StringName(signal));
}

void Playback::visual_changed()
{
    host_.playback_visual_changed();
}

// ---------------------------------------------------------------------------
// Skins and variants

void Playback::apply_skin_state()
{
    if (!asset_) {
        return;
    }
    std::shared_ptr<BakedSet> next = asset_->baked_set(skin_state_);
    if (next == baked_) {
        return;
    }
    // Switch meshes before the previous set (and its meshes) is released.
    const std::shared_ptr<BakedSet> previous = std::move(baked_);
    baked_ = std::move(next);
    visual_changed();
    host_.playback_bounds_changed();
}

bool Playback::set_variant(const String& part, const String& variant)
{
    if (!asset_) {
        return false;
    }
    const spriteloop::SplaPackage& package = asset_->package();
    const std::string part_key = to_std(part);
    const std::string variant_key = to_std(variant);
    const int part_index = spriteloop::find_part_index_by_id_key_or_name(package, part_key);
    if (part_index < 0) {
        UtilityFunctions::push_warning("SpriteLoop: unknown part '", part, "'.");
        return false;
    }
    const int variant_index =
        spriteloop::find_variant_index_by_id_key_or_name_for_part(package, part_index, variant_key);
    if (variant_index < 0) {
        // "default" clears the override unless a real variant uses that name.
        if (equals_ignore_case(variant_key, "default")) {
            return clear_variant(part);
        }
        UtilityFunctions::push_warning("SpriteLoop: unknown variant '", variant, "' for part '", part, "'.");
        return false;
    }
    skin_state_.variant_overrides_by_part.resize(package.parts.size(), -1);
    skin_state_.variant_overrides_by_part[static_cast<std::size_t>(part_index)] = variant_index;
    apply_skin_state();
    return true;
}

bool Playback::clear_variant(const String& part)
{
    if (!asset_) {
        return false;
    }
    const spriteloop::SplaPackage& package = asset_->package();
    const int part_index = spriteloop::find_part_index_by_id_key_or_name(package, to_std(part));
    if (part_index < 0) {
        UtilityFunctions::push_warning("SpriteLoop: unknown part '", part, "'.");
        return false;
    }
    skin_state_.variant_overrides_by_part.resize(package.parts.size(), -1);
    skin_state_.variant_overrides_by_part[static_cast<std::size_t>(part_index)] = -1;
    apply_skin_state();
    return true;
}

void Playback::clear_variants()
{
    skin_state_.variant_overrides_by_part.clear();
    apply_skin_state();
}

// ---------------------------------------------------------------------------
// Part transforms

bool Playback::get_part_transform_y_up(const String& part, bool center, Transform2D& result,
                                       bool warn) const
{
    if (!asset_ || !player_) {
        return false;
    }
    const spriteloop::SplaPackage& package = asset_->package();
    const int part_index = spriteloop::find_part_index_by_id_key_or_name(package, to_std(part));
    if (part_index < 0) {
        if (warn) {
            UtilityFunctions::push_warning("SpriteLoop: unknown part '", part, "'.");
        }
        return false;
    }
    const spriteloop::SplaFrame* frame = player_->current_frame();
    const spriteloop::SplaFramePart* frame_part = nullptr;
    if (frame != nullptr) {
        for (const spriteloop::SplaFramePart& candidate : frame->parts) {
            if (candidate.part_index == part_index) {
                frame_part = &candidate;
                break;
            }
        }
    }
    if (frame_part == nullptr) {
        if (warn) {
            UtilityFunctions::push_warning("SpriteLoop: part '", part, "' is not in the current frame.");
        }
        return false;
    }

    // Same composition as the SDK baker, in centered Y-up space:
    // point = R(-rotation) * K(tan(-skew_x), tan(-skew_y)) * S * local + origin.
    const spriteloop::SplaPart& entry = package.parts[static_cast<std::size_t>(part_index)];
    const spriteloop::SplaTransform& transform = frame_part->transform;
    const double radians = -transform.rotation_degrees * pi / 180.0;
    const double cos_r = std::cos(radians);
    const double sin_r = std::sin(radians);
    const double kx = std::tan(-transform.skew_x * pi / 180.0);
    const double ky = std::tan(-transform.skew_y * pi / 180.0);
    const double sx = transform.scale_x;
    const double sy = transform.scale_y;
    const Vector2 column_x(static_cast<real_t>(sx * cos_r - ky * sx * sin_r),
                           static_cast<real_t>(sx * sin_r + ky * sx * cos_r));
    const Vector2 column_y(static_cast<real_t>(kx * sy * cos_r - sy * sin_r),
                           static_cast<real_t>(kx * sy * sin_r + sy * cos_r));
    Vector2 origin(transform.x - static_cast<float>(package.canvas_width) * 0.5f,
                   static_cast<float>(package.canvas_height) * 0.5f - transform.y);
    if (center && !entry.transform_only) {
        const Vector2 local(static_cast<real_t>(entry.width) * 0.5f - entry.pivot.x,
                            entry.pivot.y - static_cast<real_t>(entry.height) * 0.5f);
        origin += column_x * local.x + column_y * local.y;
    }
    result = Transform2D(column_x, column_y, origin);
    return true;
}

bool Playback::get_part_transform_2d(const String& part, bool center, Transform2D& result,
                                     bool warn) const
{
    Transform2D y_up;
    if (!get_part_transform_y_up(part, center, y_up, warn)) {
        return false;
    }
    // Conjugate with the Y flip: basis F*M*F, origin F*o.
    result = Transform2D(Vector2(y_up.columns[0].x, -y_up.columns[0].y),
                         Vector2(-y_up.columns[1].x, y_up.columns[1].y),
                         Vector2(y_up.columns[2].x, -y_up.columns[2].y));
    return true;
}

// ---------------------------------------------------------------------------
// Rendering

RID Playback::current_mesh_2d()
{
    if (!baked_ || !player_) {
        return RID();
    }
    return baked_->mesh_2d(current_animation_index(), player_->current_frame_index());
}

RID Playback::current_mesh_3d(float pixel_size)
{
    if (!baked_ || !player_) {
        return RID();
    }
    return baked_->mesh_3d(current_animation_index(), player_->current_frame_index(), pixel_size);
}

RID Playback::texture_rid() const
{
    return asset_ ? asset_->texture_rid() : RID();
}

Rect2 Playback::bounds_2d() const
{
    return baked_ ? baked_->bounds_2d() : Rect2();
}

const spriteloop::SplaBounds* Playback::bounds() const
{
    return baked_ ? &baked_->bounds : nullptr;
}

} // namespace spriteloop_godot

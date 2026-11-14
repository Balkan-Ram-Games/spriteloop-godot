// Definitions for playback_api_decl.inc. Included in the node's .cpp with
// SPRITELOOP_NODE defined as the class name.

void SPRITELOOP_NODE::_bind_playback_api()
{
    ClassDB::bind_method(D_METHOD("set_data", "data"), &SPRITELOOP_NODE::set_data);
    ClassDB::bind_method(D_METHOD("get_data"), &SPRITELOOP_NODE::get_data);
    ClassDB::bind_method(D_METHOD("set_animation", "animation"), &SPRITELOOP_NODE::set_animation);
    ClassDB::bind_method(D_METHOD("get_animation"), &SPRITELOOP_NODE::get_animation);
    ClassDB::bind_method(D_METHOD("set_skin", "skin"), &SPRITELOOP_NODE::set_skin);
    ClassDB::bind_method(D_METHOD("get_skin"), &SPRITELOOP_NODE::get_skin);
    ClassDB::bind_method(D_METHOD("set_autoplay", "animation"), &SPRITELOOP_NODE::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &SPRITELOOP_NODE::get_autoplay);
    ClassDB::bind_method(D_METHOD("set_frame", "frame"), &SPRITELOOP_NODE::set_frame);
    ClassDB::bind_method(D_METHOD("get_frame"), &SPRITELOOP_NODE::get_frame);
    ClassDB::bind_method(D_METHOD("get_frame_count"), &SPRITELOOP_NODE::get_frame_count);
    ClassDB::bind_method(D_METHOD("set_frame_progress", "progress"), &SPRITELOOP_NODE::set_frame_progress);
    ClassDB::bind_method(D_METHOD("get_frame_progress"), &SPRITELOOP_NODE::get_frame_progress);
    ClassDB::bind_method(D_METHOD("set_frame_and_progress", "frame", "progress"),
                         &SPRITELOOP_NODE::set_frame_and_progress);
    ClassDB::bind_method(D_METHOD("set_speed_scale", "speed_scale"), &SPRITELOOP_NODE::set_speed_scale);
    ClassDB::bind_method(D_METHOD("get_speed_scale"), &SPRITELOOP_NODE::get_speed_scale);
    ClassDB::bind_method(D_METHOD("set_loop_mode", "mode"), &SPRITELOOP_NODE::set_loop_mode);
    ClassDB::bind_method(D_METHOD("get_loop_mode"), &SPRITELOOP_NODE::get_loop_mode);
    ClassDB::bind_method(D_METHOD("set_emit_events", "enabled"), &SPRITELOOP_NODE::set_emit_events);
    ClassDB::bind_method(D_METHOD("is_emitting_events"), &SPRITELOOP_NODE::is_emitting_events);
    ClassDB::bind_method(D_METHOD("set_process_callback", "callback"),
                         &SPRITELOOP_NODE::set_process_callback);
    ClassDB::bind_method(D_METHOD("get_process_callback"), &SPRITELOOP_NODE::get_process_callback);
    ClassDB::bind_method(D_METHOD("set_preview", "enabled"), &SPRITELOOP_NODE::set_preview);
    ClassDB::bind_method(D_METHOD("is_previewing"), &SPRITELOOP_NODE::is_previewing);

    ClassDB::bind_method(D_METHOD("play", "animation"), &SPRITELOOP_NODE::play, DEFVAL(StringName()));
    ClassDB::bind_method(D_METHOD("pause"), &SPRITELOOP_NODE::pause);
    ClassDB::bind_method(D_METHOD("stop"), &SPRITELOOP_NODE::stop);
    ClassDB::bind_method(D_METHOD("is_playing"), &SPRITELOOP_NODE::is_playing);
    ClassDB::bind_method(D_METHOD("seek", "seconds", "emit_events"), &SPRITELOOP_NODE::seek, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("seek_frame", "frame", "emit_events"), &SPRITELOOP_NODE::seek_frame,
                         DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_playback_position"), &SPRITELOOP_NODE::get_playback_position);
    ClassDB::bind_method(D_METHOD("advance", "delta"), &SPRITELOOP_NODE::advance);
    ClassDB::bind_method(D_METHOD("set_variant", "part", "variant"), &SPRITELOOP_NODE::set_variant);
    ClassDB::bind_method(D_METHOD("clear_variant", "part"), &SPRITELOOP_NODE::clear_variant);
    ClassDB::bind_method(D_METHOD("clear_variants"), &SPRITELOOP_NODE::clear_variants);
    ClassDB::bind_method(D_METHOD("has_part", "part"), &SPRITELOOP_NODE::has_part);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data", PROPERTY_HINT_RESOURCE_TYPE, "SpriteLoopData"),
                 "set_data", "get_data");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "animation", PROPERTY_HINT_ENUM_SUGGESTION, ""),
                 "set_animation", "get_animation");
    // Editor only and never saved: plays the animation in the editor.
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "preview", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR),
                 "set_preview", "is_previewing");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "skin", PROPERTY_HINT_ENUM_SUGGESTION, ""),
                 "set_skin", "get_skin");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "autoplay", PROPERTY_HINT_ENUM_SUGGESTION, ""),
                 "set_autoplay", "get_autoplay");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "frame", PROPERTY_HINT_RANGE, "0,0,1"), "set_frame", "get_frame");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frame_progress", PROPERTY_HINT_RANGE, "0,1,0.0001,no_slider"),
                 "set_frame_progress", "get_frame_progress");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0,8,0.01,or_greater"),
                 "set_speed_scale", "get_speed_scale");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "loop_mode", PROPERTY_HINT_ENUM, "From Package,Loop,Once"),
                 "set_loop_mode", "get_loop_mode");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "emit_events"), "set_emit_events", "is_emitting_events");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "process_callback", PROPERTY_HINT_ENUM, "Idle,Physics,Manual"),
                 "set_process_callback", "get_process_callback");

    ADD_SIGNAL(MethodInfo("animation_changed"));
    ADD_SIGNAL(MethodInfo("frame_changed"));
    ADD_SIGNAL(MethodInfo("animation_looped"));
    ADD_SIGNAL(MethodInfo("animation_finished"));
    ADD_SIGNAL(MethodInfo("event_triggered", PropertyInfo(Variant::STRING_NAME, "event_name"),
                          PropertyInfo(Variant::STRING, "data"),
                          PropertyInfo(Variant::STRING_NAME, "animation"),
                          PropertyInfo(Variant::INT, "frame"),
                          PropertyInfo(Variant::INT, "source_frame")));

    BIND_ENUM_CONSTANT(LOOP_MODE_PACKAGE);
    BIND_ENUM_CONSTANT(LOOP_MODE_LOOP);
    BIND_ENUM_CONSTANT(LOOP_MODE_ONCE);
    BIND_ENUM_CONSTANT(PROCESS_CALLBACK_IDLE);
    BIND_ENUM_CONSTANT(PROCESS_CALLBACK_PHYSICS);
    BIND_ENUM_CONSTANT(PROCESS_CALLBACK_MANUAL);
    BIND_ENUM_CONSTANT(PART_ORIGIN_PIVOT);
    BIND_ENUM_CONSTANT(PART_ORIGIN_CENTER);
}

void SPRITELOOP_NODE::_playback_notification(int what)
{
    switch (what) {
    case Node::NOTIFICATION_READY:
        if (!Engine::get_singleton()->is_editor_hint() && !autoplay_.is_empty()) {
            play(autoplay_);
        }
        _update_playback_process();
        break;
    case Node::NOTIFICATION_INTERNAL_PROCESS:
        playback_.advance(get_process_delta_time());
        _keep_preview_playing();
        break;
    case Node::NOTIFICATION_INTERNAL_PHYSICS_PROCESS:
        playback_.advance(get_physics_process_delta_time());
        break;
    default:
        break;
    }
}

void SPRITELOOP_NODE::_update_playback_process()
{
    const bool editor = Engine::get_singleton()->is_editor_hint();
    const bool previewing = _is_previewing();
    const bool active = data_.is_valid() && (!editor || previewing);
    // The editor preview always runs on idle frames, whatever the callback.
    set_process_internal(active && (previewing || process_callback_ == PROCESS_CALLBACK_IDLE));
    set_physics_process_internal(active && !previewing && process_callback_ == PROCESS_CALLBACK_PHYSICS);
}

bool SPRITELOOP_NODE::_is_previewing() const
{
    return preview_ && Engine::get_singleton()->is_editor_hint();
}

void SPRITELOOP_NODE::_keep_preview_playing()
{
    // Restart finished one-shot animations so the preview keeps playing.
    if (_is_previewing() && !playback_.is_playing()) {
        playback_.play(StringName());
    }
}

void SPRITELOOP_NODE::set_preview(bool enabled)
{
    if (enabled == preview_) {
        return;
    }
    preview_ = enabled;
    if (!Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (enabled) {
        // Previewing must not change the frame saved with the scene.
        preview_restore_frame_ = playback_.get_frame();
        playback_.play(StringName());
    } else {
        playback_.stop();
        playback_.set_frame(preview_restore_frame_);
    }
    if (is_inside_tree()) {
        _update_playback_process();
    }
}

bool SPRITELOOP_NODE::is_previewing() const
{
    return preview_;
}

void SPRITELOOP_NODE::_validate_playback_property(PropertyInfo& property) const
{
    const String name = property.name;
    if (name == "animation" || name == "autoplay" || name == "skin") {
        if (data_.is_null() || !data_->is_valid()) {
            return;
        }
        const PackedStringArray names = name == "skin" ? data_->get_skin_names() : data_->get_animation_names();
        property.hint_string = String(",").join(names);
    } else if (name == "frame") {
        property.hint_string = "0," + String::num_int64(std::max(0, get_frame_count() - 1)) + ",1";
    }
}

void SPRITELOOP_NODE::_on_data_changed()
{
    // Reimported or replaced package bytes: reload, keeping the current state.
    playback_.set_asset(data_.is_valid() ? data_->asset() : nullptr, true);
    notify_property_list_changed();
}

void SPRITELOOP_NODE::set_data(const Ref<SpriteLoopData>& data)
{
    if (data_ == data) {
        return;
    }
    const Callable on_changed = callable_mp(this, &SPRITELOOP_NODE::_on_data_changed);
    if (data_.is_valid() && data_->is_connected("changed", on_changed)) {
        data_->disconnect("changed", on_changed);
    }
    data_ = data;
    if (data_.is_valid()) {
        data_->connect("changed", on_changed);
    }
    playback_.set_asset(data_.is_valid() ? data_->asset() : nullptr, false);
    if (is_inside_tree()) {
        _update_playback_process();
    }
    notify_property_list_changed();
}

Ref<SpriteLoopData> SPRITELOOP_NODE::get_data() const
{
    return data_;
}

void SPRITELOOP_NODE::set_animation(const StringName& animation)
{
    playback_.set_animation(animation);
    notify_property_list_changed();
}

StringName SPRITELOOP_NODE::get_animation() const
{
    return playback_.get_animation();
}

void SPRITELOOP_NODE::set_skin(const StringName& skin)
{
    playback_.set_skin(skin);
}

StringName SPRITELOOP_NODE::get_skin() const
{
    return playback_.get_skin();
}

void SPRITELOOP_NODE::set_autoplay(const StringName& animation)
{
    autoplay_ = animation;
}

StringName SPRITELOOP_NODE::get_autoplay() const
{
    return autoplay_;
}

void SPRITELOOP_NODE::set_frame(int frame)
{
    playback_.set_frame(frame);
}

int SPRITELOOP_NODE::get_frame() const
{
    return playback_.get_frame();
}

int SPRITELOOP_NODE::get_frame_count() const
{
    return playback_.get_frame_count();
}

void SPRITELOOP_NODE::set_frame_progress(double progress)
{
    playback_.set_frame_progress(progress);
}

double SPRITELOOP_NODE::get_frame_progress() const
{
    return playback_.get_frame_progress();
}

bool SPRITELOOP_NODE::set_frame_and_progress(int frame, double progress)
{
    return playback_.set_frame_and_progress(frame, progress);
}

void SPRITELOOP_NODE::set_speed_scale(double speed_scale)
{
    playback_.set_speed_scale(speed_scale);
}

double SPRITELOOP_NODE::get_speed_scale() const
{
    return playback_.get_speed_scale();
}

void SPRITELOOP_NODE::set_loop_mode(LoopMode mode)
{
    playback_.set_loop_mode(static_cast<spriteloop_godot::LoopMode>(mode));
}

SPRITELOOP_NODE::LoopMode SPRITELOOP_NODE::get_loop_mode() const
{
    return static_cast<LoopMode>(playback_.get_loop_mode());
}

void SPRITELOOP_NODE::set_emit_events(bool enabled)
{
    playback_.set_emit_events(enabled);
}

bool SPRITELOOP_NODE::is_emitting_events() const
{
    return playback_.get_emit_events();
}

void SPRITELOOP_NODE::set_process_callback(ProcessCallback callback)
{
    process_callback_ = callback;
    if (is_inside_tree()) {
        _update_playback_process();
    }
}

SPRITELOOP_NODE::ProcessCallback SPRITELOOP_NODE::get_process_callback() const
{
    return process_callback_;
}

bool SPRITELOOP_NODE::play(const StringName& animation)
{
    const bool started = playback_.play(animation);
    notify_property_list_changed();
    return started;
}

void SPRITELOOP_NODE::pause()
{
    playback_.pause();
}

void SPRITELOOP_NODE::stop()
{
    playback_.stop();
}

bool SPRITELOOP_NODE::is_playing() const
{
    return playback_.is_playing();
}

bool SPRITELOOP_NODE::seek(double seconds, bool emit_events)
{
    return playback_.seek(seconds, emit_events);
}

bool SPRITELOOP_NODE::seek_frame(int frame, bool emit_events)
{
    return playback_.seek_frame(frame, emit_events);
}

double SPRITELOOP_NODE::get_playback_position() const
{
    return playback_.get_playback_position();
}

void SPRITELOOP_NODE::advance(double delta)
{
    playback_.advance(delta);
}

bool SPRITELOOP_NODE::set_variant(const String& part, const String& variant)
{
    return playback_.set_variant(part, variant);
}

bool SPRITELOOP_NODE::clear_variant(const String& part)
{
    return playback_.clear_variant(part);
}

void SPRITELOOP_NODE::clear_variants()
{
    playback_.clear_variants();
}

bool SPRITELOOP_NODE::has_part(const String& part) const
{
    Transform2D unused;
    return playback_.get_part_transform_y_up(part, false, unused, false);
}

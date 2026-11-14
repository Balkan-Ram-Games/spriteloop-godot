// Shared playback API for SpriteLoop2D and SpriteLoop3D.
//
// Included inside the class body of each node (after GDCLASS) with
// SPRITELOOP_NODE defined as the class name. Definitions and bindings are in
// playback_api_impl.inc. GDExtension binds methods per concrete class, so the
// API is declared on each node and forwards to spriteloop_godot::Playback.
//
// The node class must derive from spriteloop_godot::PlaybackHost.

public:
    enum LoopMode {
        LOOP_MODE_PACKAGE,
        LOOP_MODE_LOOP,
        LOOP_MODE_ONCE,
    };

    enum ProcessCallback {
        PROCESS_CALLBACK_IDLE,
        PROCESS_CALLBACK_PHYSICS,
        PROCESS_CALLBACK_MANUAL,
    };

    enum PartOrigin {
        PART_ORIGIN_PIVOT,
        PART_ORIGIN_CENTER,
    };

    void set_data(const Ref<SpriteLoopData>& data);
    Ref<SpriteLoopData> get_data() const;
    void set_animation(const StringName& animation);
    StringName get_animation() const;
    void set_skin(const StringName& skin);
    StringName get_skin() const;
    void set_autoplay(const StringName& animation);
    StringName get_autoplay() const;
    void set_frame(int frame);
    int get_frame() const;
    int get_frame_count() const;
    void set_frame_progress(double progress);
    double get_frame_progress() const;
    bool set_frame_and_progress(int frame, double progress);
    void set_speed_scale(double speed_scale);
    double get_speed_scale() const;
    void set_loop_mode(LoopMode mode);
    LoopMode get_loop_mode() const;
    void set_emit_events(bool enabled);
    bool is_emitting_events() const;
    void set_process_callback(ProcessCallback callback);
    ProcessCallback get_process_callback() const;
    // Editor only, not saved: plays the animation in the editor.
    void set_preview(bool enabled);
    bool is_previewing() const;

    bool play(const StringName& animation = StringName());
    void pause();
    void stop();
    bool is_playing() const;
    bool seek(double seconds, bool emit_events = true);
    bool seek_frame(int frame, bool emit_events = true);
    double get_playback_position() const;
    void advance(double delta);

    bool set_variant(const String& part, const String& variant);
    bool clear_variant(const String& part);
    void clear_variants();
    bool has_part(const String& part) const;

protected:
    static void _bind_playback_api();
    void _playback_notification(int what);
    void _validate_playback_property(PropertyInfo& property) const;
    void _update_playback_process();
    bool _is_previewing() const;
    void _keep_preview_playing();
    void _on_data_changed();

    spriteloop_godot::Playback playback_{*this};
    Ref<SpriteLoopData> data_;
    StringName autoplay_;
    ProcessCallback process_callback_ = PROCESS_CALLBACK_IDLE;
    bool preview_ = false;
    int preview_restore_frame_ = 0;

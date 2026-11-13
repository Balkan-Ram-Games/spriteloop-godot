#pragma once

#include "core/material_3d.h"
#include "core/playback.h"
#include "spriteloop_data.h"

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {

// Plays a SpriteLoop package in 3D, as a flat character in the node's XY
// plane facing +Z (like Sprite3D).
//
// Frames are shared meshes in world units (pixels * pixel_size); switching
// frames swaps the mesh. The generated material is shared by nodes with the
// same look (options, modulate, flips). A material_override replaces it.
class SpriteLoop3D : public GeometryInstance3D, public spriteloop_godot::PlaybackHost {
    GDCLASS(SpriteLoop3D, GeometryInstance3D)

#define SPRITELOOP_NODE SpriteLoop3D
#include "nodes/playback_api_decl.inc"
#undef SPRITELOOP_NODE

public:
    enum BillboardMode {
        BILLBOARD_DISABLED,
        BILLBOARD_ENABLED,
        BILLBOARD_FIXED_Y,
    };

    enum AlphaCutMode {
        ALPHA_CUT_DISABLED,
        ALPHA_CUT_DISCARD,
        ALPHA_CUT_OPAQUE_PREPASS,
        ALPHA_CUT_HASH,
    };

    enum TextureFilter {
        TEXTURE_FILTER_NEAREST,
        TEXTURE_FILTER_LINEAR,
    };

    SpriteLoop3D();
    ~SpriteLoop3D() override;

    void set_pixel_size(float pixel_size);
    float get_pixel_size() const;
    void set_modulate(const Color& modulate);
    Color get_modulate() const;
    void set_flip_h(bool flip);
    bool is_flipped_h() const;
    void set_flip_v(bool flip);
    bool is_flipped_v() const;
    void set_billboard_mode(BillboardMode mode);
    BillboardMode get_billboard_mode() const;
    void set_shaded(bool shaded);
    bool is_shaded() const;
    void set_double_sided(bool double_sided);
    bool is_double_sided() const;
    void set_no_depth_test(bool enabled);
    bool is_no_depth_test() const;
    void set_alpha_cut_mode(AlphaCutMode mode);
    AlphaCutMode get_alpha_cut_mode() const;
    void set_alpha_scissor_threshold(float threshold);
    float get_alpha_scissor_threshold() const;
    void set_texture_filter(TextureFilter filter);
    TextureFilter get_texture_filter() const;
    void set_render_priority(int priority);
    int get_render_priority() const;
    // Custom look: a ShaderMaterial whose shader includes
    // spriteloop_3d.gdshaderinc. The node renders a copy of it with the
    // SpriteLoop uniforms (atlas, flips, billboard, modulate, filter) filled in.
    void set_material(const Ref<ShaderMaterial>& material);
    Ref<ShaderMaterial> get_material() const;

    PackedStringArray _get_configuration_warnings() const override;

    // Current transform of a part in this node's local space (world units, Y
    // up, in the XY plane). Billboarding is not applied.
    Transform3D get_part_transform(const String& part, PartOrigin origin = PART_ORIGIN_PIVOT) const;

    AABB _get_aabb() const override;

    // Bounds of every frame in the node's XY plane (world units, flips
    // applied). Used for editor picking.
    Rect2 get_local_rect() const;

    // PlaybackHost
    void playback_visual_changed() override;
    void playback_bounds_changed() override;
    Object* playback_owner() override { return this; }

protected:
    static void _bind_methods();
    void _notification(int what);
    void _validate_property(PropertyInfo& property) const;

private:
    void update_mesh();
    void update_material();
    void apply_material();
    void on_custom_material_changed();
    AABB bounds_aabb() const;

    float pixel_size_ = 0.01f;
    spriteloop_godot::Material3DOptions options_;
    Ref<ShaderMaterial> material_;
    RID current_mesh_;
};

} // namespace godot

VARIANT_ENUM_CAST(SpriteLoop3D::LoopMode);
VARIANT_ENUM_CAST(SpriteLoop3D::ProcessCallback);
VARIANT_ENUM_CAST(SpriteLoop3D::PartOrigin);
VARIANT_ENUM_CAST(SpriteLoop3D::BillboardMode);
VARIANT_ENUM_CAST(SpriteLoop3D::AlphaCutMode);
VARIANT_ENUM_CAST(SpriteLoop3D::TextureFilter);

#pragma once

#include "core/mask_cache.h"

#include <godot_cpp/classes/texture2d.hpp>

#include <memory>

namespace godot {

// A composed mask frame as a Texture2D, so SpriteLoop3D materials can use it
// like the package atlas. Keeps the frame alive while a material uses it.
// Internal: created by MaskFrame::texture_resource().
class SpriteLoopMaskTexture : public Texture2D {
    GDCLASS(SpriteLoopMaskTexture, Texture2D)

public:
    void set_frame(const std::shared_ptr<spriteloop_godot::MaskFrame>& frame) { frame_ = frame; }

    RID _get_rid() const override;
    int32_t _get_width() const override;
    int32_t _get_height() const override;
    bool _has_alpha() const override { return true; }

protected:
    static void _bind_methods() {}

private:
    std::shared_ptr<spriteloop_godot::MaskFrame> frame_;
};

} // namespace godot

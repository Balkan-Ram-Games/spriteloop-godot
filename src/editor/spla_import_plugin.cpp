#include "editor/spla_import_plugin.h"

#include "core/names.h"
#include "spriteloop_data.h"

#include "spriteloop/spla_loader.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

String SplaImportPlugin::_get_importer_name() const
{
    return "spriteloop.spla";
}

String SplaImportPlugin::_get_visible_name() const
{
    return "SpriteLoop Package";
}

PackedStringArray SplaImportPlugin::_get_recognized_extensions() const
{
    PackedStringArray extensions;
    extensions.push_back("spla");
    return extensions;
}

String SplaImportPlugin::_get_save_extension() const
{
    return "res";
}

String SplaImportPlugin::_get_resource_type() const
{
    return "SpriteLoopData";
}

float SplaImportPlugin::_get_priority() const
{
    return 2.0f;
}

// Bump to reimport existing .spla files after an importer change.
int32_t SplaImportPlugin::_get_format_version() const
{
    return 2;
}

int32_t SplaImportPlugin::_get_import_order() const
{
    return 0;
}

int32_t SplaImportPlugin::_get_preset_count() const
{
    return 2;
}

String SplaImportPlugin::_get_preset_name(int32_t) const
{
    return "Default";
}

TypedArray<Dictionary> SplaImportPlugin::_get_import_options(const String&, int32_t) const
{
    // Mipmaps: for characters drawn much smaller than the art (distant 3D
    // characters, zoomed-out 2D). Costs about a third more texture memory.
    Dictionary mipmaps;
    mipmaps["name"] = "mipmaps";
    mipmaps["default_value"] = false;
    TypedArray<Dictionary> options;
    options.push_back(mipmaps);
    return options;
}

bool SplaImportPlugin::_get_option_visibility(const String&, const StringName&, const Dictionary&) const
{
    return true;
}

Error SplaImportPlugin::_import(const String& source_file, const String& save_path, const Dictionary& options,
                                const TypedArray<String>&, const TypedArray<String>&) const
{
    const PackedByteArray bytes = FileAccess::get_file_as_bytes(source_file);
    if (bytes.is_empty()) {
        UtilityFunctions::push_error("SpriteLoop: cannot read '", source_file, "'.");
        return ERR_FILE_CANT_READ;
    }

    // Validate the manifest now so broken exports fail at import time with a
    // clear message instead of when a scene first uses them.
    auto package = spriteloop::load_package_from_memory(bytes.ptr(), static_cast<std::size_t>(bytes.size()),
                                                        spriteloop_godot::to_std(source_file));
    if (!package) {
        UtilityFunctions::push_error("SpriteLoop: invalid package '", source_file,
                                     "': ", spriteloop_godot::to_godot(package.error().message));
        return ERR_FILE_CORRUPT;
    }

    Ref<SpriteLoopData> data;
    data.instantiate();
    data->set_mipmaps(options.get("mipmaps", false));
    data->set_package_bytes(bytes);
    // .spla packages are already ZIP-compressed.
    return ResourceSaver::get_singleton()->save(data, save_path + String(".") + _get_save_extension());
}

} // namespace godot

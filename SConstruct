#!/usr/bin/env python
# Builds the SpriteLoop GDExtension into addons/spriteloop/bin/.
#
#   scons platform=windows target=template_debug
#   scons platform=windows target=template_release
#
# The private SpriteLoop SDK is linked from prebuilt static libraries in
# sdk/lib/<platform>.<arch>[.nothreads]/ with headers in sdk/include/.
import os

env = Environment(tools=["default"], PLATFORM="")
# The SDK is compiled with C++ exceptions enabled and exchanges STL types with
# this library. Keep exception handling on so both sides use the same STL
# configuration (on MSVC godot-cpp otherwise defines _HAS_EXCEPTIONS=0).
env["disable_exceptions"] = False
env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": []})

# Optional object cache (CI keeps it between runs, so godot-cpp is compiled
# once): set SCONS_CACHE to a folder.
if os.environ.get("SCONS_CACHE"):
    CacheDir(os.environ["SCONS_CACHE"])

if env["platform"] == "web":
    # SCons runs tools with a minimal environment; emcc needs the emsdk
    # variables (EMSDK_PYTHON, PATH) from emsdk_env.
    for name, value in os.environ.items():
        if name == "PATH" or name.startswith("EM"):
            env["ENV"][name] = value

sdk_platform = "{}.{}".format(env["platform"], env["arch"])
if env["platform"] == "web" and not env.get("threads", True):
    sdk_platform += ".nothreads"
sdk_lib_dir = os.path.join("sdk", "lib", sdk_platform)
if not os.path.isdir(sdk_lib_dir):
    print("ERROR: missing SpriteLoop SDK library folder: " + sdk_lib_dir)
    Exit(1)

env.Append(CPPPATH=["src", "sdk/include"])
env.Append(LIBPATH=[sdk_lib_dir])
env.Append(LIBS=["spla_core"])
if env["platform"] == "android":
    # The NDK's clang++ links libc++_shared.so by default, which Godot does not
    # package. Link the C++ runtime statically, as the SDK library does.
    env.Append(LINKFLAGS=["-static-libstdc++"])


# The SpriteLoop3D built-in shader is a real shader file in the addon, with its
# reusable core in a shader include that custom shaders can #include too. The
# build embeds the shader, with the include inlined, so the built-in material
# does not depend on these files being exported with a game.
SHADER_INCLUDE_LINE = '#include "res://addons/spriteloop/shaders/spriteloop_3d.gdshaderinc"'


def embed_shader(target, source, env):
    shader = source[0].get_text_contents()
    include = source[1].get_text_contents()
    if shader.count(SHADER_INCLUDE_LINE) != 1:
        raise RuntimeError("spriteloop_3d.gdshader must include spriteloop_3d.gdshaderinc exactly once")
    text = shader.replace(SHADER_INCLUDE_LINE, include)
    delimiter = "SL_SHADER"
    if (")" + delimiter + '"') in text:
        raise RuntimeError("shader text contains the raw string delimiter")
    with open(str(target[0]), "w", newline="\n") as header:
        header.write("// Generated from {} and {} by SConstruct. Do not edit.\n".format(source[0], source[1]))
        header.write("#pragma once\n\n")
        header.write('static const char* const SPRITELOOP_3D_SHADER = R"{0}({1}){0}";\n'.format(delimiter, text))


shader_header = env.Command(
    "src/gen/spriteloop_3d_shader.gen.h",
    ["addons/spriteloop/shaders/spriteloop_3d.gdshader", "addons/spriteloop/shaders/spriteloop_3d.gdshaderinc"],
    Action(embed_shader, "Embedding $SOURCES"),
)

sources = Glob("src/*.cpp") + Glob("src/core/*.cpp") + Glob("src/nodes/*.cpp")
if env["target"] in ["editor", "template_debug"]:
    # Editor-only classes (import plugin, editor plugin) are compiled into the
    # debug library, which is the one the editor loads.
    sources += Glob("src/editor/*.cpp")
    env.Append(CPPDEFINES=["SPRITELOOP_EDITOR"])

bin_dir = "addons/spriteloop/bin"
library = env.SharedLibrary(
    "{}/libspriteloop{}{}".format(bin_dir, env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)
# material_3d.cpp includes the header; SCons' include scanner orders it first.
env.Depends(library, shader_header)

Default(library)

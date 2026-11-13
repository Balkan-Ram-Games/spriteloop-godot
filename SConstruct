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

Default(library)

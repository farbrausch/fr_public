#!/usr/bin/env python3
"""
Native macOS Build Orchestrator for Werkkzeug 4 (Mach-O 64-bit ARM64)
Zero-Entropy Protocol (ZEP) Compliant

Builds two executables out of one shared pile of objects:

  werkkzeug4  the editor
  wz4player   the standalone demo player -- takes a .wz4 on the command line

  ./scripts/build_werkkzeug4_macos.py [werkkzeug4|wz4player|all]
"""

import os
import sys
import glob
import subprocess
import concurrent.futures

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BUILD_DIR = os.path.join(ROOT_DIR, "build", "werkkzeug4_macos")
BIN_DIR = os.path.join(ROOT_DIR, "bin", "macos")

os.makedirs(BUILD_DIR, exist_ok=True)
os.makedirs(BIN_DIR, exist_ok=True)

# 1. Base files
base_srcs = [
    "altona_wz4/altona/main/base/types.cpp",
    "altona_wz4/altona/main/base/types2.cpp",
    "altona_wz4/altona/main/base/system.cpp",
    "altona_wz4/altona/main/base/system_linux.cpp",
    "altona_wz4/altona/main/base/system_macos.mm",
    "altona_wz4/altona/main/base/graphics.cpp",
    "altona_wz4/altona/main/base/graphics_ogl2.cpp",
    "altona_wz4/altona/main/base/shader_glsl_macos.cpp",
    "altona_wz4/altona/main/base/math.cpp",
    "altona_wz4/altona/main/base/serialize.cpp",
    "altona_wz4/altona/main/base/input2.cpp",
    "altona_wz4/altona/main/base/sound.cpp",
]

# 2. Util files
util_srcs = [
    "altona_wz4/altona/main/util/algorithms.cpp",
    "altona_wz4/altona/main/util/animation.cpp",
    "altona_wz4/altona/main/util/bitio.cpp",
    "altona_wz4/altona/main/util/camera.cpp",
    "altona_wz4/altona/main/util/effect.cpp",
    "altona_wz4/altona/main/util/fastcompress.cpp",
    "altona_wz4/altona/main/util/image.cpp",
    "altona_wz4/altona/main/util/ipp.cpp",
    "altona_wz4/altona/main/util/painter.cpp",
    "altona_wz4/altona/main/util/perfmon.cpp",
    "altona_wz4/altona/main/util/rasterizer.cpp",
    "altona_wz4/altona/main/util/scanconfig.cpp",
    "altona_wz4/altona/main/util/scanner.cpp",
    "altona_wz4/altona/main/util/shaders.cpp",
    "altona_wz4/altona/main/util/taskscheduler.cpp",
    "altona_wz4/altona/main/util/musicplayer.cpp",
    "altona_wz4/altona/main/util/stb_image.c",
]

# 3. GUI files
gui_srcs = [
    "altona_wz4/altona/main/gui/3dwindow.cpp",
    "altona_wz4/altona/main/gui/borders.cpp",
    "altona_wz4/altona/main/gui/color.cpp",
    "altona_wz4/altona/main/gui/controls.cpp",
    "altona_wz4/altona/main/gui/dialog.cpp",
    "altona_wz4/altona/main/gui/frames.cpp",
    "altona_wz4/altona/main/gui/gui.cpp",
    "altona_wz4/altona/main/gui/listwindow.cpp",
    "altona_wz4/altona/main/gui/manager.cpp",
    "altona_wz4/altona/main/gui/overlapped.cpp",
    "altona_wz4/altona/main/gui/tabs.cpp",
    "altona_wz4/altona/main/gui/textwindow.cpp",
    "altona_wz4/altona/main/gui/timeline.cpp",
    "altona_wz4/altona/main/gui/window.cpp",
    "altona_wz4/altona/main/gui/wire.cpp",
]

# 4. Shader compiler files
shadercomp_srcs = [
    "altona_wz4/altona/main/shadercomp/asc_doc.cpp",
    "altona_wz4/altona/main/shadercomp/asc_parser.cpp",
    "altona_wz4/altona/main/shadercomp/asc_output.cpp",
    "altona_wz4/altona/main/shadercomp/compile_cg.cpp",
    "altona_wz4/altona/main/shadercomp/compile_dx.cpp",
    "altona_wz4/altona/main/shadercomp/shaderdis.cpp",
]

# 5. Wz4Lib files
wz4lib_srcs = [
    "altona_wz4/altona/main/wz4lib/basic.cpp",
    "altona_wz4/altona/main/wz4lib/basic_ops.cpp",
    "altona_wz4/altona/main/wz4lib/build.cpp",
    "altona_wz4/altona/main/wz4lib/doc.cpp",
    "altona_wz4/altona/main/wz4lib/gui.cpp",
    "altona_wz4/altona/main/wz4lib/incbin.cpp",
    "altona_wz4/altona/main/wz4lib/poc.cpp",
    "altona_wz4/altona/main/wz4lib/poc_ops.cpp",
    "altona_wz4/altona/main/wz4lib/script.cpp",
    "altona_wz4/altona/main/wz4lib/view.cpp",
    "altona_wz4/altona/main/wz4lib/wiki.cpp",
    "altona_wz4/altona/main/wz4lib/wz4shaders.cpp",
]

# 5.5 Wiki files
wiki_srcs = [
    "altona_wz4/altona/main/wiki/resources.cpp",
    "altona_wz4/altona/main/wiki/markup.cpp",
    "altona_wz4/altona/main/wiki/layoutwindow.cpp",
    "altona_wz4/altona/main/wiki/pdf.cpp",
]

# 6. Extra files
extra_srcs = [
    "altona_wz4/altona/main/extra/mcubes.cpp",
    "altona_wz4/altona/main/extra/blobheap.cpp",
]

# 7. Wz4FrLib files (all official sources)
all_wz4frlib = glob.glob(os.path.join(ROOT_DIR, "altona_wz4/wz4/wz4frlib/*.cpp"))
exclude_wz4frlib = {
    os.path.join(ROOT_DIR, "altona_wz4/wz4/wz4frlib/wz4_ssao.cpp"),
    os.path.join(ROOT_DIR, "altona_wz4/wz4/wz4frlib/wz4_ssao_ops.cpp"),
    os.path.join(ROOT_DIR, "altona_wz4/wz4/wz4frlib/wz4_cubemap.cpp"),
    os.path.join(ROOT_DIR, "altona_wz4/wz4/wz4frlib/wz4_cubemap_ops.cpp"),
}
wz4frlib_srcs = [
    os.path.relpath(f, ROOT_DIR)
    for f in all_wz4frlib
    if f not in exclude_wz4frlib
]

# 8. Everything above is shared; each executable adds its own entry point.
common_srcs = (base_srcs + util_srcs + gui_srcs + shadercomp_srcs + wz4lib_srcs +
               wiki_srcs + extra_srcs + wz4frlib_srcs)

# The player needs a screen-mode selector (the original is a win32 dialog), an
# audio path (the original is DirectSound), and a V2 synth (the original is a
# 32 bit x86 .lib -- v2/synth_core.cpp is the portable rewrite of the same
# synth, and libv2/v2mplayer.cpp is its sequencer).
TARGETS = {
    "werkkzeug4": [
        "altona_wz4/wz4/werkkzeug4/main.cpp",
    ],
    "wz4player": [
        "altona_wz4/wz4/wz4player/main.cpp",
        "altona_wz4/wz4/wz4player/selector_posix.cpp",
        "altona_wz4/wz4/wz4player/vorbisplayer.cpp",
        "altona_wz4/wz4/wz4player/libv2/v2mplayer.cpp",
        "altona_wz4/altona/main/extra/freecam.cpp",
        "v2/synth_core.cpp",
    ],
}

# scripts/package_demo_macos.py links a second copy of the player with the
# demo's packfile baked into a __wz4pak section, and wants it somewhere other
# than bin/macos. Everything else about the link stays shared with the normal
# build, which is the point of routing it through here.
embed_pak = None
out_override = None
for a in sys.argv[1:]:
    if a.startswith("--embed-pak="):
        embed_pak = os.path.abspath(a.split("=", 1)[1])
    elif a.startswith("--out="):
        out_override = os.path.abspath(a.split("=", 1)[1])
    elif a.startswith("-"):
        sys.exit("unknown option %r" % a)

argv = [a for a in sys.argv[1:] if not a.startswith("-")]
if not argv or argv == ["all"]:
    build_targets = list(TARGETS)
else:
    for a in argv:
        if a not in TARGETS:
            sys.exit("unknown target %r, expected one of: %s, all" %
                     (a, ", ".join(TARGETS)))
    build_targets = argv

all_sources = list(common_srcs)
for t in build_targets:
    all_sources += TARGETS[t]

# Altona's sChar is `wchar_t`, forced to 16 bits by -fshort-wchar. macOS libc,
# however, is built with a 32-bit wchar_t. At -O1+ clang recognises altona's
# hand-written scan loops (sGetStringLen, sCmpString, ...) and rewrites them
# into calls to the libc wide-char routines, which then walk the strings in
# 32-bit steps and return garbage. That silently corrupts every string in the
# engine -- most visibly the scanner, which stops matching its own tokens.
# Keep the loops as loops.
no_wide_builtins = [
    "-fno-builtin-" + fn for fn in (
        "wcslen", "wcsnlen", "wcscmp", "wcsncmp", "wcschr", "wcsrchr",
        "wcsstr", "wcscpy", "wcsncpy", "wcscat", "wcsncat",
        "wmemcmp", "wmemcpy", "wmemmove", "wmemset", "wmemchr",
    )
]

cflags = [
    "-g", "-O1", "-std=c++14",
    "-fshort-wchar",
    *no_wide_builtins,
    "-Wno-reserved-user-defined-literal",
    "-Wno-deprecated-declarations",
    "-Wno-write-strings",
    "-Wno-inline-new-delete",
    "-Wno-macro-redefined",
    "-Wno-tautological-undefined-compare",
    "-Wno-undefined-bool-conversion",
    "-Wno-ignored-attributes",
    "-Wno-unused-value",
    "-DsCONFIG_SYSTEM_LINUX=1",
    "-DsCONFIG_BUILD_RELEASE=1",
    "-DsCONFIG_RENDER_OGL2=1",
    "-Ialtona_wz4/altona/main",
    "-Ialtona_wz4/altona",
    "-Ialtona_wz4/altona/main/extra",
    "-Ialtona_wz4/wz4",
    "-Ialtona_wz4/wz4/wz4frlib",
]

ldflags = [
    "-framework", "Cocoa",
    "-framework", "OpenGL",
    "-framework", "QuartzCore",
    "-framework", "AudioToolbox",
    "-framework", "AudioUnit",
    "-framework", "IOKit",
    "-framework", "CoreFoundation",
    "-framework", "Carbon",
    "-lpthread",
    "-lm",
]

# Object files are reused based on mtime, so a change to the flags above would
# otherwise go unnoticed. Stamp the flags and drop stale objects when they move.
flags_stamp = os.path.join(BUILD_DIR, ".cflags")
flags_key = "\n".join(cflags + ldflags)
if not os.path.exists(flags_stamp) or open(flags_stamp).read() != flags_key:
    for stale in glob.glob(os.path.join(BUILD_DIR, "*.o")):
        os.remove(stale)
    with open(flags_stamp, "w") as f:
        f.write(flags_key)


# The operator libraries are generated: wz4ops reads a .ops file and writes the
# .cpp/.hpp pair that registers those operators with the editor. Upstream keeps
# the generated pair out of git (altona_wz4/.gitignore), so a fresh checkout has
# only the .ops and the build has to produce the rest itself.

WZ4OPS_BIN = os.path.join(BIN_DIR, "wz4ops")
WZ4OPS_DIR = os.path.join(ROOT_DIR, "build", "wz4ops_macos")

# wz4ops is a command line tool, and has to be built as one: an altona app that
# opens a window never returns from its message loop, so a windowed wz4ops
# would generate its output and then hang instead of exiting. That means its
# own object directory -- sCONFIG_OPTION_SHELL changes what base compiles to.
WZ4OPS_SRCS = [
    "altona_wz4/altona/main/base/types.cpp",
    "altona_wz4/altona/main/base/types2.cpp",
    "altona_wz4/altona/main/base/system.cpp",
    "altona_wz4/altona/main/base/system_linux.cpp",
    "altona_wz4/altona/main/base/system_macos_shell.cpp",
    "altona_wz4/altona/main/base/graphics.cpp",
    "altona_wz4/altona/main/base/graphics_blank.cpp",
    "altona_wz4/altona/main/base/math.cpp",
    "altona_wz4/altona/main/base/serialize.cpp",
    "altona_wz4/altona/main/util/scanner.cpp",
    "altona_wz4/altona/tools/wz4ops/main.cpp",
    "altona_wz4/altona/tools/wz4ops/doc.cpp",
    "altona_wz4/altona/tools/wz4ops/parse.cpp",
    "altona_wz4/altona/tools/wz4ops/output.cpp",
    "altona_wz4/altona/tools/wz4ops/wikitext.cpp",
]


def object_for(src):
    obj_name = src.replace("/", "_").replace(".cpp", ".o").replace(".mm", ".o").replace(".c", ".o")
    return os.path.join(BUILD_DIR, obj_name)


def compile_source(src):
    ext = os.path.splitext(src)[1]
    compiler = "clang++"
    extra_flags = []
    src_cflags = list(cflags)
    if ext == ".mm":
        extra_flags = ["-x", "objective-c++"]
    elif ext == ".c":
        compiler = "clang"
        src_cflags = [f for f in src_cflags if not f.startswith("-std=")] + ["-std=c99"]
        extra_flags = []

    # The V2 synth core is not altona code: it brings its own types.h, which
    # collides with altona's (sChar is a char there and a wchar_t here). It only
    # ever talks to the rest of the program through the extern "C" synth*
    # entry points, so compile it on its own terms.
    if src.startswith("v2/"):
        src_cflags = [f for f in src_cflags
                      if not f.startswith("-I") and not f.startswith("-DsCONFIG")
                      and f != "-fshort-wchar" and not f.startswith("-fno-builtin-w")]

    obj_path = object_for(src)
    src_path = os.path.join(ROOT_DIR, src)

    if os.path.exists(obj_path):
        if os.path.getmtime(obj_path) > os.path.getmtime(src_path):
            return True, obj_path, src

    cmd = [compiler, "-c"] + src_cflags + extra_flags + [src_path, "-o", obj_path]
    res = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT_DIR)
    if res.returncode != 0:
        print(f"FAILED: {src}")
        print(res.stderr)
        return False, obj_path, src
    return True, obj_path, src

def build_wz4ops():
    """The generator is a small altona command line tool; build it on demand."""
    print("=== Building wz4ops ===")
    os.makedirs(WZ4OPS_DIR, exist_ok=True)

    shell_flags = [f for f in cflags
                   if not f.startswith("-DsCONFIG_RENDER")] + [
        "-DsCONFIG_RENDER_BLANK=1",
        "-DsCONFIG_OPTION_SHELL=1",
    ]

    objs = []
    for src in WZ4OPS_SRCS:
        obj = os.path.join(WZ4OPS_DIR, src.replace("/", "_").replace(".cpp", ".o"))
        src_path = os.path.join(ROOT_DIR, src)
        if not (os.path.exists(obj) and os.path.getmtime(obj) > os.path.getmtime(src_path)):
            res = subprocess.run(["clang++", "-c"] + shell_flags + [src_path, "-o", obj],
                                 capture_output=True, text=True, cwd=ROOT_DIR)
            if res.returncode != 0:
                print(res.stderr)
                sys.exit(f"could not compile {src} for wz4ops")
        objs.append(obj)

    res = subprocess.run(["clang++"] + objs + ["-lpthread", "-lm", "-o", WZ4OPS_BIN],
                         capture_output=True, text=True, cwd=ROOT_DIR)
    if res.returncode != 0:
        print(res.stderr)
        sys.exit("could not link wz4ops")


def generate_ops():
    """Run wz4ops over every .ops whose generated pair is missing or stale."""
    stale = []
    for ops in sorted(glob.glob(os.path.join(ROOT_DIR, "altona_wz4/wz4/wz4frlib/*.ops"))):
        gen_cpp = ops[:-4] + ".cpp"
        gen_hpp = ops[:-4] + ".hpp"
        if (not os.path.exists(gen_cpp) or not os.path.exists(gen_hpp)
                or os.path.getmtime(gen_cpp) < os.path.getmtime(ops)):
            stale.append(ops)

    if not stale:
        return

    if not os.path.exists(WZ4OPS_BIN):
        build_wz4ops()

    print(f"=== Generating {len(stale)} operator librar{'y' if len(stale)==1 else 'ies'} ===")
    for ops in stale:
        res = subprocess.run([WZ4OPS_BIN, os.path.basename(ops)],
                             capture_output=True, text=True,
                             cwd=os.path.dirname(ops))
        if res.returncode != 0 or not os.path.exists(ops[:-4] + ".cpp"):
            print(res.stdout)
            print(res.stderr)
            sys.exit(f"wz4ops failed on {os.path.basename(ops)}")


generate_ops()

# the generated files are part of the build, and may not have existed when the
# source list above was globbed
for src in sorted(glob.glob(os.path.join(ROOT_DIR, "altona_wz4/wz4/wz4frlib/*.cpp"))):
    rel = os.path.relpath(src, ROOT_DIR)
    if rel not in all_sources and os.path.join(ROOT_DIR, rel) not in exclude_wz4frlib:
        all_sources.append(rel)
        common_srcs.append(rel)

print(f"=== Compiling {', '.join(build_targets)} ({len(all_sources)} source units) ===")
failed = False

with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 8) as executor:
    futures = [executor.submit(compile_source, src) for src in all_sources]
    for future in concurrent.futures.as_completed(futures):
        success, obj, src = future.result()
        if not success:
            failed = True

if failed:
    print("Compilation failed.")
    sys.exit(1)

common_objs = [object_for(src) for src in common_srcs]

if out_override and len(build_targets) != 1:
    sys.exit("--out= needs exactly one target")

for target in build_targets:
    target_bin = out_override or os.path.join(BIN_DIR, target)
    objs = common_objs + [object_for(src) for src in TARGETS[target]]

    # -sectcreate is deliberately not part of ldflags: it must not disturb the
    # flags stamp above, which exists to drop stale objects when the *compile*
    # flags move.
    embed = ["-sectcreate", "__TEXT", "__wz4pak", embed_pak] if embed_pak else []

    print(f"=== Linking {target_bin} ===")
    link_cmd = ["clang++"] + objs + ldflags + embed + ["-o", target_bin]
    res = subprocess.run(link_cmd, capture_output=True, text=True, cwd=ROOT_DIR)
    if res.returncode != 0:
        print("Linking failed:")
        print(res.stderr)
        sys.exit(1)

    file_res = subprocess.run(["file", target_bin], capture_output=True, text=True)
    size_mb = os.path.getsize(target_bin) / (1024 * 1024)
    print(f"SUCCESS: {target_bin} ({size_mb:.2f} MB)")
    print(file_res.stdout.strip())

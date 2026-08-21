#!/usr/bin/env python3
"""
Turn a .wz4 project into a double-clickable macOS application.

  ./scripts/package_demo_macos.py altona_wz4/demos/example/example.wz4

What comes out is one self-contained executable inside a .app bundle. There is
no .wz4 next to it, no data directory, and no wz4player:

  1. wz4player -p builds a packfile. It loads the project, calculates every
     stored op with a file logger attached, and packs everything that got read
     -- the .wz4 itself, textures, songs -- plus a wz4file.txt naming the
     project, into a single .pak.

  2. That .pak is linked into a fresh copy of the player as a __TEXT,__wz4pak
     section. At startup OpenEmbeddedPackFile() (wz4player/main.cpp) finds the
     section, wraps it in a memory file and installs it as a file handler, so
     every fopen the demo does is served out of the executable's own image.

  3. The binary goes into Demo.app/Contents/MacOS and the bundle is ad-hoc
     signed.

The section has to be created at link time, not appended afterwards: on Apple
Silicon anything appended to a Mach-O invalidates its code signature.

Options:
  --name NAME     bundle and executable name (default: the .wz4's basename)
  --out DIR       where to put the .app (default: bin/macos)
  --icon FILE     an .icns to use as the application icon
  --keep-pak      leave the intermediate .pak in build/demo_packs
"""

import os
import plistlib
import shutil
import subprocess
import sys

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BUILD_SCRIPT = os.path.join(ROOT_DIR, "scripts", "build_werkkzeug4_macos.py")
PACK_DIR = os.path.join(ROOT_DIR, "build", "demo_packs")
BIN_DIR = os.path.join(ROOT_DIR, "bin", "macos")


def die(msg):
    sys.exit("package_demo_macos: " + msg)


def run(cmd, cwd=None):
    print("+ " + " ".join(cmd))
    res = subprocess.run(cmd, cwd=cwd or ROOT_DIR)
    if res.returncode != 0:
        die("command failed with %d: %s" % (res.returncode, " ".join(cmd)))


def parse_args(argv):
    wz4 = None
    opts = {"name": None, "out": BIN_DIR, "icon": None, "keep_pak": False}
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ("-h", "--help"):
            print(__doc__)
            sys.exit(0)
        elif a == "--keep-pak":
            opts["keep_pak"] = True
        elif a in ("--name", "--out", "--icon"):
            if i + 1 >= len(argv):
                die("%s needs an argument" % a)
            opts[a[2:]] = argv[i + 1]
            i += 1
        elif a.startswith("-"):
            die("unknown option %r" % a)
        elif wz4 is None:
            wz4 = a
        else:
            die("expected exactly one .wz4 file")
        i += 1
    if wz4 is None:
        die("expected a .wz4 file. see --help")
    return wz4, opts


def main():
    wz4_arg, opts = parse_args(sys.argv[1:])

    wz4 = os.path.abspath(wz4_arg)
    if not os.path.isfile(wz4):
        die("no such file: %s" % wz4_arg)

    name = opts["name"] or os.path.splitext(os.path.basename(wz4))[0]
    app = os.path.join(os.path.abspath(opts["out"]), name + ".app")
    macos_dir = os.path.join(app, "Contents", "MacOS")
    res_dir = os.path.join(app, "Contents", "Resources")
    pak = os.path.join(PACK_DIR, name + ".pak")

    if opts["icon"] and not os.path.isfile(opts["icon"]):
        die("no such icon: %s" % opts["icon"])

    os.makedirs(PACK_DIR, exist_ok=True)

    # 1. the player itself, which is also the tool that builds the packfile
    print("=== Building wz4player ===")
    run([sys.executable, BUILD_SCRIPT, "wz4player"])
    player = os.path.join(BIN_DIR, "wz4player")

    # 2. the packfile. The project's operators refer to their data by paths
    # relative to the .wz4, the way the editor left them, so pack from there.
    print("=== Packing %s ===" % os.path.basename(wz4))
    if os.path.exists(pak):
        os.remove(pak)
    run([player, "-p", pak, os.path.basename(wz4)], cwd=os.path.dirname(wz4))
    if not os.path.isfile(pak):
        die("no packfile was produced -- see the log wz4player printed above")

    # 3. relink the player around it
    print("=== Linking %s ===" % name)
    if os.path.isdir(app):
        shutil.rmtree(app)
    os.makedirs(macos_dir)
    os.makedirs(res_dir)
    exe = os.path.join(macos_dir, name)
    run([sys.executable, BUILD_SCRIPT, "wz4player",
         "--embed-pak=" + pak, "--out=" + exe])

    # 4. the bundle around that
    info = {
        "CFBundleName": name,
        "CFBundleDisplayName": name,
        "CFBundleExecutable": name,
        "CFBundleIdentifier": "com.farbrausch.wz4." + "".join(
            c.lower() if c.isalnum() else "-" for c in name),
        "CFBundleInfoDictionaryVersion": "6.0",
        "CFBundlePackageType": "APPL",
        "CFBundleShortVersionString": "1.0",
        "CFBundleVersion": "1.0",
        "LSMinimumSystemVersion": "10.13",
        "NSHighResolutionCapable": True,
        "NSPrincipalClass": "NSApplication",
    }
    if opts["icon"]:
        shutil.copy2(opts["icon"], os.path.join(res_dir, "icon.icns"))
        info["CFBundleIconFile"] = "icon.icns"

    with open(os.path.join(app, "Contents", "Info.plist"), "wb") as f:
        plistlib.dump(info, f)
    with open(os.path.join(app, "Contents", "PkgInfo"), "w") as f:
        f.write("APPL????")

    # 5. Finder refuses to launch a bundle whose signature does not cover the
    # Info.plist we just wrote, so sign the whole thing, not just the binary.
    run(["codesign", "--force", "--sign", "-", app])

    if not opts["keep_pak"]:
        os.remove(pak)

    size_mb = os.path.getsize(exe) / (1024 * 1024)
    print()
    print("SUCCESS: %s (%.2f MB, self-contained)" % (app, size_mb))
    print("double-click it, or: open %s" % app)


if __name__ == "__main__":
    main()

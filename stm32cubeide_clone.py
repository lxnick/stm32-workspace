#!/usr/bin/env python3

import os
import sys
import shutil
import glob
import re

# ============================================================
# Helpers
# ============================================================

def error(msg):
    print("")
    print(f"ERROR: {msg}")
    print("")
    sys.exit(1)

def read_text(path):

    with open(
        path,
        "r",
        encoding="utf-8",
        errors="ignore"
    ) as f:

        return f.read()

def write_text(path, content):

    with open(
        path,
        "w",
        encoding="utf-8",
        newline="\n"
    ) as f:

        f.write(content)

# ============================================================
# Copy Directory
# ============================================================

def copy_project(src, dst):

    def ignore_func(directory, files):

        ignored = []

        for f in files:

            if f in [
                "Debug",
                "Release"
            ]:
                ignored.append(f)

        return ignored

    shutil.copytree(
        src,
        dst,
        ignore=ignore_func
    )

# ============================================================
# Main
# ============================================================

if len(sys.argv) != 3:

    print("")
    print("Usage:")
    print("")
    print("  python stm32cubeide_clone.py SOURCE_PROJECT TARGET_PROJECT")
    print("")
    print("Example:")
    print("")
    print("  python stm32cubeide_clone.py l433_spi_master_test l433_spi_master_poc")
    print("")
    sys.exit(1)

src_path = os.path.abspath(sys.argv[1])
dst_name = sys.argv[2]

if not os.path.exists(src_path):
    error("Source project not found")

parent_dir = os.path.dirname(src_path)
src_name = os.path.basename(src_path)

dst_path = os.path.join(
    parent_dir,
    dst_name
)

if os.path.exists(dst_path):
    error("Target already exists")

print("")
print("============================================================")
print(" STM32CubeIDE Project Clone Utility")
print("============================================================")
print("")

print(f"Source : {src_path}")
print(f"Target : {dst_path}")

# ============================================================
# Copy
# ============================================================

print("")
print("[1/9] Copy project...")

copy_project(
    src_path,
    dst_path
)

# ============================================================
# Project Detection
# ============================================================
# ============================================================
# Find Eclipse / CubeIDE Files
# ============================================================

project_candidates = glob.glob(
    os.path.join(
        dst_path,
        "**",
        ".project"
    ),
    recursive=True
)

cproject_candidates = glob.glob(
    os.path.join(
        dst_path,
        "**",
        ".cproject"
    ),
    recursive=True
)

project_file = (
    project_candidates[0]
    if len(project_candidates) > 0
    else None
)

cproject_file = (
    cproject_candidates[0]
    if len(cproject_candidates) > 0
    else None
)

mxproject_file = os.path.join(
    dst_path,
    ".mxproject"
)

ioc_files = glob.glob(
    os.path.join(dst_path, "*.ioc")
)

has_project = project_file is not None
has_cproject = cproject_file is not None
has_mxproject = os.path.exists(mxproject_file)
has_ioc = len(ioc_files) > 0

print("")
print("Project Type Detection:")
print(f"  .project   : {has_project}")
print(f"  .cproject  : {has_cproject}")
print(f"  .mxproject : {has_mxproject}")
print(f"  .ioc       : {has_ioc}")

if not (
    has_project or
    has_mxproject or
    has_ioc
):
    error("Not a valid STM32CubeIDE/CubeMX project")

# ============================================================
# Rename Info
# ============================================================

print("")
print("Rename:")
print(f"  {src_name}")
print("  ->")
print(f"  {dst_name}")

# ============================================================
# Update .project
# ============================================================

if has_project:

    print("")
    print("[2/9] Update .project...")

    content = read_text(project_file)

    content = content.replace(
        f"<name>{src_name}</name>",
        f"<name>{dst_name}</name>"
    )

    write_text(
        project_file,
        content
    )

else:

    print("")
    print("[2/9] Skip .project")

# ============================================================
# Update .cproject
# ============================================================

if has_cproject:

    print("")
    print("[3/9] Update .cproject...")

    content = read_text(cproject_file)

    content = content.replace(
        src_name,
        dst_name
    )

    write_text(
        cproject_file,
        content
    )

else:

    print("")
    print("[3/9] Skip .cproject")

# ============================================================
# Update .mxproject
# ============================================================

if has_mxproject:

    print("")
    print("[4/9] Update .mxproject...")

    content = read_text(mxproject_file)

    patterns = [
        "SW4STM32.project.name=",
        "ProjectManager.ProjectName=",
        "project.name="
    ]

    lines = []

    for line in content.splitlines():

        for p in patterns:

            if line.startswith(p):

                line = f"{p}{dst_name}"

        lines.append(line)

    content = "\n".join(lines)

    write_text(
        mxproject_file,
        content
    )

else:

    print("")
    print("[4/9] Skip .mxproject")

# ============================================================
# Update IOC
# ============================================================

print("")
print("[5/9] Update IOC...")

for ioc in ioc_files:

    print(f"  {os.path.basename(ioc)}")

    content = read_text(ioc)

    content = re.sub(
        r"ProjectManager\.ProjectName=.*",
        f"ProjectManager.ProjectName={dst_name}",
        content
    )

    write_text(
        ioc,
        content
    )

    new_ioc = os.path.join(
        dst_path,
        f"{dst_name}.ioc"
    )

    os.rename(
        ioc,
        new_ioc
    )

    print(f"    -> {dst_name}.ioc")

# ============================================================
# Update Makefiles
# ============================================================

print("")
print("[6/9] Update Makefiles...")

makefiles = []

for root, dirs, files in os.walk(dst_path):

    for f in files:

        if (
            f == "Makefile" or
            f == "makefile" or
            f.endswith(".mk")
        ):

            makefiles.append(
                os.path.join(root, f)
            )

for mk in makefiles:

    print(f"  {os.path.basename(mk)}")

    content = read_text(mk)

    content = re.sub(
        rf"TARGET\s*=\s*{re.escape(src_name)}",
        f"TARGET = {dst_name}",
        content
    )

    content = re.sub(
        rf"OUTPUT\s*=\s*{re.escape(src_name)}",
        f"OUTPUT = {dst_name}",
        content
    )

    write_text(
        mk,
        content
    )

# ============================================================
# Update Launch
# ============================================================

print("")
print("[7/9] Update Launch...")

launch_files = glob.glob(
    os.path.join(dst_path, "*.launch")
)

for launch in launch_files:

    print(f"  {os.path.basename(launch)}")

    content = read_text(launch)

    content = content.replace(
        src_name,
        dst_name
    )

    write_text(
        launch,
        content
    )

# ============================================================
# Clean Build
# ============================================================

print("")
print("[8/9] Clean project...")

remove_list = [
    "Debug",
    "Release",
    os.path.join(".settings", "language.settings.xml")
]

for item in remove_list:

    full = os.path.join(
        dst_path,
        item
    )

    if os.path.exists(full):

        if os.path.isdir(full):

            shutil.rmtree(full)

        else:

            os.remove(full)

        print(f"  Removed: {item}")

# ============================================================
# Done
# ============================================================

print("")
print("[9/9] DONE")

print("")
print("============================================================")
print(" Clone Complete")
print("============================================================")
print("")

print("Import:")
print("")
print("  STM32CubeIDE")
print("    File")
print("    -> Import")
print("    -> Existing Projects into Workspace")
print("")
print("IMPORTANT:")
print("  [ ] Copy projects into workspace")
print("")
# pylint: skip-file
# type: ignore

import os
import fnmatch
from datetime import datetime

try:
    from SCons.Script import Import
    Import("env")
except ImportError:
    env = None

# --- CONFIGURATION ---
LOG_DIR_NAME = "build_logs"  # Folder to store the logs

# 1. DIRECTORIES TO SCAN FOR COMPILATION AUDIT
AUDIT_DIRS = ["main", "components"]
SRC_EXTS = ['*.c', '*.cpp']

# 2. IGNORE LIST (For the Tree View)
IGNORE_DIRS = {
    '.pio', 'build', 'esp_littlefs', 'cmake-build', '.vscode', 'managed_components',
    'CMakeFiles', '.git', '.github', '.idea', '__pycache__', LOG_DIR_NAME
}

IGNORE_FILES = {
    'sdkconfig', 'sdkconfig.esp32-s3.old', 'dependencies.lock',
    'CMakeCache.txt', '.DS_Store', 'Thumbs.db', 'compile_commands.json'
}

IGNORE_PATTERNS = [
    'build_log_*.txt', 'build_summary_*.txt', '*.code-workspace',
    '*.bin', '*.elf', '*.map', '*.o', '*.obj', '*.d', '*.ninja'
]


def is_ignored(name):
    """Checks if a file or folder should be hidden from the tree."""
    if name in IGNORE_DIRS or name in IGNORE_FILES:
        return True
    return any(fnmatch.fnmatch(name, p) for p in IGNORE_PATTERNS)


def generate_tree_string(root_dir, prefix=""):
    """Recursively generates a visual tree string."""
    output = ""
    try:
        entries = sorted(os.listdir(root_dir))
        entries = [e for e in entries if not is_ignored(e)]

        count = len(entries)
        for index, entry in enumerate(entries):
            path = os.path.join(root_dir, entry)
            is_last = (index == count - 1)
            connector = "└── " if is_last else "├── "
            output += f"{prefix}{connector}{entry}\n"

            if os.path.isdir(path):
                extension = "    " if is_last else "│   "
                output += generate_tree_string(path, prefix + extension)
    except OSError:
        pass
    return output


def get_audit_files(root_dir):
    """Finds source files to check against build artifacts."""
    code_files = []
    for src_dir in AUDIT_DIRS:
        start_path = os.path.join(root_dir, src_dir)
        if not os.path.exists(start_path):
            continue
        for root, dirs, files in os.walk(start_path):
            dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
            for file in files:
                if any(fnmatch.fnmatch(file, ext) for ext in SRC_EXTS):
                    full_path = os.path.join(root, file)
                    rel_path = os.path.relpath(full_path, root_dir)
                    code_files.append({"path": rel_path, "name": file})
    return code_files


def check_compilation(env_build_dir, project_files):
    """Checks if .o files exist for source files."""
    results = []
    built_objects = set()
    for root, dirs, files in os.walk(env_build_dir):
        for f in files:
            built_objects.add(f)

    for pfile in project_files:
        obj_name = pfile['name'] + ".o"
        obj_name_alt = pfile['name'] + ".obj"
        found = (obj_name in built_objects) or (obj_name_alt in built_objects)
        pfile['status'] = "COMPILED" if found else "MISSED"
        results.append(pfile)
    return results


def after_build_action(source, target, env):
    print("\n\033[1;36m[AUDIT] Generating Project Report...\033[0m")

    project_dir = env['PROJECT_DIR']
    build_dir = env['PROJECT_BUILD_DIR']

    # 1. Generate Report Content
    tree_view = "PROJECT STRUCTURE:\n.\n" + generate_tree_string(project_dir)
    files = get_audit_files(project_dir)
    audit_data = check_compilation(build_dir, files)

    timestamp_str = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    report_lines = []
    report_lines.append(f"BUILD REPORT - {timestamp_str}")
    report_lines.append("="*50)
    report_lines.append(tree_view)
    report_lines.append("="*50)
    report_lines.append(f"{'STATUS':<10} | {'SOURCE FILE USE CHECK'}")
    report_lines.append("-" * 50)

    missed_count = 0
    for item in audit_data:
        if item['status'] == "MISSED":
            indicator = "[ !! ]"
            missed_count += 1
        else:
            indicator = "[ OK ]"
        report_lines.append(f"{indicator:<10} | {item['path']}")

    report_lines.append("-" * 50)

    if missed_count > 0:
        report_lines.append(
            f"WARNING: {missed_count} file(s) are NOT being compiled.")
    else:
        report_lines.append(
            "SUCCESS: All detected source files are in the build.")

    full_report = "\n".join(report_lines)

    # 2. Save to Unique Log File
    logs_dir_path = os.path.join(project_dir, LOG_DIR_NAME)
    if not os.path.exists(logs_dir_path):
        try:
            os.makedirs(logs_dir_path)
        except OSError:
            print(
                f"\033[1;31mError: Could not create logs directory: {logs_dir_path}\033[0m")
            return

    # Generate filename: build_audit_YYYYMMDD_HHMMSS.log
    file_timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    log_filename = f"build_audit_{file_timestamp}.log"
    log_path = os.path.join(logs_dir_path, log_filename)

    with open(log_path, "w", encoding='utf-8') as f:
        f.write(full_report)

    # 3. Print to Console
    print(full_report)
    print(
        f"\033[1;32m[AUDIT] Log saved to: {LOG_DIR_NAME}/{log_filename}\033[0m")


if env:
    env.AddPostAction("buildprog", after_build_action)

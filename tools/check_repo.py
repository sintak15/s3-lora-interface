#!/usr/bin/env python3
"""Check repository paths and metadata without hardware or network access."""

import ast
import json
import re
import subprocess
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def repository_files():
    output = subprocess.check_output(
        ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"], cwd=ROOT
    )
    # Include new working files and exclude files deleted during a rename.
    return sorted({Path(name) for name in output.decode("utf-8").split("\0")
                   if name and (ROOT / name).is_file()})


def local_links(text):
    for match in re.finditer(r'!?\[[^\]]*\]\((<[^>]+>|[^\s)]+)', text):
        yield match[1].strip("<>")
    for match in re.finditer(r'\b(?:src|href)=["\']([^"\']+)["\']', text):
        yield match[1]


def missing_link(source, value):
    url = urlsplit(value)
    if url.scheme or url.netloc or not url.path:
        return False
    path = unquote(url.path)
    target = ROOT / path.lstrip("/") if path.startswith("/") else ROOT / source.parent / path
    return not target.exists()


def check_repository():
    files = repository_files()
    errors = []
    for path in files:
        source = ROOT / path
        suffix = path.suffix.lower()
        if suffix in {".log", ".pyc", ".bin", ".elf", ".o", ".bak"} or "__pycache__" in path.parts:
            errors.append(f"{path}: generated file belongs in ignored build/ or dist/")
        if len(path.parts) == 1 and path.name.startswith(("enclosure_", "enclosure-")):
            errors.append(f"{path}: enclosure assets belong in hardware/enclosure/")
        if suffix == ".json":
            try:
                json.loads(source.read_text(encoding="utf-8"))
            except (ValueError, UnicodeError) as error:
                errors.append(f"{path}: invalid JSON: {error}")
        elif suffix == ".py":
            try:
                ast.parse(source.read_text(encoding="utf-8"), filename=str(path))
            except (SyntaxError, UnicodeError) as error:
                errors.append(f"{path}: invalid Python: {error}")
        elif suffix in {".md", ".html"}:
            text = source.read_text(encoding="utf-8")
            for value in local_links(text):
                if missing_link(path, value):
                    errors.append(f"{path}: broken local link: {value}")
        elif suffix == ".scad":
            text = source.read_text(encoding="utf-8")
            text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
            includes = re.findall(r"\b(?:include|use)\s*<([^>]+)>", text)
            imports = re.findall(r'\bimport\s*\(\s*"([^"]+)"', text)
            for value in includes + imports:
                if not (source.parent / value).is_file():
                    errors.append(f"{path}: missing OpenSCAD dependency: {value}")

    constants = (ROOT / "constants.h").read_text(encoding="utf-8")
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    version = re.search(r'#define\s+FIRMWARE_VERSION\s+"([^"]+)"', constants)
    documented = re.search(r"Current firmware:\s*(v\d+\.\d+\.\d+)", readme)
    if not version or not documented or version[1] != documented[1]:
        errors.append("README.md: current firmware must match FIRMWARE_VERSION in constants.h")
    return files, errors


def main():
    files, errors = check_repository()
    if errors:
        print("Repository checks failed:")
        for error in errors:
            print(f"  {error}")
        return 1
    print(f"Repository checks passed ({len(files)} files): documentation links, CAD dependencies, "
          "JSON, Python syntax, firmware version and generated-file placement.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

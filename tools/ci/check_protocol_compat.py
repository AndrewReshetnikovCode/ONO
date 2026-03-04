#!/usr/bin/env python3
import pathlib
import re
import sys


def read_text(path: pathlib.Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except FileNotFoundError:
        print(f"error: missing file: {path}")
        sys.exit(2)


def parse_cpp_version(text: str):
    m = re.search(r"NET_PROTOCOL_VERSION_V1\s*=\s*\{\s*(\d+)\s*,\s*(\d+)\s*\}", text)
    if not m:
        print("error: could not parse NET_PROTOCOL_VERSION_V1 from NetProtocol.cpp")
        sys.exit(2)
    return int(m.group(1)), int(m.group(2))


def parse_doc_version(text: str):
    major = re.search(r"`major\s*=\s*(\d+)`", text)
    minor = re.search(r"`minor\s*=\s*(\d+)`", text)
    if not major or not minor:
        print("error: could not parse major/minor from phase0 protocol contract")
        sys.exit(2)
    return int(major.group(1)), int(minor.group(1))


def main():
    repo = pathlib.Path(__file__).resolve().parents[2]
    cpp = read_text(repo / "src" / "Lawn" / "System" / "NetProtocol.cpp")
    doc = read_text(repo / "docs" / "pvz_yg_phase0_protocol_contract.md")

    cpp_version = parse_cpp_version(cpp)
    doc_version = parse_doc_version(doc)
    if cpp_version != doc_version:
        print(f"error: protocol version mismatch cpp={cpp_version} doc={doc_version}")
        sys.exit(1)

    print(f"ok: protocol version synchronized at {cpp_version[0]}.{cpp_version[1]}")


if __name__ == "__main__":
    main()

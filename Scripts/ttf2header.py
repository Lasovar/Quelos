#!/usr/bin/env python3

import os
import re
import sys

def make_identifier(name: str) -> str:
    """Convert filename into a valid C identifier."""
    identifier = re.sub(r'[^a-zA-Z0-9]', '_', name)
    if identifier and identifier[0].isdigit():
        identifier = "_" + identifier

    return identifier

def convert_font(path: str):
    if not os.path.isfile(path):
        print(f"Skipping '{path}' (not found)")
        return

    with open(path, "rb") as f:
        data = f.read()

    filename = os.path.basename(path)
    var_name = make_identifier(filename)
    out_path = path + ".h"

    with open(out_path, "w", newline="\n") as out:
        out.write("#pragma once\n\n")
        out.write(f"static const unsigned char {var_name}[] = {{\n")

        for i in range(0, len(data), 16):
            chunk = data[i:i + 16]
            out.write("    ")
            out.write(", ".join(f"0x{b:02X}" for b in chunk))

            if i + 16 < len(data):
                out.write(",")

            out.write("\n")

        out.write("};\n\n")
        out.write(f"static constexpr unsigned int {var_name}_size = {len(data)};\n")

    print(f"Generated {out_path}")

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} font.ttf [font2.ttf ...]")
        sys.exit(1)

    for path in sys.argv[1:]:
        convert_font(path)

if __name__ == "__main__":
    main()

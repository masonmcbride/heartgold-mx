#!/usr/bin/env python3
"""Compiler-driven save record ABI audit; generated evidence stays build-local.

Compare actual compatibility declarations on native and ARMv5/AAPCS with original aligned(4) u64 targets.
This proves architecture invariance, not external cartridge interoperability.
"""
import argparse
import hashlib
import json
import re
import subprocess
import shlex
from pathlib import Path
from audit_save import tables

ROOT = Path(__file__).resolve().parents[1]

def run(args):
    result = subprocess.run(args, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return result.stdout

def layouts(text):
    result = {}
    for block in text.split("*** Dumping AST Record Layout")[1:]:
        lines = block.strip().splitlines()
        if not lines or " | " not in lines[0]:
            raise ValueError("Unrecognized compiler record layout")
        name = lines[0].split(" | ", 1)[1]
        if name in result and result[name] != lines:
            raise ValueError("Ambiguous compiler record layout: " + name)
        result[name] = lines
    return result

def closure(name, records):
    pending = [name]
    seen = set()
    while pending:
        key = pending.pop()
        if key in seen:
            continue
        if key not in records:
            raise ValueError("Missing record layout " + key)
        seen.add(key)
        for line in records[key][1:-1]:
            field = line.split(" | ", 1)[1].strip()
            if "*" in field:
                raise ValueError("Serialized record contains pointer: " + key + ": " + field)
            # Longest match resolves tagged anonymous nested records before parents.
            candidates = [k for k in records if field.startswith(k) and
                          (len(field) == len(k) or field[len(k)] in " [")]
            if candidates:
                pending.append(max(candidates, key=len))
    return seen

def callbacks(upstream):
    result = {}
    pattern = re.compile(r"^u32 (\w+)\((?:void)?\)\s*\{\s*return (sizeof\([^;]+);\s*\}", re.M)
    for path in sorted((upstream / "src").rglob("*.c")):
        for match in pattern.finditer(path.read_text()):
            if match[1] in result:
                raise ValueError("Ambiguous callback " + match[1])
            result[match[1]] = (match[2], str(path.relative_to(upstream)))
    return result

def definitions(text):
    # Balanced braces, comments removed; no substring scan through nested bodies.
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    result = {}
    for match in re.finditer(r"\b(struct|union)\s+(\w+)\s*\{", text):
        depth = 1
        end = match.end()
        while end < len(text) and depth:
            depth += (text[end] == "{") - (text[end] == "}")
            end += 1
        if depth:
            raise ValueError("Unbalanced record declaration")
        key = match[1] + " " + match[2]
        tokens = tuple(re.findall(r"\w+|[^\s]", text[match.start():end]))
        result.setdefault(key, set()).add(tokens)
    return result

def source_correspondence(names, pp, upstream):
    original = {}
    for folder in (upstream / "include", upstream / "src"):
        for path in folder.rglob("*"):
            if path.suffix in (".h", ".c"):
                for key, values in definitions(path.read_text()).items():
                    original.setdefault(key, set()).update(values)
    # Source markers come from the actual native compiler input, so a matching
    # upstream declaration elsewhere cannot disguise a different native owner.
    current = None
    origins = {}
    for line in pp.splitlines():
        marker = re.match(r'# \d+ "([^"]+)"', line)
        if marker:
            current = Path(marker[1])
            continue
        match = re.match(r"(?:typedef\s+)?(struct|union)\s+(\w+)\s*\{", line)
        if match:
            origins.setdefault(match[1] + " " + match[2], set()).add(current)
    evidence = {}
    for key in sorted(names):
        if key.startswith("struct HGSaveABI_"):
            continue
        # Anonymous records have original header paths printed directly by clang.
        if "(unnamed at " in key:
            if str(upstream) not in key and "upstream/pokeheartgold/" not in key:
                raise ValueError("Unproven anonymous native declaration: " + key)
            evidence[key] = "direct original declaration"
            continue
        paths = origins.get(key, set())
        if len(paths) != 1:
            raise ValueError("Ambiguous native declaration origin: " + key + str(paths))
        path = next(iter(paths))
        if path.resolve().is_relative_to(upstream):
            evidence[key] = "direct original declaration"
        else:
            native_defs = definitions(path.read_text()).get(key, set())
            if len(native_defs) != 1 or not native_defs.issubset(original.get(key, set())):
                raise ValueError("Native declaration differs from original: " + key + " in " + str(path))
            evidence[key] = "token-identical original declaration extracted at native boundary"
    return evidence

def audit(build, output, cc):
    upstream = ROOT / "upstream/pokeheartgold"
    normal, extra = tables((upstream / "src/save_arrays.c").read_text())
    if (len(normal), len(extra)) != (42, 6):
        raise ValueError("Unexpected registry cardinality")
    output.mkdir(parents=True, exist_ok=True)
    index = callbacks(upstream)
    rows = []
    source = ['#include "compat/save_c_records.c"',
              '#include "compat/motion.h"', '#include "save_local_field_data.h"',
              '#include "bag.h"', '#include "mail.h"',
              '#include "seal_case.h"', '#include "save_pokegear.h"']
    text = (upstream / "src/save_local_field_data.c").read_text()
    for tag in ("LocalFieldData", "SavedMapObjectList"):
        matches = re.findall(r"^struct " + tag + r" \{.*?^\};", text, re.M | re.S)
        if len(matches) != 1:
            raise ValueError("Ambiguous private original record " + tag)
        source.append(matches[0])
    for group, entries in (("normal", normal), ("extra", extra)):
        for fields in entries:
            row = dict(group=group, id=fields[0], size_callback=fields[2])
            if fields[2] in index:
                expression, path = index[fields[2]]
                match = re.fullmatch(r"sizeof\(([^)]+)\)(.*)", expression)
                if not match:
                    raise ValueError("Unsupported size expression: " + expression)
                wrapper = "HGSaveABI_" + str(len(rows))
                row.update(kind="native C record", expression=expression,
                           source=path, wrapper="struct " + wrapper)
                source.append("struct " + wrapper + " { " + match[1] + " value" +
                              ("[NUM_PALPAD_ENTRIES]" if match[2] == " * NUM_PALPAD_ENTRIES" else "") + "; };")
                if match[2] not in ("", " * NUM_PALPAD_ENTRIES"):
                    raise ValueError("Unsupported size multiplier")
                source.append("const unsigned hg_save_abi_size_" + str(len(rows)) + " = " + expression + ";")
            else:
                # Original ASM sizes and initialization are executed by existing
                # guest tests. Never pretend opaque guest records have C layouts.
                matches = []
                for path in (upstream / "asm").glob("*.s"):
                    if re.search(r"^" + fields[2] + r":", path.read_text(), re.M):
                        matches.append(str(path.relative_to(upstream)))
                if len(matches) != 1:
                    raise ValueError("Missing/ambiguous ASM callback " + fields[2])
                row.update(kind="opaque original ARM guest record", source=matches[0])
            rows.append(row)
    probe = output / "save_abi.c"
    probe.write_text("\n".join(source) + "\n")
    common = [cc, "-std=gnu11"]
    for path in (upstream / "include", build / "generated", ROOT / "src", ROOT / "src/compat/include"):
        common.append("-I" + str(path))
    for name in ("overlays", "native", "props", "field"):
        common += ["-include", str(ROOT / "src/compat" / (name + ".h"))]
    pp = run(common + ["-E", str(probe)])
    # Native libc declarations are irrelevant to the serialized closure. ARM
    # lacks its host-only 128-bit register type; a stand-in permits parsing it.
    # No serialized record may contain that type (checked below).
    arm_probe = output / "save_abi_arm.i"
    arm_probe.write_text("typedef unsigned long long __uint128_t;\n" + pp)
    native_text = run(common + ["-Xclang", "-fdump-record-layouts-complete", "-fsyntax-only", str(probe)])
    arm_text = run([cc, "-target", "armv5-none-eabi", "-mabi=aapcs", "-std=gnu11", "-fblocks", "-Wno-everything", "-Xclang", "-fdump-record-layouts-complete", "-fsyntax-only", str(arm_probe)])
    (output / "native-layout.txt").write_text(native_text)
    (output / "arm-layout.txt").write_text(arm_text)
    native, arm = layouts(native_text), layouts(arm_text)
    all_records = set()
    for row in rows:
        if "wrapper" not in row:
            continue
        records = closure(row["wrapper"], native)
        for name in records:
            if native[name] != arm.get(name):
                raise ValueError("Native/ARM serialized layout mismatch: " + name + "\n" +
                                 "\n".join(native[name]) + "\nARM:\n" + "\n".join(arm.get(name, [])))
            if any("__uint128_t" in line for line in native[name]):
                raise ValueError("Host-only type in serialized closure")
        row["layout_records"] = sorted(records)
        row["size"] = int(re.search(r"sizeof=(\d+)", native[row["wrapper"]][-1])[1])
        row["layout_sha256"] = hashlib.sha256("\n".join(line for key in sorted(records) for line in native[key]).encode()).hexdigest()
        all_records |= records
    correspondence = source_correspondence(all_records, pp, upstream)
    # Query the actual linked registry, including every original guest size
    # callback, rather than trusting source-size expressions alone.
    runtime_source = output / "runtime_sizes.c"
    runtime_source.write_text('#include "compat/save_manager.h"\nint main(void) {\n'
        'for(int i=0;i<gNumSaveChunkHeaders;i++) printf("%u\\n",gSaveChunkHeaders[i].sizeFunc());\n'
        'for(int i=0;i<gNumExtraSaveChunkHeaders;i++) printf("%u\\n",gExtraSaveChunkHeaders[i].sizeFunc());\nreturn 0; }\n')
    runtime_object = output / "runtime_sizes.o"
    run(common + ["-c", str(runtime_source), "-o", str(runtime_object)])
    # Reuse the project's authoritative dependency order, isolated output only.
    link_file = build / "CMakeFiles/test_save_manager.dir/link.txt"
    link = shlex.split(link_file.read_text())
    command = []
    skip = False
    executable = output / "runtime_sizes"
    for pos, token in enumerate(link):
        if skip:
            skip = False
            continue
        if token == "-o":
            command += ["-o", str(executable)]
            skip = True
        elif token.endswith(".o"):
            command.append(str(runtime_object))
        elif token.endswith(".a"):
            command.append(str(build / token))
        else:
            command.append(token)
    run(command)
    runtime_sizes = list(map(int, run([str(executable)]).splitlines()))
    if len(runtime_sizes) != len(rows):
        raise ValueError("Runtime registry cardinality mismatch")
    for row, size in zip(rows, runtime_sizes):
        if "size" in row and row["size"] != size:
            raise ValueError("Linked callback size mismatch: " + row["id"])
        row["runtime_size"] = size
    report = dict(method="native versus ARMv5/AAPCS with original aligned(4) u64 compiler field layouts over actual native declarations", caveat="Architecture invariance only. Original declaration correspondence and actual cartridge interoperability are separate obligations.", records=rows, unique_layouts=len(all_records), source_correspondence=correspondence)
    (output / "records.json").write_text(json.dumps(report, indent=2) + "\n")
    c_rows = sum("wrapper" in row for row in rows)
    print(f"Save ABI: {c_rows} C payloads / {len(all_records)} recursive layouts agree; {len(rows)-c_rows} opaque original ARM guest payloads classified.")
    return report

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--output", type=Path, default=ROOT / "build/save-abi")
    parser.add_argument("--cc", default="/usr/bin/cc")
    args = parser.parse_args()
    audit(args.build.resolve(), args.output.resolve(), args.cc)

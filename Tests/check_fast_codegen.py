"""Check that fast x64 code generation fires, honors opt-outs, and runs.

Uses only the standard library and the compiler's ELF intermediate objects.
All compiler invocations are serial; output files are rebuilt every time.
"""
import pathlib
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMPILER = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "cpc.exe"
OUT = ROOT / "build" / "check-fast-codegen"
OUT.mkdir(parents=True, exist_ok=True)
SOURCE = OUT / "check.c"
SOURCE.write_text(r"""
#include <string.h>
int leaf(int x) { return x * 37 + 5; }
int wrapper(int x) { return leaf(x); }
__attribute__((noinline)) int retained(int x) { return x * 19; }
int calls_retained(int x) { return retained(x); }
void *copy31(void *d, const void *s) { return memcpy(d, s, 31); }
void *clear31(void *d) { return memset(d, 0, 31); }
__attribute__((noinline)) void opaque(char *p) { p[0] = 1; }
int near_probe(int a, int b, int c, int d) {
    char padding[4017];
    padding[0] = a;
    opaque(padding); a += b;
    opaque(padding); b += c;
    opaque(padding); c += d;
    opaque(padding); d += a;
    return a + b + c + d + padding[0];
}
int main(void) {
    char a[32] = "abcdefghijklmnopqrstuvwxyz", b[32];
    if (wrapper(7) != 264 || calls_retained(7) != 133) return 1;
    if (copy31(b, a) != b || memcmp(a, b, 31)) return 2;
    if (clear31(b) != b || b[0] || b[30]) return 3;
    return 0;
}
""")


def calls(path):
    data = path.read_bytes()
    assert data[:6] == b"\x7fELF\x02\x01", "expected ELF64 little-endian object"
    section_offset = struct.unpack_from("<Q", data, 40)[0]
    section_size, count = struct.unpack_from("<HH", data, 58)
    sections = [struct.unpack_from("<IIQQQQIIQQ", data, section_offset + i * section_size)
                for i in range(count)]
    symbols = []
    for section in sections:
        if section[1] != 2:
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        for at in range(section[4], section[4] + section[5], section[9]):
            name, info, other, index, value, size = struct.unpack_from("<IBBHQQ", data, at)
            symbols.append((names[name:].split(b"\0", 1)[0].decode(), index, value, size, info))
    result = {}
    for section in sections:
        if section[1] != 4:
            continue
        for at in range(section[4], section[4] + section[5], section[9]):
            offset, info, addend = struct.unpack_from("<QQq", data, at)
            for name, index, value, size, kind in symbols:
                if kind & 15 == 2 and index == section[7] and value <= offset < value + size:
                    result.setdefault(name, set()).add(symbols[info >> 32][0])
                    break
    bodies = {name: data[sections[index][4] + value:sections[index][4] + value + size]
              for name, index, value, size, kind in symbols
              if kind & 15 == 2 and 0 < index < len(sections)}
    return result, bodies


for name, flags in [("plain", ["-O0"]), ("fast", ["-O2"]), ("fast-debug", ["-O2", "-g"]),
                    ("no-inline", ["-O2", "-fno-inline"]),
                    ("no-builtin", ["-O2", "-fno-builtin"])]:
    obj = OUT / (name + ".o")
    obj.unlink(missing_ok=True)
    subprocess.run([str(COMPILER), *flags, "-Werror", "-c", str(SOURCE), "-o", str(obj)], check=True)
    refs, bodies = calls(obj)
    frame = bodies["near_probe"]
    if frame[0] == 0xb8:  # The aligned frame requires the stack-probe prolog.
        assert frame[11:13] != b"\x4c\x89", "probed frame must not use the ordinary saved-register prolog"
    assert ("leaf" in refs.get("wrapper", set())) == (name in ("plain", "no-inline", "fast-debug")), (name, refs)
    assert "retained" in refs.get("calls_retained", set()), (name, refs)
    for fn, target in [("copy31", "memcpy"), ("clear31", "memset")]:
        assert (target in refs.get(fn, set())) == (name in ("plain", "no-builtin")), (name, refs)
    exe = OUT / (name + ".exe")
    exe.unlink(missing_ok=True)
    subprocess.run([str(COMPILER), *flags, "-Werror", str(SOURCE), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
batch_sources = {
    "a": "static int leaf(int x) { return x + 3; } int first(void) { return leaf(7); }",
    "b": "static int leaf(int x) { return x + 9; } int second(void) { return leaf(7); }",
    "main": "int first(void); int second(void); int main(void) { return first()!=10 || second()!=16; }",
}
jobs = []
objects = []
for name, source in batch_sources.items():
    path = OUT / ("batch-" + name + ".c")
    obj = path.with_suffix(".o")
    path.write_text(source)
    obj.unlink(missing_ok=True)
    objects.append(str(obj))
    jobs.append(["-O2", "-Werror", "-c", str(path), "-o", str(obj)])
batch_exe = OUT / "batch.exe"
batch_exe.unlink(missing_ok=True)
jobs.append([*objects, "-o", str(batch_exe)])
batch = OUT / "batch.txt"
batch.write_text("\n".join(" ".join('"' + arg.replace('\\', '\\\\').replace('"', '\\"') + '"'
                                      for arg in job) for job in jobs))
subprocess.run([str(COMPILER), "@" + str(batch)], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
subprocess.run([str(batch_exe)], check=True)
print("PASS: inline code, noinline attributes, opt-outs, small copies/fills, runtime results, and fresh batch state")

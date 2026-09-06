"""Exercise directory enumeration without changing include search semantics."""
import pathlib
import subprocess
import tempfile

compiler = pathlib.Path(__file__).resolve().parent.parent / "cpc.exe"
with tempfile.TemporaryDirectory(prefix="cprime-include-search-") as temp:
    root = pathlib.Path(temp).resolve()
    assert root.parent == pathlib.Path(tempfile.gettempdir()).resolve()
    for name in ("first", "second", "left", "right"):
        (root / name).mkdir()
    (root / "first/chain.h").write_text("#pragma once\n#define FIRST 7\n#include_next <chain.h>\n")
    (root / "second/chain.h").write_text("#pragma once\n#define SECOND 11\n")
    (root / "first/repeat.h").write_text("VALUE += 1;\n")
    (root / "left/local.h").write_text("#define LEFT 13\n")
    (root / "right/local.h").write_text("#define RIGHT 17\n")
    (root / "left/entry.h").write_text('#include "local.h"\n')
    (root / "right/entry.h").write_text('#include "local.h"\n')
    source = root / "main.cpp"
    source.write_text(r'''#include <chain.h>
#include <chain.h>
#include "left/entry.h"
#include "right/entry.h"
#if __has_include(<absent-header.h>)
#error missing header reported present
#endif
#if !__has_include(<repeat.h>)
#error existing header reported absent
#endif
int main() {
  int result = 0;
#define VALUE result
#include <repeat.h>
#include <repeat.h>
  return FIRST != 7 || SECOND != 11 || LEFT != 13 || RIGHT != 17 || result != 2;
}
''')
    args = [str(compiler), "-Werror", "-I" + str(root / "missing"), "-I" + str(root / "first"), "-I" + str(root / "second")]
    exe = root / "test.exe"
    subprocess.run(args + [str(source), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    # A subsequent compilation must observe newly created files/directories.
    (root / "missing").mkdir()
    (root / "missing/new.h").write_text("#define NEW_VALUE 23\n")
    source.write_text('#include <new.h>\nint main() { return NEW_VALUE != 23; }\n')
    subprocess.run(args + [str(source), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    # One compiler invocation with two inputs must reset include guards/cache.
    (root / "shared.h").write_text("#pragma once\n#define SHARED_VALUE 29\n")
    first = root / "one.cpp"
    second = root / "two.cpp"
    first.write_text('#include "shared.h"\nint other() { return SHARED_VALUE; }\n')
    second.write_text('#include "shared.h"\nint other(); int main() { return other() != SHARED_VALUE; }\n')
    subprocess.run(args + [str(first), str(second), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("PASS include order, include_next, local origins, guards, repeated unguarded headers, has_include, fresh compilation state")

"""Run the canonical CL Assimp roundtrip after building the FreeLancer graph."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--project-root", type=Path, default=Path("C:/Luke/Src/OT/cl"))
parser.add_argument("--build-dir", type=Path)
parser.add_argument("--compiler", type=Path)
args = parser.parse_args()
build = args.build_dir or args.project_root / "builds/prime"
inputs = json.loads((build / "compile_inputs.json").read_text(encoding="utf-8-sig"))
entry = next(x for x in inputs if Path(x["Source"]).name == "clAssimp.cpp")
link = json.loads((build / "link_inputs.json").read_text(encoding="utf-8-sig"))
compiler = str(args.compiler.resolve()) if args.compiler else link["Compiler"]
fixture = Path(__file__).resolve().parent / "integration/cl_assimp"

with tempfile.TemporaryDirectory(prefix="cprime-assimp-") as temporary:
    work = Path(temporary)
    for name in ("triangle.obj", "triangle.mtl", "texture.png"):
        source = fixture / (name + ".txt" if name == "triangle.obj" else name)
        shutil.copyfile(source, work / name)
    objects = []
    for index, source in enumerate((entry["Source"], str(fixture / "roundtrip.cpp"))):
        obj = str(work / f"source{index}.obj")
        subprocess.run([compiler, *entry["Flags"], "-c", source, "-o", obj],
                       cwd=entry["Directory"], check=True)
        objects.append(obj)
    flags = [x for x in link["Arguments"] if not x.endswith(".obj")]
    executable = str(work / "roundtrip.exe")
    flags[flags.index("-o") + 1] = executable
    subprocess.run([compiler, *objects, *flags], cwd=link["Directory"], check=True)
    env = os.environ.copy()
    env["PATH"] = str(args.project_root / "builds") + os.pathsep + env["PATH"]
    subprocess.run([executable, str(work)], env=env, check=True, timeout=20)

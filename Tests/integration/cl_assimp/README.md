Build FreeLancer through the ordinary CodeClip manifest and shared project
driver, then run `python Tests/test_cl_assimp.py` from the CPrime repository.
Use `--project-root`, `--build-dir`, and `--compiler` for other checkouts or
preserved build inputs. The selected build must include `clAssimp.cpp`.

The fixture checks OBJ geometry, UVs, material color and texture-path resolution;
DAE export and reimport; and missing-input/output failure handling. Each run
copies its inputs to a temporary directory and keeps exported files there.
`triangle.obj.txt` is copied to `triangle.obj` to avoid the repository's object
file ignore rule. `texture.png` is a path-resolution sentinel, not an image;
this fixture does not test image decoding or rendering.

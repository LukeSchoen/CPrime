Run the CommonLib image include-order regressions with the selected CodeClip
manifest (which must include `clImage.cpp`):

```powershell
./Tests/run.ps1 -Suite integration/unity -BuildManifestPath 'C:/Luke/Src/OT/cl/builds/manifest/Release-x64.json'
```

These compile-only probes check the real image header with Windows included
before and after it. The memory-loading method must have the same declaration,
definition and call spelling in both orders. The old `LoadImage` name collided
with the Windows object-like macro; CommonLib now uses `LoadImageMemory`.
The full application unity build exercises the real implementation and link.

`Tests/check_build_manifest.ps1` separately tests generic unity grouping,
separate builds, manifest-selected inputs, libraries, resources and execution.
It also checks that the driver defaults to the caller's project directory.

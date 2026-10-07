# Legacy M8 snapshot

The last recovered project pack before this repository was initialized is:

- `EMU_PS5_FAMILY_M8_2026-09-30.zip`
- generated 2026-09-30
- recovered from the project's prior ChatGPT file library
- approximately 4.2 MB compressed / 12 MB extracted
- 321 files

It contains the accumulated M0→M8 engineering work: CMake target generation, shared runtime/VFS, native backend catalog, platform services, Web UI, validation tests, PS5 artifacts, vendor references and 43 system target manifests.

## Why it is not copied wholesale into the new root

M8 mixes source, generated artifacts, vendored reference code, ELF/PKG outputs and historical experiments. The new repository deliberately separates:

- current architecture;
- reusable source;
- historical snapshots;
- binaries/artifacts;
- third-party/vendor material.

M8 is therefore treated as a **historical snapshot**, not as the new source layout.

## Recovered notable contents

- `CMakeLists.txt`
- `src/config.cpp`
- `src/library.cpp`
- `src/read_cache.cpp`
- `src/system_registry.cpp`
- `src/target_main*.cpp`
- `src/vfs.cpp`
- PS5 platform/runtime sources
- native port catalog generator
- target generator/validator
- M2→M8 runtime and platform tests
- `docs/M8_NATIVE_PORTS.md`
- `docs/M8_UPDATES_2026-09-30.md`
- donor pin/update lockfiles
- 43 target manifests
- Web UI
- NES native alpha build/package artifacts

## Migration rule

Useful M8 code should be migrated into the new tree intentionally, module by module, after checking:

1. whether a newer scene implementation now exists;
2. license/provenance;
3. whether the code belongs in `runtime/` or one emulator;
4. whether it was only an experiment/probe;
5. whether it has a reproducible PS5 build path.

This keeps the new project clean while preserving the history needed to recover any earlier experiment.

# Recovered Artifact Manifest

This file records concrete artifacts recovered from the earlier EMU-PS5 work.

## Latest recovered family snapshot

### EMU_PS5_FAMILY_M8_2026-09-30.zip

SHA-256:

```text
c82c155c80f8975289b82703e4c21c040187a9e1ae0bb0055f634e000ad9e452
```

Recovered metadata:

- compressed size: 4,215,643 bytes
- extracted size: approximately 12 MB
- file count: 321
- includes accumulated M0→M8 source/research/tests/artifacts
- includes 43 emulator/system target manifests
- includes CMake generation, runtime/VFS, platform code, Web UI and validation material
- contains generated ELF/PKG binaries; those are intentionally not mixed into the new source tree

See `legacy/m8/README.md`.

## NES native alpha

### EMU_NES_ALPHA_M7.pkg

SHA-256:

```text
3aef3abc9ba842a80111246bcd95fc49f6b17795f24b31d37d0d60df6cb75af0
```

Recovered size: 2,241,901 bytes.

This is preserved as a historical proof/build artifact. It is not evidence that every target in the current repository has reached the same implementation stage.

## PS5SX2 SMB patch

Recovered pack:

```text
PS5SX2_SMB_PATCH_2026-09-29.zip
```

Purpose:

- mount-free SMB2/SMB3 PS2 image loading
- libsmb2 client
- ISO random-access streaming
- CHD core_file adapter
- reconnect/reopen on transient reads
- local/USB behavior left intact
- optional build-time SMB support

Recovered patch payload contains:

- `ProsperoSMB.h/.cpp`
- `SmbFileReader.h/.cpp`
- `build-libsmb2.sh`
- `smb.ini.example`
- guarded patch installer for PS5SX2

Before migrating this patch into active work, it must be rebased against the current Swordpdf/PS5SX2 tree and revalidated because PS5SX2 is changing rapidly.

## Earlier milestone archives

Recovered Library metadata also confirms M0, M1, M2, M3, M4, M5, M6 and M7 archives with companion SHA-256 files.

They are not duplicated into Git history because M8 is the latest accumulated snapshot and the new repository is intended to remain source-oriented rather than becoming an archive of redundant ZIPs.

## Preservation policy

Binary releases belong in GitHub Releases or another artifact store when release-upload automation is available.

Git history should contain source, manifests, patches, reproducibility metadata and small test fixtures—not multi-generation duplicate build archives.

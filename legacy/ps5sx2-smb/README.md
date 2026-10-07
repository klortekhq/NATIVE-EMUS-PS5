# Recovered PS5SX2 SMB2/SMB3 patch

Historical patch created 2026-09-29 for Swordpdf/PS5SX2.

## Purpose

Add **mount-free PS2 game streaming over SMB2/SMB3** through libsmb2.

The recovered patch implemented:

- recursive remote `.iso` / `.chd` discovery;
- random-access ISO streaming through PCSX2's `ThreadedFileReader`;
- 512 KiB readahead chunks;
- reads split to the server-negotiated MaxReadSize;
- one reconnect/reopen retry after a read failure;
- libchdr `core_file` callbacks for self-contained CHDs;
- local `/data/PCSX2` and USB sources left unchanged;
- optional compile-time support when `libsmb2.a` is available;
- credentials stored only in `/data/PCSX2/smb.ini`.

## Recovered source files

```text
ps5/coreorbis/include-orbis/ProsperoSMB.h
ps5/coreorbis/orbis-shims/ProsperoSMB.cpp
pcsx2/CDVD/SmbFileReader.h
pcsx2/CDVD/SmbFileReader.cpp
ps5/tools/build-libsmb2.sh
ps5/docs/smb.ini.example
apply_smb_patch.py
```

## Limitation in the historical build

Differencing/parent CHDs over SMB were deliberately rejected. Self-contained CHD and ISO images were the supported network formats.

## Migration status

**Recovered, not yet rebased.**

Swordpdf/PS5SX2 has changed substantially since this patch was generated, particularly around Vulkan, frontend behavior and platform code. Do not blindly run the old patch installer against current upstream.

The useful architectural pieces should be migrated individually:

1. generic SMB VFS/session layer -> shared runtime candidate;
2. PCSX2 `SmbFileReader` -> PCSX2-specific adapter;
3. CHD `core_file` adapter -> reusable streaming concept;
4. frontend discovery -> current PS5SX2/frontend integration;
5. credentials/config -> project VFS configuration policy.

This history is retained because network-backed game images remain a useful target for the native emulator family.

# NATIVE Game Boy Advance — mGBA

This port track cross-builds the pinned primary mGBA upstream directly for the
public PS5 toolchain. The first gate intentionally produces only the static GBA
core archive; it does not claim a standalone PS5 title or physical-console boot.

The gate builds mGBA's own static core library directly, not its libretro
frontend. The final architecture keeps that upstream core recognizable while
PS5 lifecycle, input, AudioOut, VideoOut and VFS are supplied by this
repository's shared native runtime rather than by a RetroArch executable.

Current pin: `c3c8e5e813f245028de118a56734e1dc0f35ce2a` (MPL-2.0).

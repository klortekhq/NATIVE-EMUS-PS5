# PPSSPP upstream pin

- Upstream: https://github.com/hrydgard/ppsspp
- Release: **v1.20.4**
- Tag object: `3a31057b7e44270b4d5cef8c31b6559d51802a3b`
- Commit: `fa50bb1976065c4f8b1b47af227d367fe9771555`
- Verified: 2026-09-30

The current native PS5 extraction is intentionally pinned to the stable release while the platform delta is brought up reproducibly.

The final CPU backend is **PPSSPP's native x86-64 MIPS JIT**. Interpreter mode is not an accepted final configuration for the PS5 port.

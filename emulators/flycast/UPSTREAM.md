# Flycast upstream pin

- Upstream: https://github.com/flyinghead/flycast
- Commit: `e36e9df2dcc1487acdb1dc7725766f1f5ba029b5`
- Pin verified: 2026-10-01
- License: GPL-2.0-or-later lineage; preserve upstream notices/source obligations.

This pin includes the current rec-x64/Xbyak SH4 dynarec and Vulkan renderer.

## PS5 CPU policy

The PS5 target is built around:

- `HOST_CPU = CPU_X64`
- `FEAT_SHREC = DYNAREC_JIT`
- `FEAT_NO_RWX_PAGES`
- Zen 2 compile tuning: `-march=znver2 -msse4.1 -mavx2 -mno-vzeroupper`

The final port does **not** use the SH4 interpreter as its normal CPU backend.

The pin is intentionally exact while the PS5 platform delta is stabilized. Rebases should be explicit and CI-checked.


## 2026-10-01 upstream audit

Upstream was checked again before continuing the PS5 port. A newer Flycast
master revision exists, but the newest observed change is in Windows HTTP code
and does not affect the PS5 rec-x64, virtual-memory or Vulkan integration
points used by this gate. The exact PS5 bring-up pin therefore remains
`e36e9df2dcc1487acdb1dc7725766f1f5ba029b5` until a relevant upstream delta
justifies a reviewed rebase.

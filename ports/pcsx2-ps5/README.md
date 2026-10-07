# PCSX2 native PS5 recompiler gate

Pinned donor: `Swordpdf/PS5SX2@9a86c58f84d9fa328589fb8e14403bbcd01d35a6`.

This gate deliberately compiles **only the CPU/VU recompiler set** from
PS5SX2's public `Makefile.vk`. It does not depend on the non-public
PS5HB_Vulkan source and does not build a frontend.

Required native engine contents:

- EE / R5900 x86 recompiler
- IOP / R3000A x86 recompiler
- VIF dynarec
- microVU
- recVTLB / x86 emitter path

Compiler target remains PS5 Zen 2:

- `-march=znver2`
- SSE4.1
- AVX2
- no VZEROUPPER transition workaround
- 16 KiB host page-size override from PS5SX2

Interpreter paths may remain in PCSX2 as upstream fallback for individual
operations, but are **not** the selected host CPU engine for this project.

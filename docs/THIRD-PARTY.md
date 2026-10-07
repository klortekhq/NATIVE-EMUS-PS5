# Third-Party and Upstream Policy

NATIVE-EMUS-PS5 is an integration and porting project.

## Upstream projects

Each emulator keeps its own license and copyright.

Examples include PCSX2, Flycast, RPCS3, Vita3K, Cemu, xemu, Xenia, PPSSPP, Dolphin, Mupen64Plus and Azahar.

Before importing source or patches:

1. identify the exact upstream license;
2. preserve required notices;
3. record the upstream commit;
4. avoid mixing incompatible code into a shared module;
5. prefer patches/submodules/vendor manifests over unattributed source copies.

## Scene projects

Useful PS5 scene implementations may have different licenses from the emulator being ported.

A public implementation is **not automatically reusable code**.

When a project is useful but its license is incompatible with the intended destination:

- use it as behavioral/API research;
- document the concept;
- reimplement cleanly where legally appropriate;
- do not copy code across incompatible license boundaries.

## Proprietary material

Do not commit:

- Sony proprietary SDK headers/libraries;
- copyrighted firmware/BIOS dumps;
- console keys;
- decrypted system modules;
- commercial game data.

Public clean-room headers, SDKs and reverse-engineered interfaces may be used according to their own licenses.

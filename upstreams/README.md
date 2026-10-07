# Upstream Manifest

`manifest.json` is the source-of-truth for the external codebases behind emulator workspaces.

## Pin states

- `repository-verified-revision-pending` — the canonical/current GitHub repository has been identified, but NATIVE-EMUS-PS5 has not pinned an exact commit yet.
- `canonical-source-verification-pending` — do not import source until the canonical project source is verified.
- future `pinned` — exact revision and license have been recorded and are ready for reproducible integration.

## Required fields before source import

A workspace must have:

1. canonical upstream URL;
2. exact commit SHA/tag;
3. license/SPDX classification;
4. PS5 scene donor references, if any;
5. patch strategy: submodule, subtree, generated patchset or vendored snapshot;
6. build/toolchain notes.

## Scene references are not upstreams

PS5-specific forks are tracked separately because they may contain crucial host fixes without being the canonical emulator source.

Examples:

- PCSX2 upstream vs Swordpdf/PS5SX2;
- RPCS3 upstream vs mihawk-99/PS5_RPCS3;
- LLVM upstream vs PS5_LLVM;
- Dynarmic upstream vs PS5_Dynarmic.

This separation is deliberate: it lets the project absorb PS5 engineering without accidentally freezing itself to an unrelated fork forever.


## PS5 scene reference cache

`scene-cache.json` is the preservation manifest for **non-canonical PS5
engineering references**. It is deliberately separate from `manifest.json`,
which tracks the canonical emulator upstreams.

Each scene-cache entry records:

- exact repository and commit;
- license/provenance state;
- priority and technical categories;
- why it is useful to NATIVE-EMUS-PS5;
- whether source preservation is allowed.

For entries with `cache_source=true`,
`tools/ps5/cache_scene_references.sh` checks out the exact commit, initializes
its pinned submodules, creates a source snapshot and records SHA-256 metadata.
The `reference-cache` workflow refreshes those snapshots weekly and when the
manifest/cache tooling changes.

A cached reference is **not** vendored code and is never imported
automatically. Reuse still requires a deliberate per-file/per-component
license and architecture review.

Binary-only/proprietary projects remain metadata-only. Projects whose aggregate
license or vendored-component status has not been reviewed can also remain
metadata-only even when their source is public.

The cache covers Git-tracked source. External release assets, Git LFS payloads,
private dependencies and disappearing third-party download URLs require their
own provenance decision and are not silently treated as preserved.

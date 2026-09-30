# Recovered development branches — 2026-10-01

This directory preserves the exact text content that existed only on temporary
development branches before the repository returned to a single-main workflow.

## Original branch heads

- `checkpoint-2026-09-30-2214` — historical checkpoint originally at `d5d4e01716ab25e2c196f6a23c61a2a1924a692c`
- `dev/cemu-rpcs3-jit` — original head `8865bda3e5ef3dff0cbea671a0f7934a82770ac1`
- `dev/n64-jit-fix` — original head `314fbe017f3989c54b9734e4ff9e7e002ff13985`
- `dev/xenia-fixed-jit` — original head `4570c662c82414248bf0d91dac7171ff9dab4288`

Before the temporary refs were aligned to `main`, files whose branch versions
could differ from the current main implementation were copied below this
directory.

This is preservation only. Active development must happen on `main`.

## Policy from this point

- no feature branches for routine emulator work;
- no branch-only code;
- every useful experiment either lands on `main` or is archived under
  `legacy/`;
- old branch snapshots are never treated as the active implementation when a
  newer main version exists.

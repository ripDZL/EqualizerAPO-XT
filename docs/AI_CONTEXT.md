# AI Context

- Active task: EqualizerAPO-XT VST plugin GUI live analyzer feedback on `beta`.
- Current change: Editor-side live analyzer preview feed for open VST plugin panels.
- Approach: capture default system playback with WASAPI loopback and feed copied blocks into the visible editor-owned plugin instance; discard output.
- Scope note: this animates analyzer-style plugin GUIs while the panel is open; real APO audio processing remains in the service-owned instance.

## Matrix pet backlog

- Task: Hatch a Codex v2 animated pet based on Matrix from ReBoot.
- Pet name: Matrix.
- Reference: `pet-runs/matrix/references/Matrix_with_Gun.webp`.
- Identity locks: green skin, black hair, yellow cybernetic left eye, muscular build, black armored sleeveless top, blue trousers, black boots, chain belt, large black Gun prop.
- Style: compact polished 3D character sprite; preserve the ReBoot-era rendered look.
- Blocker: this Codex environment does not expose the internal bundled Python/Pillow runtime; there is no documented user-facing toggle.

## EqualizerAPO-XT VST3

- Commit: `9e2ed30`.
- Branch: `agent/vst3-host-editor-compatibility`.
- Draft PR: `https://github.com/ripDZL/EqualizerAPO-XT/pull/1`.
- CI fixes: `e7e5e65`, `c3f0d4e`.
- Fork `main` synced to upstream `115dkk/main` at `a19f777` on 2026-07-21.
- Fork `main` synced to upstream `115dkk/main` at `4aaeddd` on 2026-07-26.
- Review/fix branch: `codex/upstream-main-review-fixes-20260726`.
- Review/fix commit: `5ecf01a`.
- Branch policy: keep `main`/master as stable and `beta` as integration; consolidate usable branch work into `beta`.
- `beta` pushed at `7eca352` on 2026-07-26.
- Fixes: VST param float parsing, Qt artifact cleanup/exclusions, VS env var casing, HybridConvTests Debug whole-archive.
- Old feature branch `agent/vst3-host-editor-compatibility` is superseded; direct merge hit VST3 conflicts and should be avoided.
- Beta contains upstream VST3/editor port commits plus the remaining factory host explicit `queryInterface` cleanup from old commit `c3f0d4e`.
- Validation after `7eca352`: Common/TestVst3Plugin/HybridConvTests rebuild with `v143`; `HybridConvTests.exe` passed 1635 checks including 62 VST3 host checks.
- GitHub Actions run `30209856962` on `beta` at `7eca352` passed all normal workflow gates and produced x64/ARM64 artifacts.
- Current install bundle: `artifacts/EqualizerAPO-XT-x64-avx2-a19f777-dragdrop`.
- Current install zip: `artifacts/EqualizerAPO-XT-x64-avx2-a19f777-dragdrop.zip`.
- Current beta GitHub artifact: `artifacts/github-run-30209856962/EqualizerAPO-x64-avx2`.
- Build note: repo targets `v145`; local build used VS 2022 `v143` override plus Community ATL.
- 2026-07-28 live analyzer build: `build-Editor-x64-live-preview/release/Editor.exe`.
- 2026-07-28 validation: Qt Editor x64 AVX2 build passed; Editor `--selftest-vst` passed; `HybridConvTests.exe` passed 1623 checks including VST2/VST3 host tests; `EditorLogicTests.exe` passed 2520 checks; `EngineOrchestrationTests.exe` passed 617 checks.
- 2026-07-28 validation note: `AudioRegressionTests.exe` could not complete locally because reference `.raw` files are missing from `x64/Release/references`.

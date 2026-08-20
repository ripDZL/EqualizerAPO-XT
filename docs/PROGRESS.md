# Progress

- [x] Fetched upstream v2.39.0 (`ff9c174`).
- [x] Evaluated a direct `beta` merge; 149 conflicts made it unsuitable.
- [x] Created local upstream-first feature-port branch.
- [x] Ported VST live analyzer preview and microphone-source support.
- [x] Ported selected EAPO-endpoint preview selection and regression coverage.
- [x] Ported row-scoped `Device:` resolution for VST preview capture.
- [x] Ported VST2 transport-time/process-level safety with a red/green host test.
- [x] Ported block-paced VST preview processing with join-before-stop lifecycle ordering.
- [x] Ported saved custom-theme persistence, token derivation, and JSON round-trips.
- [x] Built Editor and passed offscreen `Editor.exe --selftest-vst`.
- [x] Built and passed `EditorLogicTests.exe` (3073 checks).
- [x] Built and passed `HybridConvTests.exe` (1635 checks).
- [ ] Manually validate real VST2/VST3 analyzer movement before an integration-candidate install.
- [ ] Wire saved custom themes into SkinManager and the Theme Lab UI.
- [ ] Port remaining fork-only features with focused tests.
- [ ] Build, CI, and manual-host-test an integration candidate before any beta promotion.

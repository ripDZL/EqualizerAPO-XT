# Progress

- [x] Fetched upstream v2.39.0 (`ff9c174`).
- [x] Evaluated a direct `beta` merge; 149 conflicts made it unsuitable.
- [x] Created local upstream-first feature-port branch.
- [x] Ported VST live analyzer preview and microphone-source support.
- [x] Ported selected EAPO-endpoint preview selection and regression coverage.
- [x] Ported row-scoped `Device:` resolution for VST preview capture.
- [x] Ported VST2 transport-time/process-level safety with a red/green host test.
- [x] Built Editor and passed `Editor.exe --selftest-vst`.
- [x] Built and passed `EditorLogicTests.exe` (3069 checks).
- [x] Built and passed `HybridConvTests.exe` (1635 checks).
- [ ] Audit remaining VST preview hardening separately from unrelated legacy skin changes.
- [ ] Port remaining fork-only features with focused tests.
- [ ] Build, CI, and manual-host-test an integration candidate before any beta promotion.

# AI Context

- Local integration branch: `codex/upstream-289-port`.
- Base: upstream `main` at `ff9c1747614546b68418a5376d0e5f893babd130` (v2.39.0).
- A direct merge from `beta` produced 149 conflicts; port fork-only features onto current upstream instead.
- Preserve upstream VST3 `Input`/`Output` bus-layout controls while porting VST preview work.
- The separately built VST2 no-native-editor safety fix (`9c9e95d6`) is not part of this branch and is not promoted.
- Ported here: Editor-side VST live analyzer preview, microphone sources, and selected-endpoint selection.

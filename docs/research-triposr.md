# Research: TripoSR for Canal Prop Blocking

Related issue: `3VI-53`

Paper:

- TripoSR (arXiv 2024-03): <https://arxiv.org/abs/2403.02151>

## Why It Fits This Project

- It can convert a single reference image to a rough mesh quickly.
- That is useful for early prop blocking in the Canal pipeline before final Blender retopo/UV/bake.
- It is best treated as a prototype accelerator, not final content generation.

## Recommended Experiment (5 Props)

Goal: measure whether TripoSR reduces time-to-UE-ready proxy assets.

Input set (5 references):

1. bollard
2. canal ring
3. bench
4. lamp post
5. waste bin

Process per prop:

1. Run TripoSR inference from one concept/reference image.
2. Import mesh into Blender.
3. Perform minimal cleanup:
   - scale/origin/pivot fix
   - topology cleanup for stable normals
   - basic UV unwrap
4. Export FBX/GLTF and import into UE.
5. Validate collisions + pivot behavior in engine.

## What To Measure

Per prop record:

- `time_inference_min`
- `time_blender_cleanup_min`
- `time_ue_import_and_collision_min`
- `triangle_count_raw`
- `triangle_count_clean`
- `usable_without_major_rebuild` (`yes/no`)

Suggested acceptance threshold for “worth adopting”:

- median end-to-end time <= 20 minutes per usable proxy prop
- at least 4/5 props usable without full remodel

## Risks and Mitigations

- Geometry artifacts/noise: keep to proxy scope; do not use raw output directly in shipping assets.
- Inconsistent scale/pivots: enforce Blender import/export conventions as fixed checklist.
- Style mismatch: use only as blockout source; final art pass remains in existing DCC pipeline.

## Decision

Current recommendation:

- Proceed with the 5-prop experiment in a separate branch/workspace.
- If thresholds above are met, adopt TripoSR only for prototype props in M1/M2.
- Keep final production assets on the standard Blender/Substance workflow.


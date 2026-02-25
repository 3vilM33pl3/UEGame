# Hex WFC Batch Harness

`UCanalWfcBlueprintLibrary::RunHexWfcBatch(...)` runs many seeds and returns aggregate solve stats.

## API

- Header: `Source/UEGame/Public/CanalGen/HexWfcSolver.h`
- Implementation: `Source/UEGame/Private/CanalGen/HexWfcSolver.cpp`

## Inputs

- `FHexWfcSolveConfig`:
  - `MaxAttempts`
  - `MaxSolveTimeSeconds` (per-seed time limit)
  - `BiomeProfile`
  - `BiomeWeightMultipliers` (tile ID + multiplier)
- `FHexWfcBatchConfig`:
  - `StartSeed`
  - `NumSeeds`
  - `MaxBatchTimeSeconds` (whole batch time limit)

## Output Stats

`FHexWfcBatchStats` includes:

- solved/failed counts
- contradiction and time-budget counts/rates
- single-water-component validation failure count
- average attempts and solve time
- attempt histogram (`AttemptHistogram`)
- tile frequency histogram (`TileHistogram`)

Use tile histogram + profile-specific multipliers to validate biome bias behavior across seed ranges.

## Headless Batch Commandlet

Use `CanalWfcBatch` commandlet for CLI-style batch runs with report export.

### Example

```bash
/home/olivier/Projects/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd \
  /home/olivier/Projects/CanalGame/UEGame/UEGame.uproject \
  -run=CanalWfcBatch \
  -GridWidth=16 -GridHeight=12 \
  -StartSeed=1 -NumSeeds=1000 \
  -MaxAttempts=8 -MaxPropagationSteps=100000 \
  -RequireEntryExitPath=true \
  -RequireSingleWaterComponent=true \
  -AutoSelectBoundaryPorts=true \
  -DisallowUnassignedBoundaryWater=true \
  -OutputDir=/tmp/wfc_reports -OutputPrefix=m1_hex \
  -log
```

### Outputs

- `<prefix>_<timestamp>.json`
- `<prefix>_<timestamp>.csv`

Files include solve success metrics, attempts/time aggregates, and histograms.

### Recommended Wrapper

Use `scripts/run_wfc_batch.sh` to run commandlet batches robustly in this repo:

```bash
./scripts/run_wfc_batch.sh --output-prefix m1_hex -- \
  -GridWidth=16 -GridHeight=12 \
  -NumSeeds=1000 \
  -RequireEntryExitPath=true \
  -RequireSingleWaterComponent=true
```

The wrapper returns success when JSON/CSV reports are produced, even if Unreal exits
non-zero due unrelated asset-registry errors in project content.

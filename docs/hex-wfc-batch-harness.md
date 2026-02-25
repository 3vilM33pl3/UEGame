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
- average attempts and solve time
- attempt histogram (`AttemptHistogram`)
- tile frequency histogram (`TileHistogram`)

Use tile histogram + profile-specific multipliers to validate biome bias behavior across seed ranges.

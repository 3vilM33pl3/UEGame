# Canal Topology Tile Schema (M1)

This document defines the canonical socket schema and rotation rules for hex topology tiles.

## Direction Indexing

All tile socket arrays use this direction order (index `0..5`):

1. `0` = `East`
2. `1` = `NorthEast`
3. `2` = `NorthWest`
4. `3` = `West`
5. `4` = `SouthWest`
6. `5` = `SouthEast`

Neighbor offsets in axial coordinates (`q`, `r`):

- `East`: `( +1,  0)`
- `NorthEast`: `( +1, -1)`
- `NorthWest`: `(  0, -1)`
- `West`: `( -1,  0)`
- `SouthWest`: `( -1, +1)`
- `SouthEast`: `(  0, +1)`

## Socket Enum

Use `ECanalSocketType`:

- `Water`
- `Bank`
- `TowpathL`
- `TowpathR`
- `Lock`
- `Road`

Adjacency rule in M1: two touching edges are compatible only when socket types are equal.

## Rotation Rules

- Rotation is **clockwise**, in 60-degree steps.
- Valid rotation steps: `0..5`.
- For world direction `d` and clockwise rotation `r`, the effective base socket index is:

`baseIndex = (d - r + 6) % 6`

This is implemented in `FCanalTopologyTileDefinition::GetSocket(...)`.

## Tile Definition Contract

Each tile definition must satisfy:

- `TileId` is set (`FName`, non-empty).
- Exactly 6 sockets are provided.
- `Weight >= 0`.

Validation is implemented by `FCanalTopologyTileDefinition::EnsureValid(...)`.

## Prototype Tile Set v0

A code-defined v0 set is available via:

- `FCanalPrototypeTileSet::BuildV0()`

Includes 13 starter tiles:

- `water_straight_ew`
- `water_straight_nesw`
- `water_straight_nwse`
- `water_bend_gentle`
- `water_bend_hard`
- `water_t_junction`
- `water_cross`
- `water_dead_end`
- `lock_gate`
- `towpath_left`
- `towpath_right`
- `road_crossing`
- `solid_bank`

## Compatibility Cache

Use `FCanalTileCompatibilityTable` to precompute legal adjacencies.

- Build once from tile definitions.
- Query compatible variants by `(tileIndex, rotation, direction)`.
- Use `DescribeCompatibility(...)` for debug inspection.

## WFC Solver Baseline (M1)

`FHexWfcSolver` currently provides:

- Lowest-entropy cell selection.
- Deterministic tie-break (row-major `r,q`).
- Weighted candidate selection (tile `Weight`) with deterministic seed.
- Constraint propagation across neighbors.
- Restart policy via `MaxAttempts` in `FHexWfcSolveConfig`.
- Optional Entry/Exit validation:
  - `bRequireEntryExitPath`
  - `EntryPort` and `ExitPort` (`coord + boundary-facing direction`)
  - `bAutoSelectBoundaryPorts` to auto-pick a valid pair when ports are not fully specified
- Optional global connected-water validation:
  - `bRequireSingleWaterComponent`
- Boundary socket policy:
  - `bDisallowUnassignedBoundaryWater` rejects boundary water sockets unless they are:
    - one of the resolved Entry/Exit ports, or
    - explicitly allowed by tile (`bAllowAsBoundaryPort`)

This baseline directly supports:

- contradiction handling / retry policy
- entry/exit boundary constraints
- connected-water validation checks
- boundary half-segment prevention

## Asset Authoring Guidance

For production content, create `UCanalTopologyTileSetAsset` assets and fill `Tiles` with the same schema.
Use the debug compatibility description to verify authored tiles before running WFC.

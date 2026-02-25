# Canal Topology Generator Actor (M1)

`ACanalTopologyGeneratorActor` provides an end-to-end in-editor generator for:

- solving hex topology with WFC
- instantiating topology with HISM components
- generating a navigable water spline path
- exposing generation metadata (master/topology/dressing seeds, biome profile, resolved Entry/Exit ports, spline points)

## Class

- `Source/UEGame/Public/CanalGen/CanalTopologyGeneratorActor.h`
- `Source/UEGame/Private/CanalGen/CanalTopologyGeneratorActor.cpp`

## Components

- `WaterInstances` (`UHISM`)
- `BankInstances` (`UHISM`)
- `TowpathInstances` (`UHISM`)
- `LockInstances` (`UHISM`)
- `RoadInstances` (`UHISM`)
- `WaterPathSpline` (`USplineComponent`)

## Typical Usage

1. Place `CanalTopologyGeneratorActor` in a level.
2. Assign a `UCanalTopologyTileSetAsset` to `TileSet`.
3. Set grid and solve options (`GridConfig`, `SolveConfig`) and determinism option (`bDeriveSeedStreamsFromMaster`).
4. Click `GenerateTopology` (CallInEditor).
5. Inspect generated instances and `WaterPathSpline`.
6. Inspect `LastGenerationMetadata` for derived stream seeds, biome profile, resolved ports, and spline point count.

## Notes

- If no custom meshes are assigned, the actor uses engine cube mesh fallback.
- When `bDeriveSeedStreamsFromMaster=true`, topology and dressing seeds are derived deterministically from `SolveConfig.Seed`.
- Spline uses resolved entry/exit ports when available; otherwise it uses a longest-path heuristic over water cells.
- Spline points are generated as `CurveClamped` to smooth channel corners.
- Enable `bDrawPortDebug` to visualize entry (green) and exit (red) port markers in-world.
- Enable `bDrawGridDebug` to draw hex outlines and per-cell tile labels (`tileId` + rotation).
- Enable `bDrawSemanticOverlay` to draw semantic edge overlays:
  - water channels
  - towpath left/right edges
  - lock edges
  - road edges
- Use `bAllowSemanticOverlayInDatasetCapture=false` (default) to keep overlays out of dataset capture passes.
- Use `ClearGenerated` to reset all generated instances/spline.

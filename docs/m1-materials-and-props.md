# M1 Prototype Materials + Towpath Props

This document covers:

- `3VI-263` (`CCV-014`) prototype water/bank/towpath material behavior.
- `3VI-265` (`CCV-016`) spawnable towpath prop set + procedural hooks.

## What Landed

Implementation file:

- `Source/UEGame/Private/CanalGen/CanalTopologyGeneratorActor.cpp`
- `Source/UEGame/Public/CanalGen/CanalTopologyGeneratorActor.h`

Key additions:

1. Prototype material profiles:
   - `WaterMaterialProfile`
   - `BankMaterialProfile`
   - `TowpathMaterialProfile`
2. Seed-driven material variation:
   - deterministic per generation seed
   - exposes `Tint` + `Wetness` with jitter controls
   - records resolved values in:
     - `LastWaterMaterialRuntime`
     - `LastBankMaterialRuntime`
     - `LastTowpathMaterialRuntime`
3. Towpath prop system:
   - 8 semantic prop types: `bollard`, `ring`, `sign`, `lamp`, `bench`, `reeds`, `bin`, `fence`
   - per-type mesh + scale + weight config in `TowpathPropDefinitions`
   - deterministic random spawning on towpath sockets
   - coverage-first pass ensures all 8 semantic types appear when candidates are available
4. Blueprint/query helpers:
   - `GetTotalTowpathPropCount()`
   - `GetTowpathPropCountByTag(Tag)`
   - `GetTowpathPropSemanticTags(...)`

## Automated Validation

These tests are now part of `UEGame.Canal.M1`:

- `UEGame.Canal.M1.MaterialProfiles`
- `UEGame.Canal.M1.TowpathProps`

Run:

```bash
/home/olivier/Projects/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd \
  /home/olivier/Projects/CanalGame/UEGame/UEGame.uproject \
  -unattended -nullrhi -nosound -NoSplash \
  -ExecCmds="Automation RunTests UEGame.Canal.M1;Quit" \
  -TestExit="Automation Test Queue Empty" -log
```

## Manual Sanity Pass (Editor)

1. Select `CanalTopologyGeneratorActor` in `StartMap`.
2. Confirm:
   - `bSpawnTowpathProps = true`
   - `TowpathPropDensity` > `0`
3. Click `GenerateTopology`.
4. Verify:
   - towpath now contains mixed semantic props
   - water/bank/towpath material look is visually distinct
   - changing `SolveConfig.Seed` changes the resolved tint/wetness variation

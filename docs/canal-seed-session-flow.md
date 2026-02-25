# Seed Session Flow (M1)

`UCanalSeedSessionComponent` provides a Blueprint-callable flow for:

- Generate new seed
- Generate using manual seed input
- Regenerate current seed
- Persist and reload recent seed history locally

## Files

- `Source/UEGame/Public/CanalGen/CanalSeedSessionComponent.h`
- `Source/UEGame/Private/CanalGen/CanalSeedSessionComponent.cpp`
- `Source/UEGame/Public/CanalGen/CanalSeedSessionSaveGame.h`

## Button Mappings

Use these methods directly from UI buttons:

- Generate new seed: `GenerateNewSeed(...)`
- Manual seed entry: `GenerateSeedFromInput(ManualSeed, ...)`
- Regenerate current: `RegenerateCurrentSeed(...)`

## Persistence

The component stores history to a `USaveGame` slot:

- Slot name: `SaveSlotName` (default `CanalSeedSession`)
- User index: `SaveUserIndex` (default `0`)
- `RecentSeeds` (deduped, newest first)
- `CurrentSeed`

The list is trimmed to `MaxRecentSeeds` and restored on begin play when `bAutoLoadOnBeginPlay=true`.

## Integration Notes

- Assign `TopologyGeneratorActor` (or enable auto-find) to auto-apply seeds to `ACanalTopologyGeneratorActor::SolveConfig.Seed`.
- Set `bRegenerateTopology=true` when button actions should immediately call `GenerateTopology`.

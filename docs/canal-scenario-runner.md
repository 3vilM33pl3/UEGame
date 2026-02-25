# Canal Scenario Runner (M1)

`UCanalScenarioRunnerComponent` standardizes the capture lifecycle:

- `SetupScenario(seed)`
- `BeginCapture()`
- `EndCapture()`

It is designed for Blueprint scenarios and records scenario metadata on `ACanalTopologyGeneratorActor`.

## Files

- `Source/UEGame/Public/CanalGen/CanalScenarioInterface.h`
- `Source/UEGame/Public/CanalGen/CanalScenarioRunnerComponent.h`
- `Source/UEGame/Private/CanalGen/CanalScenarioRunnerComponent.cpp`

## Interface Contract

Implement `UCanalScenarioInterface` on your scenario actor (or owning actor):

- `SetupScenario(int32 Seed)`
- `BeginCapture()`
- `EndCapture()`
- `GetScenarioRequest() -> FCanalScenarioRequest`

`FCanalScenarioRequest` includes:

- `ScenarioName`
- `RequestedDurationSeconds`
- `bUseGeneratedWaterSpline`
- `RequestedCameraPathPoints`

## Typical Setup

1. Add `CanalScenarioRunnerComponent` to an actor.
2. Set `ScenarioActor` to any actor implementing `UCanalScenarioInterface` (or keep auto-owner mode).
3. Set `TopologyGeneratorActor` to your `ACanalTopologyGeneratorActor` (or keep auto-find enabled).
4. Call `RunScenario(Seed, bRegenerateTopology)` from UI, input, or automation.
5. Call `GetEffectiveCameraPathPoints(...)` to resolve camera path points:
   - scenario-provided path first
   - generated water spline fallback

## Metadata

On scenario start, the runner writes to `ACanalTopologyGeneratorActor::LastGenerationMetadata`:

- `ScenarioName`
- `ScenarioDurationSeconds`

This keeps capture metadata aligned with the active scenario and seed.

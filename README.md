# Canal Game (UEGame)

Unreal Engine project for CanalCV experiments, with a minimal external CLI scaffold.

## Repository Layout

- `Config/` Unreal project configuration (`DefaultEngine.ini`, `DefaultGame.ini`).
- `Content/` Unreal assets and maps.
- `Plugins/` project plugins.
- `Source/` C++ game module code.
- `scripts/` local automation (for example Steam Deck deploy and editor startup).
- `External/` external tooling workspace (currently a minimal Rust CLI).

## Unreal: Open and Run

1. Open `UEGame.uproject` in Unreal Editor.
2. Start Play In Editor (PIE) from the editor.
3. Project defaults are configured to open `/Game/StartMap.StartMap`.

## UnrealCV Integration Baseline (`3VI-251`)

- Unreal Engine baseline: `5.7.2` (`/home/olivier/Projects/UnrealEngine/Engine/Build/Build.version`).
- UnrealCV source baseline: `unrealcv/unrealcv` branch `5.2`, commit `8ec7a8cf389c0e2eb6658d5fe64806ad22e586d3`.
- Plugin is vendored at `Plugins/UnrealCV/` and enabled in `UEGame.uproject`.

Validation commands used:

```bash
/home/olivier/Projects/UnrealEngine/Engine/Build/BatchFiles/Linux/Build.sh UEGameEditor Linux Development -Project=/home/olivier/Projects/CanalGame/UEGame/UEGame.uproject -WaitMutex -NoHotReloadFromIDE
```

```bash
/home/olivier/Projects/UnrealEngine/Engine/Binaries/Linux/UnrealEditor /home/olivier/Projects/CanalGame/UEGame/UEGame.uproject -game -unattended -nullrhi -nosound -NoSplash -ExecCmds="vget /unrealcv/status,quit" -log
```

Notes:
- `vget /unrealcv/status` is accepted and returns server status/config in logs.
- If TCP port `9000` is already occupied on your machine, set another UnrealCV port (see Steam Deck section below or use `-cvport=<port>`).
- Quick day-to-day guide: `docs/unrealcv-quickstart.md`.
- One-command smoke test: `./scripts/unrealcv_smoke_test.sh`.
- Full manual test checklist: `docs/manual-testing-unrealcv.md`.

If you use local helper scripts:

```bash
./scripts/start_editor.sh
```

## External CLI (Rust)

The first external scaffold is under `External/`:

```bash
cd External
cargo build
cargo run -- --host 127.0.0.1 --port 9000 --output ./output
```

Current behavior:
- Parses host/port/output args.
- Creates the output directory if needed.
- Prints effective configuration.

## Steam Deck Deployment (Linux Build)

Use the local deploy script to package, sync, and optionally launch on Steam Deck:

```bash
./scripts/deploy_steamdeck.sh
```

Defaults:
- `CONFIG=Shipping`
- `PLATFORM=Linux`
- `STEAMDECK_USER=deck`
- `VERIFY_WRITABLE_PATHS=1` (checks `Saved/Logs` and `Saved/Exports` on Deck)
- `RUN_AFTER_DEPLOY=0` (set to `1` to launch immediately after sync)

Common overrides:

```bash
CONFIG=Shipping STEAMDECK_HOST=steamdeck STEAMDECK_DEST=~/Games/UEGame ./scripts/deploy_steamdeck.sh
RUN_AFTER_DEPLOY=1 ./scripts/deploy_steamdeck.sh
VERIFY_SAVE_ROOT=~/Games/UEGame/UEGame/SavedCustom VERIFY_LOG_DIR=~/Games/UEGame/UEGame/SavedCustom/Logs VERIFY_EXPORT_DIR=~/Games/UEGame/UEGame/SavedCustom/Exports ./scripts/deploy_steamdeck.sh
```

Runtime launcher hardening (`UEGame.sh`):
- Sources optional `UEGame.env` from the deployment root.
- Supports configurable save/log/export roots (`UE_SAVE_ROOT`, `UE_LOG_DIR`, `UE_EXPORT_DIR`).
- Writes timestamped logs and updates `Logs/UEGame-latest.log`.
- Uses `-forcelogflush` and explicit `-abslog`.
- Passes `-cvport=$UNREALCV_PORT` to the game when `UNREALCV_PORT` is set.
- Can auto-restart on network route changes (`UE_NETWORK_RESTART_ON_CHANGE=1`).
- Can auto-restart when UnrealCV port is not listening (`UNREALCV_PORT`, `UE_UNREALCV_PORT_RESTART=1`).

## Conventions

- Keep Unreal runtime code in `Source/UEGame`.
- Keep project-wide runtime settings in `Config/`.
- Keep automation scripts in `scripts/`.
- Keep non-Unreal tools and clients under `External/`.
- Tile schema / hex + WFC generator notes: `docs/canal-topology-tile-schema.md`.
- Prototype tile set catalog and diagrams: `docs/canal-topology-tiles-v0.md`.
- Topology generator actor usage: `docs/canal-topology-generator-actor.md`.
- Scenario runner and capture lifecycle hooks: `docs/canal-scenario-runner.md`.
- Seed session flow and local seed history: `docs/canal-seed-session-flow.md`.
- WFC batch harness + CLI reports: `docs/hex-wfc-batch-harness.md` and `scripts/run_wfc_batch.sh`.

## Platform Validation Notes

- Linux validation is performed locally.
- Windows validation for the Rust CLI is intentionally deferred in this pass and should be tracked as follow-up work.

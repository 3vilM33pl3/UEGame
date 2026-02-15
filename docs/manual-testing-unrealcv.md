# Manual Test Plan: UnrealCV Integration

This checklist validates UnrealCV integration for `3VI-251` and gives a repeatable path for local and Steam Deck verification.

## Scope

- Verify UnrealCV plugin loads in editor and packaged game.
- Verify at least one UnrealCV command works (`vget /unrealcv/status`).
- Verify TCP connectivity from an external client machine.

## Test Environments

- Dev PC: Ubuntu/Linux with UE `5.7.2` and project at `UEGame.uproject`.
- Steam Deck: packaged Linux build deployed with `scripts/deploy_steamdeck.sh`.

## Preconditions

- UnrealCV plugin exists at `Plugins/UnrealCV` and project compiles.
- UnrealCV port chosen and free on target host (default `9000`).
- Deck and client PC are on reachable network/VPN.

## Test Case 1: Editor Build Validation

1. Run:
   ```bash
   /home/olivier/Projects/UnrealEngine/Engine/Build/BatchFiles/Linux/Build.sh UEGameEditor Linux Development -Project=/home/olivier/Projects/CanalGame/UEGame/UEGame.uproject -WaitMutex -NoHotReloadFromIDE
   ```
2. Expected:
   - Build succeeds.
   - `libUnrealEditor-UnrealCV.so` is linked.

## Test Case 2: PIE Command Validation (Manual In-Editor)

1. Launch editor:
   ```bash
   ./scripts/start_editor.sh
   ```
2. Open map `/Game/StartMap.StartMap`.
3. Enter PIE.
4. Open in-game console with `` ` ``.
5. Run command:
   ```text
   vget /unrealcv/status
   ```
6. Expected:
   - Console returns status text including `Is Listening`.
   - Status includes config path and current port.

## Test Case 3: Headless Smoke Command Validation

1. Run:
   ```bash
   /home/olivier/Projects/UnrealEngine/Engine/Binaries/Linux/UnrealEditor /home/olivier/Projects/CanalGame/UEGame/UEGame.uproject -game -unattended -nullrhi -nosound -NoSplash -ExecCmds="vget /unrealcv/status,quit" -log
   ```
2. If port `9000` is in use, rerun with another port:
   ```bash
   /home/olivier/Projects/UnrealEngine/Engine/Binaries/Linux/UnrealEditor /home/olivier/Projects/CanalGame/UEGame/UEGame.uproject -game -unattended -nullrhi -nosound -NoSplash -cvport=9010 -ExecCmds="vget /unrealcv/status,quit" -log
   ```
3. Expected log signals:
   - `LogPluginManager: Mounting Project plugin UnrealCV`
   - `LogUnrealCV: ... Loading config`
   - `LogUnrealCV: ... Is Listening`

## Test Case 4: Steam Deck Packaged Build + Port Availability

1. Deploy with explicit UnrealCV port in env file on Deck:
   - In `UEGame.env` set:
     ```bash
     UNREALCV_PORT=9000
     UE_UNREALCV_PORT_RESTART=1
     ```
2. Deploy and run:
   ```bash
   RUN_AFTER_DEPLOY=1 ./scripts/deploy_steamdeck.sh
   ```
3. Expected:
   - Launcher output includes selected `unrealcv_port`.
   - `ss -lnt sport = :9000` on Deck shows listener.
   - No restart loop due port check.

## Test Case 5: External Client Connectivity (PC -> Deck)

From client machine:

1. Verify TCP connectivity:
   ```bash
   nc -vz <deck-ip-or-hostname> 9000
   ```
2. Install official UnrealCV Python client:
   ```bash
   python3 -m pip install unrealcv
   ```
3. Send a command:
   ```bash
   python3 - <<'PY'
from unrealcv import Client
client = Client(('<deck-ip-or-hostname>', 9000))
client.connect()
print(client.isconnected())
print(client.request('vget /unrealcv/status'))
client.disconnect()
PY
   ```
4. Expected:
   - Connection succeeds.
   - Response includes UnrealCV status text (`Is Listening`, port, config path).

## Test Case 6: Sleep/Wake Networking Regression (Deck)

1. Start packaged build on Deck and confirm UnrealCV port is listening.
2. Put Deck to sleep, then wake.
3. Wait for launcher watchdog interval.
4. Expected:
   - Game remains healthy or restarts automatically if route/port check fails.
   - UnrealCV port becomes reachable again from client machine.

## Pass Criteria

- All tests above pass, or any failures have a documented cause and mitigation.
- At least one command from PIE and one command from external client are confirmed.

## Evidence to Capture

- Build log snippet showing UnrealCV module link.
- PIE console screenshot (`vget /unrealcv/status`).
- Deck launcher log with `unrealcv_port` and watchdog behavior.
- External client command output.

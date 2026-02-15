# UnrealCV Quickstart (Integrate, Test, Use)

This project already has UnrealCV integrated as a project plugin:

- Plugin source: `Plugins/UnrealCV/`
- Plugin enabled: `UEGame.uproject` (`"Name": "UnrealCV", "Enabled": true`)

Use this guide for day-to-day validation and client usage.

## 1. Quick Smoke Test (Headless)

Run:

```bash
./scripts/unrealcv_smoke_test.sh
```

Expected:

- Script exits `0`.
- Output contains UnrealCV startup lines like:
  - `Mounting Project plugin UnrealCV`
  - `Start listening on 9000` (or your configured port)

You can pick another port:

```bash
./scripts/unrealcv_smoke_test.sh --port 9010
```

## 2. In-Editor Command Test

1. Start editor:
   ```bash
   ./scripts/start_editor.sh
   ```
2. Enter PIE.
3. Open console with `` ` ``.
4. Run:
   ```text
   vget /unrealcv/status
   ```

Expected: status text including config path and listening state.

## 3. Use UnrealCV from a Client (Python)

Install client:

```bash
python3 -m pip install unrealcv
```

Query server (local example):

```bash
python3 - <<'PY'
from unrealcv import Client

client = Client(("127.0.0.1", 9000))
client.connect()
print("connected:", client.isconnected())
print(client.request("vget /unrealcv/status"))
print(client.request("vget /camera/0/location"))
client.disconnect()
PY
```

If your game runs on another host (for example Steam Deck), replace host/port.

## 4. Steam Deck Notes

If using `scripts/deploy_steamdeck.sh`, set UnrealCV port in `UEGame.env` on Deck:

```bash
UNREALCV_PORT=9000
UE_UNREALCV_PORT_RESTART=1
UE_UNREALCV_PORT_GRACE_SECONDS=30
```

Then deploy:

```bash
RUN_AFTER_DEPLOY=1 ./scripts/deploy_steamdeck.sh
```

From your client machine, test connectivity:

```bash
nc -vz <deck-ip> 9000
```

## 5. Common Commands You’ll Use

- `vget /unrealcv/status`
- `vget /unrealcv/version`
- `vget /cameras`
- `vget /camera/0/location`
- `vset /camera/0/location <x> <y> <z>`
- `vget /camera/0/lit png`
- `vget /camera/0/depth npy`
- `vget /camera/0/object_mask png`

## 6. Troubleshooting

- Port conflict: run with another port (`--port 9010` or `-cvport=9010`).
- `DerivedDataCache` write errors:
  - Ensure your UE install/cache paths are writable by your current user.
  - Re-run the smoke test after fixing permissions.
- No response from client:
  - Verify game/editor is running.
  - Verify `ss -lnt sport = :9000` on host.
  - Verify firewall/network path.
- Plugin not loading:
  - Confirm `Plugins/UnrealCV` exists.
  - Confirm `UEGame.uproject` still has `"UnrealCV"` enabled.

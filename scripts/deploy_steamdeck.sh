#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_FILE="${PROJECT_ROOT}/UEGame.uproject"
PROJECT_GAME_INI="${PROJECT_ROOT}/Config/DefaultGame.ini"

UE_ROOT="${UE_ROOT:-/home/olivier/Projects/UnrealEngine}"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/../Builds/SteamDeck}"
CONFIG="${CONFIG:-Shipping}"
PLATFORM="${PLATFORM:-Linux}"
AUTO_BUMP_PATCH="${AUTO_BUMP_PATCH:-1}"
VERIFY_WRITABLE_PATHS="${VERIFY_WRITABLE_PATHS:-1}"
RUN_AFTER_DEPLOY="${RUN_AFTER_DEPLOY:-0}"

STEAMDECK_HOST="${STEAMDECK_HOST:-steamdeck}"
STEAMDECK_USER="${STEAMDECK_USER:-deck}"
STEAMDECK_DEST="${STEAMDECK_DEST:-~/Games/UEGame}"
VERIFY_SAVE_ROOT="${VERIFY_SAVE_ROOT:-${STEAMDECK_DEST}/UEGame/Saved}"
VERIFY_LOG_DIR="${VERIFY_LOG_DIR:-${VERIFY_SAVE_ROOT}/Logs}"
VERIFY_EXPORT_DIR="${VERIFY_EXPORT_DIR:-${VERIFY_SAVE_ROOT}/Exports}"

if [[ "${STEAMDECK_HOST}" == *"@"* ]]; then
  STEAMDECK_REMOTE="${STEAMDECK_HOST}"
else
  STEAMDECK_REMOTE="${STEAMDECK_USER}@${STEAMDECK_HOST}"
fi

DOTNET="${UE_ROOT}/Engine/Binaries/ThirdParty/DotNet/8.0.412/linux-x64/dotnet"
AUTOMATION_DIR="${UE_ROOT}/Engine/Binaries/DotNET/AutomationTool"

read_project_version() {
  local project_version
  project_version="$(sed -n 's/^ProjectVersion=//p' "${PROJECT_GAME_INI}" | head -n 1 | tr -d '\r')"

  if [[ -z "${project_version}" ]]; then
    project_version="0.0.0"
  fi

  printf '%s\n' "${project_version}"
}

write_project_version() {
  local new_version="$1"
  local temp_file
  temp_file="$(mktemp)"

  awk -v new_version="${new_version}" '
    BEGIN {
      in_section = 0
      section_seen = 0
      version_set = 0
    }
    {
      if ($0 == "[/Script/EngineSettings.GeneralProjectSettings]") {
        in_section = 1
        section_seen = 1
        print
        next
      }

      if (in_section && $0 ~ /^\[/) {
        if (!version_set) {
          print "ProjectVersion=" new_version
          version_set = 1
        }
        in_section = 0
      }

      if (in_section && $0 ~ /^ProjectVersion=/) {
        if (!version_set) {
          print "ProjectVersion=" new_version
          version_set = 1
        }
        next
      }

      print
    }
    END {
      if (in_section && !version_set) {
        print "ProjectVersion=" new_version
        version_set = 1
      }

      if (!section_seen) {
        if (NR > 0) {
          print ""
        }
        print "[/Script/EngineSettings.GeneralProjectSettings]"
        print "ProjectVersion=" new_version
      } else if (!version_set) {
        print "ProjectVersion=" new_version
      }
    }
  ' "${PROJECT_GAME_INI}" > "${temp_file}"

  chmod --reference="${PROJECT_GAME_INI}" "${temp_file}"
  mv "${temp_file}" "${PROJECT_GAME_INI}"
}

bump_project_patch_version() {
  local current_version
  current_version="$(read_project_version)"

  if [[ ! "${current_version}" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
    echo "ProjectVersion must be in x.y.z format, got: ${current_version}" >&2
    exit 1
  fi

  local major="${BASH_REMATCH[1]}"
  local minor="${BASH_REMATCH[2]}"
  local patch="${BASH_REMATCH[3]}"
  local new_version="${major}.${minor}.$((patch + 1))"

  write_project_version "${new_version}"
  printf '%s\n' "${new_version}"
}

ORIGINAL_VERSION=""
VERSION_BUMPED=0
restore_version_on_failure() {
  local exit_code=$?

  if [[ ${exit_code} -ne 0 && "${VERSION_BUMPED}" == "1" && -n "${ORIGINAL_VERSION}" ]]; then
    echo "Release failed. Restoring ProjectVersion=${ORIGINAL_VERSION}" >&2
    write_project_version "${ORIGINAL_VERSION}" || true
  fi
}
trap restore_version_on_failure EXIT

if [[ ! -f "${PROJECT_FILE}" ]]; then
  echo "Project not found: ${PROJECT_FILE}" >&2
  exit 1
fi

if [[ ! -f "${PROJECT_GAME_INI}" ]]; then
  echo "Game config not found: ${PROJECT_GAME_INI}" >&2
  exit 1
fi

if [[ ! -x "${DOTNET}" ]]; then
  echo "Dotnet runtime not found: ${DOTNET}" >&2
  echo "Set UE_ROOT to your UnrealEngine path." >&2
  exit 1
fi

if [[ ! -d "${AUTOMATION_DIR}" ]]; then
  echo "AutomationTool dir not found: ${AUTOMATION_DIR}" >&2
  exit 1
fi

DEPLOYED_VERSION="$(read_project_version)"
if [[ "${AUTO_BUMP_PATCH}" == "1" ]]; then
  ORIGINAL_VERSION="${DEPLOYED_VERSION}"
  DEPLOYED_VERSION="$(bump_project_patch_version)"
  VERSION_BUMPED=1
fi

if [[ ! "${DEPLOYED_VERSION}" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
  echo "ProjectVersion must be in x.y.z format, got: ${DEPLOYED_VERSION}" >&2
  exit 1
fi

echo "Release version: ${DEPLOYED_VERSION}"
echo "Build config: ${CONFIG}"

echo "Building ${PROJECT_FILE}"
pushd "${AUTOMATION_DIR}" >/dev/null
"${DOTNET}" AutomationTool.dll BuildCookRun \
  -project="${PROJECT_FILE}" \
  -noP4 \
  -platform="${PLATFORM}" \
  -clientconfig="${CONFIG}" \
  -cook -allmaps -build -stage -pak -archive \
  -archivedirectory="${BUILD_DIR}" \
  -unattended -utf8output
popd >/dev/null

LAUNCHER="${BUILD_DIR}/Linux/UEGame.sh"
BIN="${BUILD_DIR}/Linux/UEGame/Binaries/Linux/UEGame"

if [[ ! -f "${LAUNCHER}" ]]; then
  echo "Launcher not found: ${LAUNCHER}" >&2
  exit 1
fi

cat > "${LAUNCHER}" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

UE_TRUE_SCRIPT_NAME="$(readlink -f "$0")"
UE_PROJECT_ROOT="$(dirname "$UE_TRUE_SCRIPT_NAME")"

if [ -f "$UE_PROJECT_ROOT/UEGame.env" ]; then
  # Optional runtime overrides for save/log/network behavior.
  # shellcheck disable=SC1091
  . "$UE_PROJECT_ROOT/UEGame.env"
fi

UE_SAVE_ROOT_DEFAULT="$UE_PROJECT_ROOT/UEGame/Saved"
UE_SAVE_ROOT="${UE_SAVE_ROOT:-$UE_SAVE_ROOT_DEFAULT}"
UE_LOG_DIR="${UE_LOG_DIR:-$UE_SAVE_ROOT/Logs}"
UE_EXPORT_DIR="${UE_EXPORT_DIR:-$UE_SAVE_ROOT/Exports}"
UE_LOG_FILE="${UE_LOG_FILE:-$UE_LOG_DIR/UEGame-$(date +%Y%m%d-%H%M%S).log}"
UE_NETWORK_CHECK_INTERVAL="${UE_NETWORK_CHECK_INTERVAL:-5}"
UE_NETWORK_RESTART_ON_CHANGE="${UE_NETWORK_RESTART_ON_CHANGE:-1}"
UE_UNREALCV_PORT_RESTART="${UE_UNREALCV_PORT_RESTART:-1}"
UE_UNREALCV_PORT_GRACE_SECONDS="${UE_UNREALCV_PORT_GRACE_SECONDS:-30}"
UE_MAX_RESTARTS="${UE_MAX_RESTARTS:-3}"
UNREALCV_PORT="${UNREALCV_PORT:-0}"

mkdir -p "$UE_LOG_DIR" "$UE_EXPORT_DIR"
ln -sfn "$(basename "$UE_LOG_FILE")" "$UE_LOG_DIR/UEGame-latest.log" || true

# Best-effort display setup for SteamOS if Steam does not pass env
if [ -z "${WAYLAND_DISPLAY:-}" ] && [ -z "${DISPLAY:-}" ]; then
  export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/run/user/1000}
  if [ -S "$XDG_RUNTIME_DIR/wayland-0" ]; then
    export WAYLAND_DISPLAY=wayland-0
    export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-wayland}
  elif [ -S /tmp/.X11-unix/X0 ]; then
    export DISPLAY=:0
    export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-x11}
  fi
fi

chmod +x "$UE_PROJECT_ROOT/UEGame/Binaries/Linux/UEGame"

route_signature() {
  ip route get 1.1.1.1 2>/dev/null | awk '
    / dev / {
      for (i = 1; i <= NF; ++i) {
        if ($i == "dev") { dev = $(i + 1) }
        if ($i == "src") { src = $(i + 1) }
      }
    }
    END {
      if (dev != "") {
        printf "%s|%s", dev, src
      }
    }'
}

restart_launcher() {
  local reason="$1"
  shift
  local restart_count="${UE_RESTART_COUNT:-0}"
  if [ "$restart_count" -ge "$UE_MAX_RESTARTS" ]; then
    echo "UEGame launcher restart budget exhausted ($UE_MAX_RESTARTS). Last reason: $reason"
    return 1
  fi

  restart_count=$((restart_count + 1))
  export UE_RESTART_COUNT="$restart_count"
  echo "Restarting UEGame launcher ($restart_count/$UE_MAX_RESTARTS): $reason"
  exec "$UE_TRUE_SCRIPT_NAME" "$@"
}

"$UE_PROJECT_ROOT/UEGame/Binaries/Linux/UEGame" UEGame \
  -fullscreen -ResX=1280 -ResY=800 \
  -log \
  -forcelogflush \
  -abslog="$UE_LOG_FILE" \
  -UserDir="$UE_SAVE_ROOT" \
  "$@" &
UE_PID=$!

echo "UEGame started pid=$UE_PID save_root=$UE_SAVE_ROOT log_file=$UE_LOG_FILE exports_dir=$UE_EXPORT_DIR"

last_route="$(route_signature || true)"
port_grace_deadline=$(( $(date +%s) + UE_UNREALCV_PORT_GRACE_SECONDS ))

while kill -0 "$UE_PID" 2>/dev/null; do
  sleep "$UE_NETWORK_CHECK_INTERVAL"

  if [ "$UE_NETWORK_RESTART_ON_CHANGE" = "1" ]; then
    current_route="$(route_signature || true)"
    if [ -n "$last_route" ] && [ -n "$current_route" ] && [ "$current_route" != "$last_route" ]; then
      echo "Detected network route change: '$last_route' -> '$current_route'"
      kill -TERM "$UE_PID" || true
      wait "$UE_PID" || true
      restart_launcher "network route changed" "$@"
    fi
    if [ -n "$current_route" ]; then
      last_route="$current_route"
    fi
  fi

  if [ "$UE_UNREALCV_PORT_RESTART" = "1" ] && [ "$UNREALCV_PORT" != "0" ] && [ "$(date +%s)" -ge "$port_grace_deadline" ]; then
    if ! ss -lnt "sport = :$UNREALCV_PORT" 2>/dev/null | awk 'NR > 1 { found = 1 } END { exit(found ? 0 : 1) }'; then
      echo "UnrealCV port $UNREALCV_PORT not listening after grace period"
      kill -TERM "$UE_PID" || true
      wait "$UE_PID" || true
      restart_launcher "UnrealCV port unavailable" "$@"
    fi
  fi
done

wait "$UE_PID"
EOF

chmod +x "${LAUNCHER}" "${BIN}"

VERSION_FILE="${BUILD_DIR}/Linux/UEGame/version.txt"
mkdir -p "$(dirname "${VERSION_FILE}")"
printf '%s\n' "${DEPLOYED_VERSION}" > "${VERSION_FILE}"

RUNTIME_ENV="${BUILD_DIR}/Linux/UEGame.env"
if [[ ! -f "${RUNTIME_ENV}" ]]; then
  cat > "${RUNTIME_ENV}" <<'EOF'
# Optional runtime overrides for UEGame.sh on Steam Deck.
# Uncomment and customize as needed.
#
# UE_SAVE_ROOT="$HOME/Games/UEGame/UEGame/Saved"
# UE_LOG_DIR="$UE_SAVE_ROOT/Logs"
# UE_EXPORT_DIR="$UE_SAVE_ROOT/Exports"
#
# UNREALCV_PORT=9000
# UE_UNREALCV_PORT_GRACE_SECONDS=30
# UE_UNREALCV_PORT_RESTART=1
# UE_NETWORK_RESTART_ON_CHANGE=1
# UE_NETWORK_CHECK_INTERVAL=5
# UE_MAX_RESTARTS=3
EOF
fi

echo "Syncing to ${STEAMDECK_REMOTE}:${STEAMDECK_DEST}"
rsync -av --delete "${BUILD_DIR}/Linux/" "${STEAMDECK_REMOTE}:${STEAMDECK_DEST}/"
ssh "${STEAMDECK_REMOTE}" "chmod +x ${STEAMDECK_DEST}/UEGame.sh ${STEAMDECK_DEST}/UEGame/Binaries/Linux/UEGame"

if [[ "${VERIFY_WRITABLE_PATHS}" == "1" ]]; then
  echo "Verifying writable save paths on Steam Deck"
  ssh "${STEAMDECK_REMOTE}" "set -e; \
    mkdir -p ${VERIFY_LOG_DIR} ${VERIFY_EXPORT_DIR}; \
    touch ${VERIFY_LOG_DIR}/.write_test ${VERIFY_EXPORT_DIR}/.write_test; \
    rm -f ${VERIFY_LOG_DIR}/.write_test ${VERIFY_EXPORT_DIR}/.write_test"
  echo "Writable path verification passed"
fi

if [[ "${RUN_AFTER_DEPLOY}" == "1" ]]; then
  echo "Launching on Steam Deck"
  ssh "${STEAMDECK_REMOTE}" "nohup ${STEAMDECK_DEST}/UEGame.sh >/tmp/UEGame-launch.log 2>&1 &"
  echo "Launch command sent (log: /tmp/UEGame-launch.log on Steam Deck)"
fi

echo "Deployed version: ${DEPLOYED_VERSION}"
echo "Done."

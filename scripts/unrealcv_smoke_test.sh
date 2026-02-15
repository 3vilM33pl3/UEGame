#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_FILE="${PROJECT_ROOT}/UEGame.uproject"

UE_ROOT="${UE_ROOT:-/home/olivier/Projects/UnrealEngine}"
EDITOR_BIN="${EDITOR_BIN:-${UE_ROOT}/Engine/Binaries/Linux/UnrealEditor}"
PORT="${UNREALCV_PORT:-9000}"
TIMEOUT_SEC="${TIMEOUT_SEC:-180}"
LOG_FILE=""

usage() {
  cat <<'EOF'
Usage:
  ./scripts/unrealcv_smoke_test.sh [options] [-- <extra Unreal args>]

Runs a headless UnrealEditor session and validates UnrealCV startup using:
  vget /unrealcv/status

Options:
  --port <port>          UnrealCV port override (default: 9000)
  --timeout <seconds>    Process timeout (default: 180)
  --log-file <path>      Write full command output to this file
  --editor-bin <path>    Override UnrealEditor binary path
  --project <path>       Override .uproject path
  -h, --help             Show this help

Environment overrides:
  UE_ROOT, EDITOR_BIN, UNREALCV_PORT, TIMEOUT_SEC
EOF
}

EXTRA_ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      PORT="$2"
      shift 2
      ;;
    --timeout)
      TIMEOUT_SEC="$2"
      shift 2
      ;;
    --log-file)
      LOG_FILE="$2"
      shift 2
      ;;
    --editor-bin)
      EDITOR_BIN="$2"
      shift 2
      ;;
    --project)
      PROJECT_FILE="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      EXTRA_ARGS+=("$@")
      break
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ ! -f "${PROJECT_FILE}" ]]; then
  echo "Project file not found: ${PROJECT_FILE}" >&2
  exit 1
fi

if [[ ! -x "${EDITOR_BIN}" ]]; then
  echo "UnrealEditor binary not found or not executable: ${EDITOR_BIN}" >&2
  echo "Set UE_ROOT or EDITOR_BIN to your Unreal Engine path." >&2
  exit 1
fi

if [[ -z "${LOG_FILE}" ]]; then
  LOG_FILE="$(mktemp /tmp/unrealcv-smoke.XXXXXX.log)"
fi

echo "Running UnrealCV smoke test..."
echo "  project : ${PROJECT_FILE}"
echo "  editor  : ${EDITOR_BIN}"
echo "  port    : ${PORT}"
echo "  timeout : ${TIMEOUT_SEC}s"
echo "  log     : ${LOG_FILE}"

set +e
timeout "${TIMEOUT_SEC}" \
  "${EDITOR_BIN}" "${PROJECT_FILE}" \
  -game -unattended -nullrhi -nosound -NoSplash \
  "-cvport=${PORT}" \
  "-ExecCmds=vget /unrealcv/status,quit" \
  -log \
  "${EXTRA_ARGS[@]}" \
  >"${LOG_FILE}" 2>&1
cmd_status=$?
set -e

if [[ ${cmd_status} -ne 0 ]]; then
  if grep -q "Failed to write to .*DerivedDataCache" "${LOG_FILE}"; then
    echo "Detected DerivedDataCache write-permission failure." >&2
    echo "Make sure your Unreal Engine install/cache path is writable for this user." >&2
  fi
  if [[ ${cmd_status} -eq 124 ]]; then
    echo "Smoke test timed out after ${TIMEOUT_SEC}s. Check ${LOG_FILE}" >&2
  else
    echo "Smoke test command failed with code ${cmd_status}. Check ${LOG_FILE}" >&2
  fi
  exit ${cmd_status}
fi

if ! grep -q "Mounting Project plugin UnrealCV" "${LOG_FILE}"; then
  echo "UnrealCV plugin mount log not found. Check ${LOG_FILE}" >&2
  exit 1
fi

if ! grep -Eq "LogUnrealCV:.*(Is Listening|Start listening on)" "${LOG_FILE}"; then
  echo "UnrealCV listening log not found. Check ${LOG_FILE}" >&2
  exit 1
fi

echo "UnrealCV smoke test passed."
grep -E "Mounting Project plugin UnrealCV|LogUnrealCV:.*(Port:|Start listening on|Is Listening|Config file:)" "${LOG_FILE}" | tail -n 20 || true

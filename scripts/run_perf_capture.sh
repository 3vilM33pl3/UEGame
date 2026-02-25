#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_FILE="${PROJECT_ROOT}/UEGame.uproject"
UE_EDITOR_CMD="${UE_EDITOR_CMD:-/home/olivier/Projects/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd}"

MAP="${MAP:-/Game/StartMap}"
DURATION="${DURATION:-20}"
WARMUP="${WARMUP:-2}"
OUTPUT="${OUTPUT:-Saved/Reports/perf-baseline.json}"
USE_NULLRHI="${USE_NULLRHI:-0}"

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --map <path>          Unreal map path (default: ${MAP})
  --duration <seconds>  Capture duration in seconds (default: ${DURATION})
  --warmup <seconds>    Warmup duration in seconds (default: ${WARMUP})
  --output <path>       Output JSON path, relative to project root or absolute (default: ${OUTPUT})
  --nullrhi             Use NullRHI (faster, non-rendering benchmark)
  --help                Show this help

Environment overrides:
  UE_EDITOR_CMD, MAP, DURATION, WARMUP, OUTPUT, USE_NULLRHI
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --map)
      MAP="$2"
      shift 2
      ;;
    --duration)
      DURATION="$2"
      shift 2
      ;;
    --warmup)
      WARMUP="$2"
      shift 2
      ;;
    --output)
      OUTPUT="$2"
      shift 2
      ;;
    --nullrhi)
      USE_NULLRHI=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ ! -f "${PROJECT_FILE}" ]]; then
  echo "Project file not found: ${PROJECT_FILE}" >&2
  exit 1
fi

if [[ ! -x "${UE_EDITOR_CMD}" ]]; then
  echo "UnrealEditor-Cmd not executable: ${UE_EDITOR_CMD}" >&2
  exit 1
fi

EXTRA_ARGS=()
if [[ "${USE_NULLRHI}" != "1" && -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
  USE_NULLRHI=1
  echo "No display detected; enabling --nullrhi automatically."
fi

if [[ "${USE_NULLRHI}" == "1" ]]; then
  EXTRA_ARGS+=("-NullRHI")
fi

echo "Running perf capture:"
echo "  map=${MAP}"
echo "  duration=${DURATION}"
echo "  warmup=${WARMUP}"
echo "  output=${OUTPUT}"
echo "  nullrhi=${USE_NULLRHI}"

"${UE_EDITOR_CMD}" "${PROJECT_FILE}" "${MAP}" \
  -game -unattended -nosplash -nosound \
  "${EXTRA_ARGS[@]}" \
  -ExecCmds="Canal.RunPerfCapture Duration=${DURATION} Warmup=${WARMUP} Output=${OUTPUT} ExitOnComplete=1" \
  -log

if [[ "${OUTPUT}" = /* ]]; then
  JSON_PATH="${OUTPUT}"
else
  JSON_PATH="${PROJECT_ROOT}/${OUTPUT}"
fi
CSV_PATH="${JSON_PATH%.json}.csv"

echo "Capture complete."
echo "  json=${JSON_PATH}"
echo "  csv=${CSV_PATH}"

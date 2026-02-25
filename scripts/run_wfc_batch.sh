#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_FILE="${PROJECT_ROOT}/UEGame.uproject"

UE_ROOT="${UE_ROOT:-/home/olivier/Projects/UnrealEngine}"
EDITOR_CMD_BIN="${EDITOR_CMD_BIN:-${UE_ROOT}/Engine/Binaries/Linux/UnrealEditor-Cmd}"
OUTPUT_DIR="${OUTPUT_DIR:-/tmp/wfc_reports}"
OUTPUT_PREFIX="${OUTPUT_PREFIX:-wfc_batch}"
CV_PORT="${CV_PORT:-19000}"
STRICT_EXIT=0

usage() {
  cat <<'EOF'
Usage:
  ./scripts/run_wfc_batch.sh [options] [-- <extra commandlet args>]

Runs the CanalWfcBatch commandlet and treats generated report files as success,
even if Unreal exits non-zero due unrelated asset-registry errors.

Options:
  --output-dir <dir>       Report output directory (default: /tmp/wfc_reports)
  --output-prefix <name>   Report filename prefix (default: wfc_batch)
  --cv-port <port>         UnrealCV port override to avoid collisions (default: 19000)
  --editor-cmd-bin <path>  Override UnrealEditor-Cmd binary path
  --project <path>         Override .uproject path
  --strict-exit            Fail on non-zero Unreal exit even if reports exist
  -h, --help               Show this help

Examples:
  ./scripts/run_wfc_batch.sh -- --GridWidth=16 --GridHeight=12 --NumSeeds=1000
  OUTPUT_PREFIX=m1_relaxed ./scripts/run_wfc_batch.sh -- --RequireEntryExitPath=false --RequireSingleWaterComponent=false
EOF
}

EXTRA_ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --output-dir)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    --output-prefix)
      OUTPUT_PREFIX="$2"
      shift 2
      ;;
    --cv-port)
      CV_PORT="$2"
      shift 2
      ;;
    --editor-cmd-bin)
      EDITOR_CMD_BIN="$2"
      shift 2
      ;;
    --project)
      PROJECT_FILE="$2"
      shift 2
      ;;
    --strict-exit)
      STRICT_EXIT=1
      shift
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

if [[ ! -x "${EDITOR_CMD_BIN}" ]]; then
  echo "UnrealEditor-Cmd binary not found or not executable: ${EDITOR_CMD_BIN}" >&2
  exit 1
fi

if [[ ! -f "${PROJECT_FILE}" ]]; then
  echo "Project file not found: ${PROJECT_FILE}" >&2
  exit 1
fi

mkdir -p "${OUTPUT_DIR}"

RUN_STAMP="$(date -u +%Y%m%d_%H%M%S)"
LOG_FILE="${OUTPUT_DIR}/${OUTPUT_PREFIX}_${RUN_STAMP}.log"

echo "Running CanalWfcBatch commandlet..."
echo "  project: ${PROJECT_FILE}"
echo "  editor : ${EDITOR_CMD_BIN}"
echo "  output : ${OUTPUT_DIR}"
echo "  prefix : ${OUTPUT_PREFIX}"
echo "  cvport : ${CV_PORT}"
echo "  log    : ${LOG_FILE}"

set +e
"${EDITOR_CMD_BIN}" "${PROJECT_FILE}" \
  -run=CanalWfcBatch \
  "-OutputDir=${OUTPUT_DIR}" \
  "-OutputPrefix=${OUTPUT_PREFIX}" \
  "-cvport=${CV_PORT}" \
  -log \
  "${EXTRA_ARGS[@]}" >"${LOG_FILE}" 2>&1
UE_EXIT=$?
set -e

LATEST_JSON="$(ls -1t "${OUTPUT_DIR}/${OUTPUT_PREFIX}"_*.json 2>/dev/null | head -n 1 || true)"
LATEST_CSV="$(ls -1t "${OUTPUT_DIR}/${OUTPUT_PREFIX}"_*.csv 2>/dev/null | head -n 1 || true)"

if [[ -n "${LATEST_JSON}" && -n "${LATEST_CSV}" ]]; then
  echo "Reports detected:"
  echo "  ${LATEST_JSON}"
  echo "  ${LATEST_CSV}"

  if [[ ${UE_EXIT} -ne 0 ]]; then
    echo "Unreal exited with code ${UE_EXIT}, but reports were generated."
    if [[ ${STRICT_EXIT} -eq 1 ]]; then
      echo "Strict mode enabled; failing." >&2
      exit "${UE_EXIT}"
    fi
  fi

  exit 0
fi

echo "No report files were generated. Check ${LOG_FILE}" >&2
exit "${UE_EXIT:-1}"

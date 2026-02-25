#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_FILE="${PROJECT_ROOT}/UEGame.uproject"

UE_EDITOR_CMD="${UE_EDITOR_CMD:-/home/olivier/Projects/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd}"
OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_ROOT}/Saved/Reports}"
WFC_OUTPUT_DIR="${WFC_OUTPUT_DIR:-/tmp/wfc_reports}"
CV_PORT="${CV_PORT:-19000}"

M1_MIN_SOLVE_RATE="${M1_MIN_SOLVE_RATE:-0.90}"
M1_MIN_FPS="${M1_MIN_FPS:-30.0}"
M1_PERF_MAP="${M1_PERF_MAP:-/Game/StartMap}"
M1_PERF_DURATION="${M1_PERF_DURATION:-5}"
M1_PERF_WARMUP="${M1_PERF_WARMUP:-1}"
M1_AUTOMATION_FILTER="${M1_AUTOMATION_FILTER:-UEGame.Canal.M1.SplineAndSockets}"

RUN_STAMP="$(date -u +%Y%m%d_%H%M%S)"
PERF_OUTPUT="${M1_PERF_OUTPUT:-Saved/Reports/m1_checkpoint_perf_${RUN_STAMP}.json}"
AUTOMATION_LOG="${OUTPUT_DIR}/m1_checkpoint_automation_${RUN_STAMP}.log"
SUMMARY_JSON="${OUTPUT_DIR}/m1_checkpoint_summary_${RUN_STAMP}.json"

json_number() {
  local key="$1"
  local file="$2"
  if command -v jq >/dev/null 2>&1; then
    jq -r ".${key}" "${file}"
  else
    grep -E "\"${key}\"" "${file}" | head -n1 | sed -E "s/.*\"${key}\":[[:space:]]*([^,]+).*/\\1/" | tr -d ' '
  fi
}

to_bool() {
  local value="$1"
  local threshold="$2"
  awk -v v="${value}" -v t="${threshold}" 'BEGIN { if ((v + 0.0) >= (t + 0.0)) print "true"; else print "false"; }'
}

compute_rate() {
  local solved="$1"
  local processed="$2"
  awk -v s="${solved}" -v p="${processed}" 'BEGIN { if ((p + 0) <= 0) printf "0.000000"; else printf "%.6f", (s + 0.0) / (p + 0.0); }'
}

run_wfc_case() {
  local case_name="$1"
  local grid_w="$2"
  local grid_h="$3"
  local seeds="$4"
  local start_seed="$5"

  ./scripts/run_wfc_batch.sh \
    --output-dir "${WFC_OUTPUT_DIR}" \
    --output-prefix "${case_name}" \
    --cv-port "${CV_PORT}" \
    -- \
    -GridWidth="${grid_w}" \
    -GridHeight="${grid_h}" \
    -NumSeeds="${seeds}" \
    -StartSeed="${start_seed}" \
    -MaxAttempts=8 \
    -MaxPropagationSteps=100000 \
    -RequireEntryExitPath=false \
    -RequireSingleWaterComponent=false \
    -AutoSelectBoundaryPorts=true \
    -DisallowUnassignedBoundaryWater=false \
    >&2

  local latest_json
  latest_json="$(ls -1t "${WFC_OUTPUT_DIR}/${case_name}"_*.json | head -n1)"

  local solved processed rate
  solved="$(json_number "num_solved" "${latest_json}")"
  processed="$(json_number "num_seeds_processed" "${latest_json}")"
  rate="$(compute_rate "${solved}" "${processed}")"

  echo "${latest_json}|${solved}|${processed}|${rate}"
}

mkdir -p "${OUTPUT_DIR}" "${WFC_OUTPUT_DIR}"

echo "Running M1 checkpoint validation..."
echo "  project: ${PROJECT_FILE}"
echo "  output : ${OUTPUT_DIR}"
echo "  wfc    : ${WFC_OUTPUT_DIR}"
echo "  min solve rate: ${M1_MIN_SOLVE_RATE}"
echo "  min fps: ${M1_MIN_FPS}"

WFC_128_PREFIX="m1_relaxed_128_${RUN_STAMP}"
WFC_512_PREFIX="m1_relaxed_512_${RUN_STAMP}"

WFC_128_RESULT="$(run_wfc_case "${WFC_128_PREFIX}" 16 8 20 2000)"
WFC_512_RESULT="$(run_wfc_case "${WFC_512_PREFIX}" 32 16 10 5000)"

WFC_128_JSON="$(echo "${WFC_128_RESULT}" | cut -d'|' -f1)"
WFC_128_SOLVED="$(echo "${WFC_128_RESULT}" | cut -d'|' -f2)"
WFC_128_PROCESSED="$(echo "${WFC_128_RESULT}" | cut -d'|' -f3)"
WFC_128_RATE="$(echo "${WFC_128_RESULT}" | cut -d'|' -f4)"

WFC_512_JSON="$(echo "${WFC_512_RESULT}" | cut -d'|' -f1)"
WFC_512_SOLVED="$(echo "${WFC_512_RESULT}" | cut -d'|' -f2)"
WFC_512_PROCESSED="$(echo "${WFC_512_RESULT}" | cut -d'|' -f3)"
WFC_512_RATE="$(echo "${WFC_512_RESULT}" | cut -d'|' -f4)"

WFC_128_PASS="$(to_bool "${WFC_128_RATE}" "${M1_MIN_SOLVE_RATE}")"
WFC_512_PASS="$(to_bool "${WFC_512_RATE}" "${M1_MIN_SOLVE_RATE}")"

./scripts/run_perf_capture.sh \
  --map "${M1_PERF_MAP}" \
  --duration "${M1_PERF_DURATION}" \
  --warmup "${M1_PERF_WARMUP}" \
  --output "${PERF_OUTPUT}"

if [[ "${PERF_OUTPUT}" = /* ]]; then
  PERF_JSON="${PERF_OUTPUT}"
else
  PERF_JSON="${PROJECT_ROOT}/${PERF_OUTPUT}"
fi

PERF_AVG_FPS="$(json_number "avg_fps" "${PERF_JSON}")"
PERF_PASS="$(to_bool "${PERF_AVG_FPS}" "${M1_MIN_FPS}")"

set +e
"${UE_EDITOR_CMD}" "${PROJECT_FILE}" \
  -unattended -nullrhi -nosound -NoSplash \
  -ExecCmds="Automation RunTests ${M1_AUTOMATION_FILTER};Quit" \
  -TestExit="Automation Test Queue Empty" \
  -log >"${AUTOMATION_LOG}" 2>&1
AUTOMATION_EXIT=$?
set -e

AUTOMATION_PASS="false"
if [[ ${AUTOMATION_EXIT} -eq 0 ]]; then
  AUTOMATION_PASS="true"
fi

cat > "${SUMMARY_JSON}" <<EOF
{
  "timestamp_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "profile": "m1_relaxed",
  "min_solve_rate": ${M1_MIN_SOLVE_RATE},
  "min_fps": ${M1_MIN_FPS},
  "wfc_128": {
    "json_report": "${WFC_128_JSON}",
    "num_solved": ${WFC_128_SOLVED},
    "num_seeds_processed": ${WFC_128_PROCESSED},
    "solve_rate": ${WFC_128_RATE},
    "pass": ${WFC_128_PASS}
  },
  "wfc_512": {
    "json_report": "${WFC_512_JSON}",
    "num_solved": ${WFC_512_SOLVED},
    "num_seeds_processed": ${WFC_512_PROCESSED},
    "solve_rate": ${WFC_512_RATE},
    "pass": ${WFC_512_PASS}
  },
  "perf": {
    "json_report": "${PERF_JSON}",
    "avg_fps": ${PERF_AVG_FPS},
    "pass": ${PERF_PASS}
  },
  "automation": {
    "filter": "${M1_AUTOMATION_FILTER}",
    "log": "${AUTOMATION_LOG}",
    "exit_code": ${AUTOMATION_EXIT},
    "pass": ${AUTOMATION_PASS}
  }
}
EOF

echo "M1 checkpoint summary: ${SUMMARY_JSON}"
cat "${SUMMARY_JSON}"

if [[ "${WFC_128_PASS}" != "true" || "${WFC_512_PASS}" != "true" || "${PERF_PASS}" != "true" || "${AUTOMATION_PASS}" != "true" ]]; then
  echo "M1 checkpoint validation failed." >&2
  exit 1
fi

echo "M1 checkpoint validation passed."

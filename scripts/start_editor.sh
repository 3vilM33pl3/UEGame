#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_FILE="${PROJECT_ROOT}/UEGame.uproject"

UE_ROOT="${UE_ROOT:-/home/olivier/Projects/UnrealEngine}"
EDITOR_BIN="${EDITOR_BIN:-${UE_ROOT}/Engine/Binaries/Linux/UnrealEditor}"
UE_DISPLAY_BACKEND="${UE_DISPLAY_BACKEND:-x11}"

if [[ ! -f "${PROJECT_FILE}" ]]; then
  echo "Project not found: ${PROJECT_FILE}" >&2
  exit 1
fi

if [[ ! -x "${EDITOR_BIN}" ]]; then
  echo "UnrealEditor binary not found or not executable: ${EDITOR_BIN}" >&2
  echo "Set UE_ROOT or EDITOR_BIN to your Unreal Engine path." >&2
  exit 1
fi

case "${UE_DISPLAY_BACKEND}" in
  x11)
    # Wayland popup/menu placement can be wrong in Unreal; X11 is more stable.
    export SDL_VIDEODRIVER=x11
    ;;
  wayland)
    export SDL_VIDEODRIVER=wayland
    ;;
  auto)
    ;;
  *)
    echo "Invalid UE_DISPLAY_BACKEND='${UE_DISPLAY_BACKEND}'. Use: x11, wayland, or auto." >&2
    exit 1
    ;;
esac

exec "${EDITOR_BIN}" "${PROJECT_FILE}" "$@"

#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

print_usage() {
	echo "Usage: $0 [local|cloud] [extra server args...]"
	echo ""
	echo "Examples:"
	echo "  $0 local"
	echo "  $0 cloud --duration-seconds 120"
	echo ""
	echo "Environment overrides:"
	echo "  BUILD_DIR (default: <repo>/build-server)"
	echo "  CMAKE_BUILD_TYPE (default: Release)"
	echo "  CMAKE_GENERATOR (default: Ninja)"
	echo "  SERVER_EXTRA_ARGS (default: empty)"
}

PROFILE="local"
if [[ $# -gt 0 ]]; then
	if [[ "$1" == "-h" || "$1" == "--help" ]]; then
		print_usage
		exit 0
	fi

	if [[ "$1" == "local" || "$1" == "cloud" ]]; then
		PROFILE="$1"
		shift
	fi
fi

if [[ "${PROFILE}" != "local" && "${PROFILE}" != "cloud" ]]; then
	print_usage
	exit 1
fi

BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build-server}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"
C_COMPILER="${C_COMPILER:-$(command -v gcc || command -v clang || true)}"
CXX_COMPILER="${CXX_COMPILER:-$(command -v g++ || command -v clang++ || true)}"
ASM_COMPILER="${ASM_COMPILER:-${C_COMPILER}}"

echo "[launcher] profile=${PROFILE}"
echo "[launcher] configuring ${BUILD_DIR}"
if [[ -z "${C_COMPILER}" || -z "${CXX_COMPILER}" ]]; then
	echo "[launcher] error: no C/C++ compiler found in PATH"
	echo "[launcher] set C_COMPILER and CXX_COMPILER env vars explicitly"
	exit 1
fi

cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" -G "${CMAKE_GENERATOR}" \
	-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
	-DCMAKE_C_COMPILER="${C_COMPILER}" \
	-DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
	-DCMAKE_ASM_COMPILER="${ASM_COMPILER}"

echo "[launcher] building pvz-authoritative-server"
cmake --build "${BUILD_DIR}" --target pvz-authoritative-server

SERVER_BIN="${BUILD_DIR}/pvz-authoritative-server"
if [[ ! -x "${SERVER_BIN}" ]]; then
	echo "[launcher] error: server binary not found at ${SERVER_BIN}"
	exit 1
fi

declare -a PROFILE_ARGS
if [[ "${PROFILE}" == "local" ]]; then
	PROFILE_ARGS=(--profile local)
else
	PROFILE_ARGS=(--profile cloud)
fi

declare -a EXTRA_ARGS=()
if [[ -n "${SERVER_EXTRA_ARGS:-}" ]]; then
	# shellcheck disable=SC2206
	EXTRA_ARGS=(${SERVER_EXTRA_ARGS})
fi

echo "[launcher] starting ${SERVER_BIN} ${PROFILE_ARGS[*]} $* ${EXTRA_ARGS[*]}"
exec "${SERVER_BIN}" "${PROFILE_ARGS[@]}" "$@" "${EXTRA_ARGS[@]}"

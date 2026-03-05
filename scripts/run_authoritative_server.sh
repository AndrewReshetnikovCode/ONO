#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

print_usage() {
	echo "Usage: $0 [local|cloud] [--build-only] [extra server args...]"
	echo ""
	echo "Examples:"
	echo "  $0 local"
	echo "  $0 local --build-only"
	echo "  $0 cloud --duration-seconds 120"
	echo ""
	echo "Environment overrides:"
	echo "  BUILD_DIR (default: <repo>/build-server)"
	echo "  CMAKE_BUILD_TYPE (default: Release)"
	echo "  CMAKE_GENERATOR (default: Ninja)"
	echo "  BUILD_PVZ_PORTABLE (default: OFF for server-only build)"
	echo "  C_COMPILER / CXX_COMPILER / ASM_COMPILER (optional explicit compiler paths)"
	echo "  TOOLCHAIN_FILE (optional CMake toolchain path, e.g. MinGW cross build)"
	echo "  SERVER_EXTRA_ARGS (default: empty)"
	echo "  LOG_FILE (default: <build-dir>/run_authoritative_server.log)"
	echo "  SERVER_LOG_FILE (default: <build-dir>/server_log.txt)"
	echo "  QUIET_BUILD (default: 1, hide CMake output in console)"
	echo "  PAUSE_ON_ERROR (default: 1 on Windows-like shells, else 0)"
}

PROFILE="local"
BUILD_ONLY=0
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

declare -a SERVER_ARGS=()
while [[ $# -gt 0 ]]; do
	if [[ "$1" == "--build-only" ]]; then
		BUILD_ONLY=1
	else
		SERVER_ARGS+=("$1")
	fi
	shift
done

if [[ "${PROFILE}" != "local" && "${PROFILE}" != "cloud" ]]; then
	print_usage
	exit 1
fi

BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build-server}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"
BUILD_PVZ_PORTABLE="${BUILD_PVZ_PORTABLE:-OFF}"
C_COMPILER="${C_COMPILER:-}"
CXX_COMPILER="${CXX_COMPILER:-}"
ASM_COMPILER="${ASM_COMPILER:-}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-}"

UNAME_S="$(uname -s | tr '[:upper:]' '[:lower:]')"
IS_WINDOWS_ENV=0
if [[ "${UNAME_S}" == *"mingw"* || "${UNAME_S}" == *"msys"* || "${UNAME_S}" == *"cygwin"* ]]; then
	IS_WINDOWS_ENV=1
fi

mkdir -p "${BUILD_DIR}"
LOG_FILE="${LOG_FILE:-${BUILD_DIR}/run_authoritative_server.log}"
SERVER_LOG_FILE="${SERVER_LOG_FILE:-${BUILD_DIR}/server_log.txt}"
QUIET_BUILD="${QUIET_BUILD:-1}"
: > "${LOG_FILE}"

log() {
	echo "$*" | tee -a "${LOG_FILE}"
}

run_build_step() {
	if [[ "${QUIET_BUILD}" == "1" ]]; then
		"$@" >> "${LOG_FILE}" 2>&1
	else
		"$@" 2>&1 | tee -a "${LOG_FILE}"
	fi
}

if [[ -z "${PAUSE_ON_ERROR:-}" ]]; then
	if [[ ${IS_WINDOWS_ENV} -eq 1 ]]; then
		PAUSE_ON_ERROR=1
	else
		PAUSE_ON_ERROR=0
	fi
fi

on_error() {
	local exit_code=$?
	log "[launcher] failed with exit code ${exit_code}"
	log "[launcher] see build log: ${LOG_FILE}"
	log "[launcher] see server log: ${SERVER_LOG_FILE}"
	if [[ "${PAUSE_ON_ERROR}" == "1" && -t 0 ]]; then
		read -r -p "[launcher] Press Enter to exit..." _
	fi
	exit "${exit_code}"
}
trap on_error ERR

log "[launcher] profile=${PROFILE}"
log "[launcher] configuring ${BUILD_DIR}"

declare -a CMAKE_ARGS
CMAKE_ARGS=(
	-S "${REPO_ROOT}"
	-B "${BUILD_DIR}"
	-G "${CMAKE_GENERATOR}"
	-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
	-DBUILD_PVZ_PORTABLE="${BUILD_PVZ_PORTABLE}"
)
if [[ -n "${C_COMPILER}" ]]; then
	CMAKE_ARGS+=(-DCMAKE_C_COMPILER="${C_COMPILER}")
fi
if [[ -n "${CXX_COMPILER}" ]]; then
	CMAKE_ARGS+=(-DCMAKE_CXX_COMPILER="${CXX_COMPILER}")
fi
if [[ -n "${ASM_COMPILER}" ]]; then
	CMAKE_ARGS+=(-DCMAKE_ASM_COMPILER="${ASM_COMPILER}")
fi
if [[ -n "${TOOLCHAIN_FILE}" ]]; then
	CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}")
fi

run_build_step cmake "${CMAKE_ARGS[@]}"

log "[launcher] building pvz-authoritative-server"
run_build_step cmake --build "${BUILD_DIR}" --target pvz-authoritative-server

SERVER_BIN_NOEXT="${BUILD_DIR}/pvz-authoritative-server"
SERVER_BIN_EXE="${BUILD_DIR}/pvz-authoritative-server.exe"
SERVER_BIN=""

if [[ ${IS_WINDOWS_ENV} -eq 1 ]]; then
	if [[ -f "${SERVER_BIN_EXE}" ]]; then
		SERVER_BIN="${SERVER_BIN_EXE}"
	elif [[ -f "${SERVER_BIN_NOEXT}" ]]; then
	log "[launcher] error: found non-Windows server binary at ${SERVER_BIN_NOEXT}"
	log "[launcher] expected a Windows executable at ${SERVER_BIN_EXE}"
	log "[launcher] clean build directory and rebuild on Windows toolchain"
		exit 1
	fi
else
	if [[ -x "${SERVER_BIN_NOEXT}" ]]; then
		SERVER_BIN="${SERVER_BIN_NOEXT}"
	elif [[ -f "${SERVER_BIN_EXE}" ]]; then
		log "[launcher] error: found Windows-only executable at ${SERVER_BIN_EXE}"
		log "[launcher] expected native executable at ${SERVER_BIN_NOEXT}"
		log "[launcher] clean build directory and rebuild on your native toolchain"
		exit 1
	fi
fi

if [[ -z "${SERVER_BIN}" ]]; then
	log "[launcher] error: server binary not found."
	log "[launcher] looked for:"
	log "[launcher]   - ${SERVER_BIN_NOEXT}"
	log "[launcher]   - ${SERVER_BIN_EXE}"
	exit 1
fi

if [[ ${BUILD_ONLY} -eq 1 ]]; then
	log "[launcher] build-only requested; not starting server."
	log "[launcher] artifact: ${SERVER_BIN}"
	log "[launcher] server log file (when run): ${SERVER_LOG_FILE}"
	exit 0
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

log "[launcher] build log file: ${LOG_FILE}"
log "[launcher] full server log file: ${SERVER_LOG_FILE}"
log "[launcher] starting ${SERVER_BIN} ${PROFILE_ARGS[*]} ${SERVER_ARGS[*]} ${EXTRA_ARGS[*]}"
exec "${SERVER_BIN}" "${PROFILE_ARGS[@]}" "${SERVER_ARGS[@]}" --server-log "${SERVER_LOG_FILE}" "${EXTRA_ARGS[@]}"

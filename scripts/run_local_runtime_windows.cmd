@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM Run local authoritative server + web client host on Windows.
REM Usage:
REM   run_local_runtime_windows.cmd
REM Optional env overrides:
REM   PORT (default 8091)
REM   SERVER_PROFILE (default local)

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_DIR=%%~fI"

if "%PORT%"=="" set "PORT=8091"
if "%SERVER_PROFILE%"=="" set "SERVER_PROFILE=local"

set "SERVER_EXE=%REPO_DIR%\build-server\pvz-authoritative-server.exe"
if not exist "%SERVER_EXE%" (
  set "SERVER_EXE=%REPO_DIR%\build-server\build-server\pvz-authoritative-server.exe"
)

if not exist "%SERVER_EXE%" (
  echo [ERROR] Server executable not found.
  echo         Expected one of:
  echo         - %REPO_DIR%\build-server\pvz-authoritative-server.exe
  echo         - %REPO_DIR%\build-server\build-server\pvz-authoritative-server.exe
  echo         Build it first, then re-run this script.
  pause
  exit /b 1
)

set "CLIENT_DIR="
if exist "%REPO_DIR%\build\index.html" set "CLIENT_DIR=build"
if not defined CLIENT_DIR if exist "%REPO_DIR%\build-yg\index.html" set "CLIENT_DIR=build-yg"

if not defined CLIENT_DIR (
  echo [ERROR] Client web files not found.
  echo         Expected index.html in either:
  echo         - %REPO_DIR%\build
  echo         - %REPO_DIR%\build-yg
  echo         Build or pull artifacts, then re-run this script.
  pause
  exit /b 1
)

set "SERVER_LOG=%REPO_DIR%\build-server\server_log.txt"

echo [INFO] Repo: %REPO_DIR%
echo [INFO] Server: %SERVER_EXE%
echo [INFO] Client dir: %REPO_DIR%\%CLIENT_DIR%
echo [INFO] Port: %PORT%

echo [INFO] Starting authoritative server...
start "PvZ Authoritative Server" cmd /k ^
"cd /d ""%REPO_DIR%"" && ""%SERVER_EXE%"" --profile %SERVER_PROFILE% --players 8 --duration-seconds 0 --log-every-ticks 60 --server-log ""%SERVER_LOG%"""

echo [INFO] Starting Python HTTP server...
where py >nul 2>nul
if errorlevel 1 (
  start "PvZ Web Server %PORT%" cmd /k ^
  "cd /d ""%REPO_DIR%"" && python -m http.server %PORT% --directory ""%REPO_DIR%\%CLIENT_DIR%"""
) else (
  start "PvZ Web Server %PORT%" cmd /k ^
  "cd /d ""%REPO_DIR%"" && py -3 -m http.server %PORT% --directory ""%REPO_DIR%\%CLIENT_DIR%"""
)

echo [INFO] Opening browser...
start "" "http://127.0.0.1:%PORT%/?pvz_auto_runtime=1"

echo [SUCCESS] Runtime launch commands started.
exit /b 0

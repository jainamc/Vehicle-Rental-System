@echo off
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
  echo g++ was not found. Install MinGW/MSYS2 and add g++ to PATH.
  pause
  exit /b 1
)

echo Building Roadwise C++ dashboard...
g++ -std=c++17 -Wall -Wextra -pedantic -I".vscode\vendor" ".vscode\shared_server.cpp" -o ".vscode\shared_server.exe" -lws2_32
if errorlevel 1 (
  echo Build failed. Read the compiler error above.
  pause
  exit /b 1
)

echo Starting dashboard at http://127.0.0.1:8080/
start "Roadwise C++ Dashboard" ".vscode\shared_server.exe"
timeout /t 2 /nobreak >nul
start "" "http://127.0.0.1:8080/"
echo Keep the Roadwise server window open while using the dashboard.
endlocal

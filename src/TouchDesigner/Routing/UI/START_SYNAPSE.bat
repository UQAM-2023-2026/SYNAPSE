@echo off
title SYNAPSE Server
cd /d "%~dp0"

echo ============================================================
echo   SYNAPSE Server
echo ============================================================
echo.
echo   OSC Port:  6970
echo   WebSocket: 8765
echo.
echo   Dashboard: Ouvre dashboard.html dans ton navigateur
echo.
echo   Press Ctrl+C to stop
echo ============================================================
echo.

REM Use Anaconda Python
set PYTHON=C:\ProgramData\anaconda3\python.exe
if not exist "%PYTHON%" set PYTHON=python

"%PYTHON%" server.py

pause

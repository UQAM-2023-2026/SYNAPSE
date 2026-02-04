@echo off
echo Stopping SYNAPSE server...
taskkill /F /FI "WINDOWTITLE eq SYNAPSE*" >nul 2>&1
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :8765 ^| findstr LISTENING') do taskkill /F /PID %%a >nul 2>&1
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :6970') do taskkill /F /PID %%a >nul 2>&1
echo Done.
timeout /t 2 >nul

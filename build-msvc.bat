@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-msvc.ps1" %*
exit /b %ERRORLEVEL%

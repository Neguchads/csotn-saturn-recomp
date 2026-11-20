@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0sotn-experiment\tools\play-windows.ps1" %*
exit /b %errorlevel%

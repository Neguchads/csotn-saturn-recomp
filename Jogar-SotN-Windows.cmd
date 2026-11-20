@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0sotn-experiment\tools\play-windows.ps1" %*
if errorlevel 1 pause

@echo off
setlocal
set "PYTHON=%~dp0.venv\Scripts\python.exe"
if not exist "%PYTHON%" (echo Python local ausente. Consulte docs\SETUP.md. & exit /b 1)
"%PYTHON%" "%~dp0sotn-experiment\tools\make_all_config.py"
if errorlevel 1 exit /b 1
"%PYTHON%" "%~dp0sotn-experiment\tools\generate-jp.py" "%~dp0sotn-experiment\game-jp-all.toml"
if errorlevel 1 exit /b 1
call "%~dp0sotn-experiment\tools\build-windows-reference.cmd" "%~1"
exit /b %errorlevel%

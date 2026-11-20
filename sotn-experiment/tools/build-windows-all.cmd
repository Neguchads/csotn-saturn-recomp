@echo off
setlocal
for %%I in ("%~dp0..") do set "EXP=%%~fI"
set "SOTN_BUILD_VARIANT=windows-reference-all"
set "SOTN_RECOMP_DIR=%EXP%\build\jp-all\recomp"
call "%~dp0build-windows-common.cmd" ON "%~1"
exit /b %errorlevel%

@echo off
call "%~dp0build-windows-all.cmd" "%~1"
exit /b %errorlevel%

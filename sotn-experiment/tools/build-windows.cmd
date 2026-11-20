@echo off
call "%~dp0build-windows-common.cmd" OFF "%~1"
exit /b %errorlevel%

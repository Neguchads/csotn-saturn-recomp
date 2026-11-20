@echo off
setlocal
if "%~1"=="ON" (set "VARIANT=windows-reference-current") else (set "VARIANT=windows-native-current")
if defined SOTN_BUILD_VARIANT set "VARIANT=%SOTN_BUILD_VARIANT%"
for %%I in ("%~dp0..") do set "EXP=%%~fI"
if not defined VCVARS64 (
  for /f "usebackq tokens=*" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS64=%%I\VC\Auxiliary\Build\vcvars64.bat"
)
if not exist "%VCVARS64%" (echo Configure VCVARS64 com o vcvars64.bat do Visual Studio. & exit /b 1)
call "%VCVARS64%" >nul
if errorlevel 1 exit /b 1
if not defined SATURN_RUNTIME set "SATURN_RUNTIME=%EXP%\..\engine\runtime"
if not defined SDL3_ROOT set "SDL3_ROOT=%EXP%\build\windows-deps\SDL3-3.4.14"
if not defined SOTN_RECOMP_DIR set "SOTN_RECOMP_DIR=%EXP%\build\jp\recomp"
set "PATH=%EXP%\build\windows-deps\llvm\bin;%EXP%\build\windows-tools\Scripts;%PATH%"
if not exist "%SATURN_RUNTIME%" (echo Runtime ausente. Consulte docs/SETUP.md. & exit /b 1)
set "SATURN_RUNTIME=%SATURN_RUNTIME:\=/%"
cmake -S "%EXP%\tools\windows" -B "%EXP%\build\jp\%VARIANT%" -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded "-DSATURN_RUNTIME=%SATURN_RUNTIME%" "-DCMAKE_PREFIX_PATH=%SDL3_ROOT%" "-DSOTN_RECOMP_DIR=%SOTN_RECOMP_DIR%" -DSATURN_REQUIRE_SDL=ON -DSATURN_VDP1_GPL=%~1
if errorlevel 1 exit /b 1
if "%~2"=="--clean" (
  cmake --build "%EXP%\build\jp\%VARIANT%" --target clean
  if errorlevel 1 exit /b 1
)
cmake --build "%EXP%\build\jp\%VARIANT%" --target saturn -j 4
if errorlevel 1 exit /b 1
copy /Y "%SDL3_ROOT%\lib\x64\SDL3.dll" "%EXP%\build\jp\%VARIANT%\game\SDL3.dll" >nul
if errorlevel 1 exit /b 1

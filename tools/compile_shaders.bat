@echo off
setlocal enabledelayedexpansion

echo Starting shaders compiler..

REM change to shaders directory
cd shaders

REM Loop through all .{frag,vert,glsl} files
for %%f in (*.frag *.vert *.glsl) do (
  REM Get filename without ext
  set filename=%%~nf
  set extension=%%~xf

  set "message=Compiling: %%f to !filename!!extension!.spv"
  call echo !message!

  REM Compile shader
  ..\tools\glslc.exe "%%f" -o "!filename!!extension!.spv"

)

echo Shaders compiler finished.
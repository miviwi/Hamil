@echo off
set "cwd=%cd%"

cd %~dp0win32

python setup.py build
for /f "delims=" %%f in ('dir /s /b build\*.pyd') do (
  copy %%f ..\src
)

cd %cwd%

echo.
echo         ...Done!

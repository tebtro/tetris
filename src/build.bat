@echo off

IF NOT EXIST ..\build  mkdir ..\build
pushd ..\build

:: -O2 optimization level 2
:: -Od for debbugging, no optimization
set CommonCompilerFlags=/nologo /Fe:tetris -FC -Zi /EHsc -Od -diagnostics:column -diagnostics:caret

:: Linker Options
set AdditionalLinkerFlags=-incremental:no -opt:ref

cl %CommonCompilerFlags% %AdditionalIncludeDirectories% ../src/win32_tetris.cpp /link %AdditionalLinkerFlags% user32.lib gdi32.lib winmm.lib

popd

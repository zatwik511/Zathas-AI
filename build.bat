@echo off
set PATH=C:\msys64\ucrt64\bin;%PATH%
cmake -B build -DZLIB_ROOT=C:\msys64\ucrt64
cmake --build build --parallel

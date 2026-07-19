@echo off
echo Compiling raylib program...
gcc -o game.exe main.c -L. -lraylib -lgdi32 -lwinmm
if %errorlevel% equ 0 (
    echo Compilation successful!
    echo Running program...
    game.exe
) else (
    echo Compilation failed!
    pause
)
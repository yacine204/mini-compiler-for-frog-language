# FROG Mini Compiler

A mini compiler for the FROG programming language (`.frg`), written in C with a GTK3 graphical interface. It performs lexical, syntax, and semantic analysis on FROG source files through an interactive GUI.

For full language rules, overview, and code examples, see `plan.txt`.

---

## How to Run

### Linux

**Requirements:** `gcc`, `gtk+-3.0`, `pkg-config`

```bash
sudo apt install gcc libgtk-3-dev pkg-config
```

```bash
gcc -o frog_compiler main.c src/*.c $(pkg-config --cflags --libs gtk+-3.0)
./frog_compiler
```

### Windows

**Requirements:** [MSYS2](https://www.msys2.org/)

Install dependencies via MSYS2 terminal:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gtk3 mingw-w64-x86_64-pkg-config
```

Compile:

```bash
gcc -o frog_compiler.exe main.c src/*.c $(pkg-config --cflags --libs gtk+-3.0)
./frog_compiler.exe
```

You need Raylib 5.x built and installed with `make install`.

Running `run.sh` will build and launch the game in Linux. You need GCC 13+ for this.

Running `valgrind.sh` will run Valgrind on the program with a preset of options.

Using camelCase and PascalCase so the code has the same style as Raylib.

The Pixantiqua font is designed by designed by Gerhard Grossmann and is a free to use font distributed alongside Raylib.

Compilation for Windows is done with `make win`, it expects the raylib repository in `../raylib` with the compiled static for Windows in `../raylib/src`, that you can compile with mingw32 (TODO: include in repo static libraries for compilation in case the libraries are altered).

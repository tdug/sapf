# sapf, cross-platform edition

this is a highly work-in-progress fork of James McCartney's [sapf](https://github.com/lfnoise/sapf) (Sound As Pure Form) which aims to implement cross-platform alternatives for the various macOS libraries used in the original codebase. for the time being, the top priority platform is Linux.

[original README](README.txt)

## building

a Nix flake is included. simply run:

```shell
nix develop
meson setup build
meson compile -C build
```

and you should get a binary at `./build/sapf`.

if not using Nix, you will need to install dependencies manually and probably specify Clang as the compiler:

```shell
CC=clang CXX=clang++ meson setup build
CC=clang CXX=clang++ meson compile -C build
```

# cube

spinning cube (and the other platonic solids)

### usage
```sh
usage: ./cube [--shape]

shapes:
  --cube (default)
  --tetrahedron
  --octahedron
  --dodecahedron
  --icosahedron
```

![demo](docs/demo.gif)

q or esc quits.

### if you're on gentoo:
```sh
emerge --ask app-eselect/eselect-repository
eselect repository add noahburchell git https://github.com/noahburchell/noahburchell-overlay.git
emaint sync --repo noahburchell
# may be masked
emerge --ask games-misc/cube-cli

cube
```
distro support status:
  - gentoo ✅
  - everything else ❌

### if you're on a different distro:

you have to build it

dependencies:
  - linux
  - make
  - gcc 14+ (or clang 19+)

building:
  - run 'make' then 'make install'

licence: GPLv3

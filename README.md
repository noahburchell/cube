# cube

spinning cube (and the other platonic solids)

### usage
```sh
usage: cube [--shape]

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
emerge --ask app-misc/cube
cube --help
```
### if you're on nix:

run it without installing anything:
```sh
nix run github:noahburchell/cube
nix run github:noahburchell/cube -- --icosahedron
```

install it into your profile:
```sh
nix profile install github:noahburchell/cube
```

or add it to a flake:
```nix
{
  inputs.cube.url = "github:noahburchell/cube";

  # then, in your config:
  #   environment.systemPackages = [ inputs.cube.packages.${pkgs.system}.default ];
  # or, with the overlay:
  #   nixpkgs.overlays = [ inputs.cube.overlays.default ];
  #   environment.systemPackages = [ pkgs.cube ];
}
```

hacking on it (drops you in a shell with gcc, make, bear, clangd and gdb):
```sh
nix develop
make
```

distro support status:
  - gentoo ✅
  - nix ✅
  - arch ❔ (i made the PKGBUILD, but AUR account registrations are closed)
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

# cube 
### spinning cube (and the other platonic solids)
### get it: [gentoo](#if-youre-on-gentoo) | [nix](#if-you-have-nix) | [brew](#if-you-have-brew) | [source](#if-youre-on-something-else-apart-from-windows)

demo: https://nburch.org

### usage:
```sh
usage: cube [option]

options:
  -h, --help          show this help

shapes:
  -c, --cube          (default)
  -t, --tetrahedron
  -o, --octahedron
  -d, --dodecahedron
  -i, --icosahedron

q or esc quits
```

### if you're on gentoo:
```sh
emerge --ask app-eselect/eselect-repository
eselect repository add nburch git https://github.com/noahburchell/nburch-overlay.git
emaint sync --repo nburch
emerge --ask app-misc/cube
cube --help
```

### if you have brew:
```sh
# you need xcode 16.3+ (apple clang 17)
brew install noahburchell/cube/cube
```

### if you have nix:

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

hacking it (drops you in a shell with a compiler, make, bear, clangd and a debugger):
```sh
nix develop
make
```

os support status:
  - linux ✅
  - macos ✅
  - windows ❌

distro packaging status:
  - gentoo ✅
  - brew ✅
  - nix ✅
  - arch ❔ (i made the PKGBUILD, but AUR account registrations are closed)
  - everything else ❌

### if you're on something else (apart from windows):
you have to build it

dependencies:
  - linux, macos, or a bsd
  - make (`gmake` on the bsds)
  - gcc 14+ or clang 19+

#### macos:

you need xcode 16.3+ (apple clang 17). older xcode accepts `-std=gnu23`
but has no c23 `constexpr`, so the build will fail. if that happens:

```sh
brew install llvm
make CC="$(brew --prefix llvm)/bin/clang"
```

homebrew `gcc` works too.

#### bsd:

run `gmake`, not `make`.

freebsd 14.2+ has a new enough clang in base. on openbsd and netbsd, install one
from packages/pkgsrc and the makefile will find it on its own: it looks for
`clang19`, `clang20`, `gcc14`, `gcc15` and `egcc` on path.


#### get the source
grab the release tarball:

```sh
curl -LO https://github.com/noahburchell/cube/archive/refs/tags/v1.2.tar.gz
tar xf v1.2.tar.gz
cd cube-1.2
```

or clone the repo:

```sh
git clone --depth 1  https://github.com/noahburchell/cube.git
cd cube
```

#### build it:

```sh
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
sudo make install
cube --help
```

`make install` puts the binary in `/usr/bin` by default. so if you don't have root, or you
just don't want it there:

```sh
make install PREFIX="$HOME/.local" # make sure its in path
```

### configuration

are you unhappy with the characters i chose for shading?
  - change them in shapes.c, it should be clear how to do so

do you want the shapes to spin faster/slower?
  - spin period was chosen for a reason. changing it may introduce stutter

### contact

if you have any questions contact me: cube@nburch.org

### license

GNU General Public Licence v3.0

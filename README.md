# cube 
### spinning cube (and the other platonic solids)
### get it: [gentoo](#if-youre-on-gentoo) | [arch](#if-youre-on-arch) | [nix](#if-you-have-nix) | [brew](#if-you-have-brew) | [source](#if-youre-on-something-else-apart-from-windows)

latest release: **v1.2.2** — https://github.com/noahburchell/cube/releases/latest

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

### if you're on arch:

```sh
curl -LO https://github.com/noahburchell/cube/releases/download/v1.2.2/PKGBUILD
# 'less PKGBUILD' if you want to read it
makepkg -si
```

needs `base-devel`

### if you have brew:
```sh
# on xcode older than 16.3 (apple clang 17) the formula pulls in homebrew llvm to build
brew install noahburchell/cube/cube
```

### if you have nix:

run it without installing anything:
```sh
nix run github:noahburchell/cube/v1.2.2
nix run github:noahburchell/cube/v1.2.2 -- --icosahedron
```

install it into your profile:
```sh
nix profile add github:noahburchell/cube/v1.2.2
```

on nix older than 2.30 the command is `nix profile install`

or add it to a flake:
```nix
{
  inputs.cube.url = "github:noahburchell/cube/v1.2.2";

  # then, in your config:
  #   environment.systemPackages = [ inputs.cube.packages.${pkgs.stdenv.hostPlatform.system}.default ];
  # or, with the overlay:
  #   nixpkgs.overlays = [ inputs.cube.overlays.default ];
  #   environment.systemPackages = [ pkgs.cube ];
}
```

hacking it?:
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
  - arch ✅ (not on the AUR because registrations are closed)
  - everything else ❌

### if you're on something else (apart from windows):
you have to build it

dependencies:
  - linux, macos, or a bsd
  - make (`gmake` on the bsds)
  - gcc 14+ or clang 19+

#### debian and ubuntu:

debian 13+ and ubuntu 26.04+ ship a new enough gcc by default:

```sh
sudo apt install build-essential
```

ubuntu 24.04 ships gcc 13 by default, install gcc 14

```sh
sudo apt install make gcc-14 libc6-dev
```

debian 12 has no gcc 14 package, so use clang 19:

```sh
sudo apt install make clang-19
```

the makefile finds `gcc-14` and `clang-19` on its own, `CC` does not need to be set.
ubuntu 22.04 doesnt have a new enough compiler in its archive

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

freebsd 14.3+ has a new enough clang in base (14.2 ships clang 18). on openbsd and netbsd,
install one from packages/pkgsrc and the makefile will find it on its own: it tries `gcc`,
`clang` and `cc`, then `clang-19` to `clang-21`, `gcc-14`, `gcc-15`, `clang19`, `clang20`,
`gcc14`, `gcc15` and `egcc` on path.


#### get the source

get the release tarball

```sh
curl -LO https://github.com/noahburchell/cube/releases/download/v1.2.2/cube-1.2.2.tar.gz
tar xf cube-1.2.2.tar.gz
cd cube-1.2.2
```

verify it first if you want, the checksum is also printed in the release notes:

```sh
curl -LO https://github.com/noahburchell/cube/releases/download/v1.2.2/cube-1.2.2.tar.gz.sha256
sha256sum -c cube-1.2.2.tar.gz.sha256
```

or clone the repo:

```sh
git clone --depth 1 https://github.com/noahburchell/cube.git
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

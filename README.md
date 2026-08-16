# cube-cli

spinning cube

if you're on gentoo:
```sh
emerge --ask app-eselect/eselect-repository
eselect repository add noahburchell git https://github.com/noahburchell/noahburchell-overlay.git
emaint sync --repo noahburchell
# may be masked
emerge --ask games-misc/cube-cli
```

dependencies:
  - linux
  - make
  - gcc 13+

building:
  - run 'make'

licence: GPLv3

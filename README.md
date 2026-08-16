# cube-cli

spinning cube
<img width="1004" height="1142" alt="cube" src="https://github.com/user-attachments/assets/7e26a7ac-0c52-4cce-9a07-c9a3c8c838e8" />

### if you're on gentoo:
```sh
emerge --ask app-eselect/eselect-repository
eselect repository add noahburchell git https://github.com/noahburchell/noahburchell-overlay.git
emaint sync --repo noahburchell
# may be masked
emerge --ask games-misc/cube-cli
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

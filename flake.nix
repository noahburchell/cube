{
  description = "spinning cube (and the other platonic solids) for your terminal";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      # termios, poll and TIOCGWINSZ, per the README's "dependencies: linux".
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = fn: nixpkgs.lib.genAttrs systems (system: fn nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (
        pkgs:
        let
          inherit (pkgs) lib;
        in
        rec {
          default = cube;

          cube = pkgs.stdenv.mkDerivation {
            pname = "cube";
            version = "1.0.0";

            src = lib.fileset.toSource {
              root = ./.;
              fileset = lib.fileset.unions [
                ./Makefile
                ./src
                ./LICENSE
              ];
            };

            strictDeps = true;
            enableParallelBuilding = true;

            # install -Dm755 $(DESTDIR)$(BINDIR)/cube, with BINDIR = PREFIX/bin.
            makeFlags = [ "PREFIX=$(out)" ];

            meta = {
              description = "spinning cube (and the other platonic solids) for your terminal";
              homepage = "https://github.com/noahburchell/cube";
              # No per-file headers granting "or later", so: version 3 exactly.
              license = lib.licenses.gpl3Only;
              mainProgram = "cube";
              platforms = systems;
            };
          };
        }
      );

      apps = forAllSystems (pkgs: rec {
        default = cube;
        cube = {
          type = "app";
          program = nixpkgs.lib.getExe self.packages.${pkgs.stdenv.hostPlatform.system}.cube;
        };
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.cube ];
          # bear regenerates compile_commands.json for clangd: bear -- make
          packages = [
            pkgs.bear
            pkgs.clang-tools
            pkgs.gdb
          ];
        };
      });

      overlays.default = final: prev: {
        cube = self.packages.${final.stdenv.hostPlatform.system}.cube;
      };
    };
}

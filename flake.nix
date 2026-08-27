{
  description = "spinning cube (and the other platonic solids) for your terminal";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = fn: nixpkgs.lib.genAttrs systems (system: fn nixpkgs.legacyPackages.${system});

      mkCube =
        pkgs:
        let
          inherit (pkgs) lib;
        in
        pkgs.stdenv.mkDerivation {
          pname = "cube";
          version = "1.1.1";

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

          makeFlags = [ "PREFIX=$(out)" ];

          meta = {
            description = "spinning cube (and the other platonic solids) for your terminal";
            homepage = "https://github.com/noahburchell/cube";
            license = lib.licenses.gpl3Only;
            mainProgram = "cube";
            platforms = systems;
          };
        };
    in
    {
      packages = forAllSystems (pkgs: rec {
        default = cube;
        cube = mkCube pkgs;
      });

      apps = forAllSystems (pkgs: rec {
        default = cube;
        cube = {
          type = "app";
          program = nixpkgs.lib.getExe self.packages.${pkgs.stdenv.hostPlatform.system}.cube;
        };
      });

      # so 'nix flake check' actually builds the thing.
      checks = forAllSystems (pkgs: {
        cube = self.packages.${pkgs.stdenv.hostPlatform.system}.cube;
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.cube ];
          # bear regenerates compile_commands.json for clangd: bear -- make
          packages = [
            pkgs.bear
            pkgs.clang-tools
            (if pkgs.stdenv.hostPlatform.isDarwin then pkgs.lldb else pkgs.gdb)
          ];
        };
      });

      overlays.default = final: prev: {
        cube = mkCube final;
      };
    };
}

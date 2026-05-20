{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs = {nixpkgs, ...}: let
    system = "x86_64-linux";
  in {
    devShells.${system}.default = nixpkgs.legacyPackages.${system}.mkShell {
      packages = with nixpkgs.legacyPackages.${system}; [
        clang
        clang-tools
        cmake
        gtest
      ];

      shellHook = ''
        if [ -f .env.local ]; then
          source .env.local
        fi
      '';
    };
  };
}

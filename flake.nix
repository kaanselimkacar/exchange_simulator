{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs = {nixpkgs, ...}: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    devShells.${system}.default = (pkgs.mkShell.override { stdenv = pkgs.clangStdenv; }) {
      nativeBuildInputs = with pkgs; [
        clang-tools
        cmake
      ];

      buildInputs = with pkgs; [
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

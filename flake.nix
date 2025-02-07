# TODO: currently we have to run CC=clang CXX=clang++ meson setup build
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    {
      overlays.default = final: prev:
        let
          system = prev.stdenv.hostPlatform.system;
        in {
          # sapf = TODO;
          libdispatch = final.callPackage ./nix/libdispatch/default.nix {
            stdenv = final.llvmPackages_16.stdenv;
          };
        };
    } //
    (flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [self.overlays.default];
        };
      in rec {
        # packages.default = pkgs.sapf;

        devShell = pkgs.mkShell.override {
          stdenv = pkgs.llvmPackages_16.stdenv;
        } {
          buildInputs = with pkgs; [
            fftw
            libdispatch
            libedit
            meson
            ninja
          ];
        };
      }
    ));
}

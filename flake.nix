{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    let
      clangStdenv = pkgs: pkgs.llvmPackages_16.stdenv;
    in
      {
        overlays.default = final: prev:
          let
            system = prev.stdenv.hostPlatform.system;
          in {
            # sapf = TODO;
            libdispatch = final.callPackage ./nix/libdispatch/default.nix {
              stdenv = clangStdenv final;
            };
          };
      } //
      (flake-utils.lib.eachDefaultSystem (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [self.overlays.default];
          };
          stdenv = clangStdenv pkgs;
        in rec {
          # packages.default = pkgs.sapf;

          devShell = pkgs.mkShell.override {
            inherit stdenv;
          } {
            buildInputs = with pkgs; [
              fftw
              libdispatch
              libedit
              meson
              ninja
            ];

            CC = "${stdenv}/bin/clang";
            CXX = "${stdenv}/bin/clang++";
          };
        }
      ));
}

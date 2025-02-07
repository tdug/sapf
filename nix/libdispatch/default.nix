{
  cmake,
  lib,
  fetchFromGitHub,
  stdenv,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "libdispatch";
  version = "1.3";

  src = fetchFromGitHub {
    owner = "swiftlang";
    repo = "swift-corelibs-libdispatch";
    rev = "swift-6.0.3-RELEASE";
    hash = "sha256-vZ0JddR31wyFOjmSqEuzIJNBQIUgcLpjabbnlSF3LqY=";
  };

  nativeBuildInputs = [
    cmake
  ];
  
  meta = {
    homepage = "https://github.com/swiftlang/swift-corelibs-libdispatch";
    description = "The libdispatch Project, (a.k.a. Grand Central Dispatch), for concurrency on multicore hardware";
    license = lib.licenses.asl20;
    platforms = lib.platforms.unix;
  };
})

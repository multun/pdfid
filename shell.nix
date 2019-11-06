with import <nixpkgs> {};
mkShell {
  buildInputs = [
    bear gmpxx gmp
    cmake zlib freetype libjpeg libtiff fontconfig
    openssl libpng lua5 pkgconfig libidn expat
  ];
}

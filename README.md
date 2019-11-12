<p align="center">
    <img src="https://dl.multun.net/pdfid-logo.png" width="400px" alt="pdfid logo" />
</p>

[pdfid](https://github.com/multun/pdfid) is a tool for hiding small amounts of data in pdf files. It uses a modified version of [PoDoFo](http://podofo.sourceforge.net/).

It was designed to enable identifying publishers of confidential data.

# Pros

 - the size of the output file does not depend on the hidden data.
 - no destructive transformation is performed. the document should display in the _exact same way_.
 - uses very simple steganography.

# Cons

 - each pdf file only has a limited hidden storage space.
 - when reading, there's no way to know how much of the hidden storage space of the pdf is actually used. The `-r` option just reads all of it.
 - parsing and re-writting a PDF file destroys all hidden data.

# Usage

```
sh$ pdfid capacity document.pdf
291
sh$ du -b hidden-data.txt
4	hidden-data.txt
sh$ pdfid write document.pdf document-for-bob.pdf hidden-data.txt
sh$ pdfid read document-for-bob.pdf | head -c 4 > recovered-data.txt
sh$ diff recovered-data.txt hidden-data.txt
sh$ echo $?
0
```

# Build instructions

The build process is a bit awkward, as pdfid is an in-tree patch of a library.

The package depends on all packages needed by PoDoFo, plus `libgmp-dev`.

```
 cmake
 libboost-dev
 libcppunit-dev
 libfontconfig1-dev
 libfreetype6-dev
 libidn11-dev
 libjpeg-dev
 liblua5.1-0-dev
 libssl-dev
 libtiff-dev
 libunistring-dev
 zlib1g-dev
 libgmp-dev
```

``` sh
mkdir build
cd build

# prepare the out of tree build.
# the library must be staticaly linked against pdfid, as the shared
# library would conflict with the unpatched version of PoDoFo.
cmake -DPODOFO_BUILD_SHARED=OFF \
      -DPODOFO_BUILD_STATIC=ON \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_BUILD_TYPE=Release ..

# only build what's required.
# the `all` target build all tests and tools
make pdfid

# run the correct subset of `make install`
DESTDIR="${DESTDIR:?missing DESTDIR}" cmake -DCOMPONENT=pdfid -P ./cmake_install.cmake
```

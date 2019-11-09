<p align="center">
    <img src="https://dl.multun.net/pdfid-logo.png" width="400px" alt="pdfid logo" />
</p>

[pdfid](https://github.com/multun/pdfid) is a tool for hiding small amounts of data in pdf files. It uses a modified version of [PoDoFo](http://podofo.sourceforge.net/).

It was to designed enable identifying publishers of confidential data.

# Pros

 - the size of the output file does not depend on the hidden data.
 - no destructive transformation is performed. the document should display in the _exact same way_.
 - uses very simple steganography.

# Cons

 - each pdf file only has a limited hidden storage space.
 - when reading, there's no way to know how much of the hidden storage space of the pdf is actually used. The `-r` option just reads all of it.
 - parsing and re-writting a PDF file destroys all hidden data.

# GEL++


C++ library dedicated to read and process executable/library files.
Basically GEL++ is devoted to ELF binary format (GEL stands for Generic ELF Loader)
but its open interface is able to support other formats like COFF.

## COFF support through COFFI Library

[COFFI](https://github.com/serge1/COFFI) is library providing load for COFF
files. GEL++ provides an interface to COFF files through COFFI.

To use it, pull the sumbodule:

```sh
$ git submodule --init
```

This is equivalent to a clone of COFFI in the top-level of the source directory of GEL++, as if you ran

```sh
$ git clone https://github.com/serge1/COFFI.git coffi
```

libx6
==========================

This C/C++ shared library enables interaction with the BBN custom firmware for the [Innovative Integration X6-1000](http://www.innovative-dsp.com/products.php?product=X6-1000M) FPGA card as a data acquisition card for superconducting qubit experiments.


Building Quickstart
----------------------
Since we use the Malibu library from II we support building on Windows only.

### Requirements 
* cmake build tool version 2.8 or higher (http://www.cmake.org/)
* gcc: g++ 4.7.3.  Using the mingw-builds distribution.  This version is necessary for ABI compatibility with the shared libraries from Malibu. 
* Malibu: [Innovative Integration Malibu library](http://www.innovative-dsp.com/products.php?product=Malibu)

### Instructions

```shell
mkdir build
cd build
cmake -G "MSYS Makefiles" ../src/
make
```

LICENSE
------------

Licensed under the Apache License v2.
# libx6

This C/C++ shared library enables interaction with the BBN custom firmware for the [Innovative Integration X6-1000](http://www.innovative-dsp.com/products.php?product=X6-1000M) FPGA card as a data acquisition card for superconducting qubit experiments.


#Building
----------------------

N.B. This is only necessary if you wish to develop the library. BBN ships releases with a pre-compiled shared library.

## Windows

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

## Linux

It is possible to develop on Linux as well.  Download the following rpm's from the Innovative website by searching for [Malibu](http://www.innovative-dsp.com/cgi-bin/dlLinux64.cgi?product=64Malibu) and [MalibuRED](http://www.innovative-dsp.com/cgi-bin/dlLinux64.cgi?product=64MalibuRed).

* 64WinDriver-11.5-0.x86_64.rpm
* intel-ipp-sp1-293-7.0-6.x86_64.rpm
* 64Malibu-LinuxPeriphLib-1.7-9b.x86_64.rpm

The LinuxNotes.pdf is also a useful reference.

Move all the rpms to an ``Innovative`` folder and then use ``rpm2cpio`` and ``cpio`` to extract the files. E.g

```shell
rpm2cpio 64WinDriver-11.5-0.x86_64.rpm | cpio -idmv
```

Then we just need to let cmake know where all the shared libraries are by specifying the folder we unarchived everything in as INNOVATIVE_PATH.  On Linux Mint 17.1 I also needed to install the Intel version of OpenMP with ``sudo apt-get install libiomp-dev``.

Finally to get the WinDriver shared library to link we need to make a symbolic link to the latest version

```shell
cd /INNOVATIVE_PATH/usr/Innovative/WinDriver-11.5/lib
ln -s libwdapi1150.so libwdapi.so
```

If you actually have the X6 card in a Linux machine and need to build the WinDriver kernel module then this configure worked on Linux Mint 17.1:

```shell
./configure --with-kernel-source=/usr/src/linux-headers-3.13.0-24-generic --disable-usb-support
```



# LICENSE

Licensed under the Apache License v2.

# libx6

This C/C++ shared library enables interaction with the BBN custom firmware for the
[Innovative Integration
X6-1000](http://www.innovative-dsp.com/products.php?product=X6-1000M) FPGA card
as a data acquisition card for superconducting qubit experiments.

# Dependencies
* Test cases use [philsquared/Catch](https://github.com/philsquared/Catch). Pull the submodule using:
```
git submodule init
git submodule update
```

# Building
----------------------

N.B. This is only necessary if you wish to develop the library. BBN ships
releases with a pre-compiled shared library.

## Windows

### Requirements
* cmake build tool version 2.8 or higher (http://www.cmake.org/)
* Innovative Integration development libraries. Use the [Install from
Web](http://www.innovative-dsp.com/support/installfromwebAutomatic.htm) tool
from II to download and install the X6-1000M - PCIe XMC Module Development Kit.
As of September 2015 we have built against Malibu 1.8.5.
* C++11 compliant compiler.  If using the gcc compiler stack, II has used a
specific compiler in the Malibu libraries that libx6 links against - as of
Malibu 1.8.5 this is TDM GCC 4.8.1 ( use `strings -a libFramework_Mb.a | grep
GCC` to discover). We have used [mingw-w64](http://mingw-w64.org/) and the
mingw-builds installer to use the ``x86_64-4.8.1-posix-seh-rt_v3_rev2`` stack
which is compatible with the compiler II used.  It is easiest to use this with
the MSYS2 environment for cmake and make.
### Uninstalling previous II drivers and software

If you already have II software installed and want to start from scratch try the
following modified from end of this [forum
thread](http://www.innovative-dsp.com/forum/viewtopic.php?t=2032).

1. Open Device Manager and disable the WinJungo driver. Careful if you have
other WinJungo devices like Xilinx USB JTAG cables.
2. Open a PowerShell as administrator.
3. Move to  `C:\Innovative\Driver` (not wdreg as the forum says and might be "Drivers")
4. Run ` .\wdreg_gui.exe -inf .\windrvr6.inf uninstall` (might be `IIWDrvr1190.inf`)
5. Run ` .\wdreg_gui.exe -name kp_malibu uninstall` (may not be there)
6. Run ` .\wdreg_gui.exe -name memdrv uninstall` (may not be there)
7. Uninstall and remaining II Jungo drivers using Device Manager.
8. Uses MSYS2 grep to find all files in `C:\Windows\INF` that contain 1303 and remove them.
9. In the `C:\windows\system32\drivers` folder. Delete kp_malibu.sys, memdrv.sys, and windrvr6.sys
10. Delete `C:\Inovative` folder.
11. Restart

### Instructions

#### MSYS2 Development Environment Setup

1. [Download](http://msys2.github.io/) the MSYS2 installer and follow the instructions to install and update in place.
2. Use pacman package manager to install some additional tools and libraries:

  ```bash
  pacman -S make
  pacman -S mingw64/mingw-w64-x86_64-cmake
  ```
3. Put the mingw-w64 gcc 4.8.1 compiler on the path

  ```bash
  export PATH=/c/Program\ Files/mingw-w64/x86_64-4.8.1-posix-seh-rt_v3-rev2/mingw64/bin:$PATH
  ```

#### Building
Assuming a default install of the II software into ``C:\Innovative``:
```shell
mkdir build
cd build
cmake -G "MSYS Makefiles" -DINNOVATIVE_PATH="C:\\Innovative" ../src/
make
```

## Linux

It is possible to develop on Linux as well.  Download the following rpm's from
the Innovative website by searching for
[Malibu](http://www.innovative-dsp.com/cgi-bin/dlLinux64.cgi?product=64Malibu)
and
[MalibuRED](http://www.innovative-dsp.com/cgi-bin/dlLinux64.cgi?product=64MalibuRed).
As of September 2015 we have used

* 64Malibu-LinuxRed-1.3-4.x86_64.rpm
* 64Malibu-LinuxPeriphLib-1.8-1b.x86_64.rpm
* 64WinDriver-11.5-0.x86_64.rpm
* intel-ipp-sp1-293-7.0-6.x86_64.rpm

The LinuxNotes.pdf is also a useful reference.

Move all the rpms to an ``INNOVATIVE_PATH`` folder and then use ``rpm2cpio`` and
``cpio`` to extract the files. E.g

```shell
rpm2cpio 64WinDriver-11.5-0.x86_64.rpm | cpio -idmv
```

Then we just need to let cmake know where all the shared libraries are by
specifying the folder we unarchived everything in as INNOVATIVE_PATH.  On Linux
Mint 17.1 I also needed to install the Intel version of OpenMP with ``sudo
apt-get install libiomp-dev``.

Finally to get the WinDriver shared library to link we need to make a symbolic
link to the latest version

```shell
cd /INNOVATIVE_PATH/usr/Innovative/WinDriver-11.5/lib
ln -s libwdapi1150.so libwdapi.so
```

If you actually have the X6 card in a Linux machine and need to build the
WinDriver kernel module then this configure worked on Linux Mint 17.1:

```shell
./configure --with-kernel-source=/usr/src/linux-headers-3.13.0-24-generic --disable-usb-support
```


# LICENSE

Licensed under the Apache License v2.

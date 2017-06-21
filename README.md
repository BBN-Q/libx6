# libx6

This C/C++ shared library enables interaction with the [BBN QDSP firmware](https://github.com/BBN-Q/BBN-QDSP-X6)
for the [Innovative Integration X6-1000](http://www.innovative-dsp.com/products.php?product=X6-1000M)
FPGA card as a data acquisition card for superconducting qubit experiments.

# Installation

The libx6 shared library binaries are available for windows and linux in BBN's
[conda](https://www.continuum.io/downloads) channel. These libraries depend
on Innovative Integration's Malibu library, which must be obtained directly
from the vendor. After following the Malibu installation instructions below, you
can install libx6 by simply executing:
```
conda install -c bbn-q libx6
```

This command installs the shared libraries, header files, and python `libx6`
module into your Anaconda environment folder. To use libx6 from C++, MATLAB, or
Julia you must add `~/anaconda3/lib` to `LD_LIBRARY_PATH` on linux/mac, or add
`HOMEDIR/Anaconda3/Library/bin` to the `PATH` on windows. The MATLAB and Julia
wrappers can be obtained by cloning the source repository:
```
git clone --recursive https://github.com/BBN-Q/libx6.git
```
or downloading and unzipping the most recent release.

# Building
----------------------

N.B. This is only necessary if you wish to develop the library. BBN ships
releases with a pre-compiled shared library.

## Dependencies

1. C++11 compliant compiler
2. Test cases use [Catch](https://github.com/philsquared/Catch) as a
git submodule.
3. [CMake](https://cmake.org/) >= 3.2
4. Innovative Integration libraries.

## Windows

### Requirements

* Innovative Integration development libraries. Use the [Install from
Web](http://www.innovative-dsp.com/support/installfromwebAutomatic.htm) tool
from II to download and install the X6-1000M - PCIe XMC Module Development Kit.
As of September 2015 we have built against Malibu 1.8.5.
* C++11 compliant compiler.  If using the gcc compiler stack, II has used a
specific compiler in the Malibu libraries that libx6 links against - as of
Malibu 1.8.5 this is TDM GCC 4.8.1 ( use `strings -a libFramework_Mb.a | grep
GCC` to discover). We have used [mingw-w64](http://mingw-w64.org/) and the
mingw-builds installer to get the ``x86_64-4.8.1-posix-seh-rt_v3_rev2`` stack
which is compatible with the compiler II used.  It is easiest to run this
within the MSYS2 environment for access to cmake and make.

### Uninstalling previous II drivers and software

If you already have II software installed and want to start from scratch try the
following modified from the end of this [forum
thread](http://www.innovative-dsp.com/forum/viewtopic.php?t=2032).

1. Open Device Manager and disable the WinJungo driver. Careful if you have
other WinJungo devices like Xilinx USB JTAG cables.
2. Open a PowerShell as administrator.
3. Move to  `C:\Innovative\Driver` (not wdreg as the forum says and might be "Drivers")
4. Run ` .\wdreg_gui.exe -inf .\windrvr6.inf uninstall` (might be `IIWDrvr1190.inf`)
5. Run ` .\wdreg_gui.exe -name kp_malibu uninstall` (may not be there)
6. Run ` .\wdreg_gui.exe -name memdrv uninstall` (may not be there)
7. Uninstall any remaining II Jungo drivers using Device Manager.
8. Use MSYS2 grep to find all files in `C:\Windows\INF` that contain 1303 and remove them.
9. In the `C:\windows\system32\drivers` folder, delete: kp_malibu.sys, memdrv.sys, and windrvr6.sys
10. Delete `C:\Innovative` folder.
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

### Innovative Integration Setup

1. download all files for II-X6 by going to Support->Software Downloads then Linux - 64bit - X6-1000M
or http://www.innovative-dsp.com/cgi-bin/dlLinux64.cgi?product=64X6-1000M. The current file list is:
    1. 64Malibu-LinuxPeriphLib-1.9-1f.tar.gz
    1. 64Malibu-LinuxRed-1.4-0.tar.gz
    1. 64WinDriver-12.1-0.x86_64.tar.gz
    1. 64X6-1000M-LinuxPeriphLib-1.2-5c.tar.gz
    1. Ipp9_redist.tar
    1. LinuxNotes.pdf
    1. Make_All.pro
    1. Setup_Debian.sh
    1. Setup.sh
    1. X6-1000M_Firmware.zip
2. copy all files above to `/usr/Innovative`
3. link kernel source in `/usr/src`: `sudo ln -s kernels/3.10.0-514.2.2.el7.x86_64 linux`
4. run the Innovative `Setup.sh` script
5. setup links for Ipp
    ```shell
    cd /usr/Innovative/Lib/Gcc
    sudo ln -s libippcore-9.0.a libippcore.a
    sudo ln -s libipps-9.0.a libipps.a
    ```
6. copy the X6-1000M folder from `/usr/Innovative` to some user folder
7. install `qt5-qtbase-devel`
8. rebuild the Finder and VsProm applets but first modify the `.pro` files to add `LIBS += -ldl`
    ```shell
    cd /path/to/X6-1000M/Applets/Finder/Qt
    qmake-qt5 Finder.pro
    make
    ./Release/Finder
    ```
9. Program the latest QDSP firmware using the VsProm applets
10. Load the kernel driver on boot
    1. Add a file to `/etc/systemd/system` called `iix6.service`

        [Unit]
        Description=Load Kernel driver for II X6

        [Service]
        Type=oneshot
        ExecStart=/bin/sh -c "/usr/Innovative/WinDriver/util/wdreg windrvr1221;chmod 666 /dev/windrvr1221;insmod /usr/Innovative/KerPlug/linux/LINUX*/kp_malibu_module.ko"

        [Install]
        WantedBy=multi-user.target

    2. Enable the service `sudo systemctl enable iix6.service`
10. Reboot the computer with a full power down.

### Building libx6

    ```shell
    git clone --recursive https://github.com/BBN-Q/libx6.git
    cd libx6
    mkdir build
    cd build
    cmake ../src
    make -j4
    ./run_tests
    ```

# License

Licensed under the Apache License v2.

# Funding

This software was funded in part by the Office of the Director of National
Intelligence (ODNI), Intelligence Advanced Research Projects Activity (IARPA),
through the Army Research Office contract No. W911NF-10-1-0324 and No.
W911NF-14-1-0114. All statements of fact, opinion or conclusions contained
herein are those of the authors and should not be construed as representing the
official views or policies of IARPA, the ODNI, or the US Government.

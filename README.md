# ProThOS: programmable taskflow oriented operating system

* https://manythreads.github.io/prothos/

# Quick Usage Guide

## Running ProThOS with MyThOS in the QEMU virtual machine

* First, run `git submodule init && git submodule update && mythos/3rdparty/mcconf/install-python-libs.sh && mythos/3rdparty/install-libcxx.sh` in order to install the needed libraries for the build configuration tool. This requires python, pip, qemu and virtualenv.
* Now you can run `make` in the root folder. This will assemble the source code into the subfolder `prothos-amd64`.
* Change into the `prothos-amd64` folder and run `make qemu`. This will compile the init application, the kernel, and finally boot the kernel image inside the qemu emulator. The debug and application output will be written to the console.
* Whenever you add or remove files from modules (the `mcconf.module` files in the `prothos` folder), rerun `make` in the root folder and then `make clean` in the target-specific folder.

# Acknowledgements

The ProThOS project is funded by the Federal Ministry of Education and Research (BMBF) under Grant No. 01IH16011 from January 2017 to December 2019. The grant was part of the 5th HPC-Call of the Gauss Allianz.

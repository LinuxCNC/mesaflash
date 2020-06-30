# mesaflash

Configuration and diagnostic tool for Mesa Electronics PCI(E)/ETH/EPP/USB/SPI boards

Quickstart:
===========

MesaFlash depends on a couple of packages to build, so install those
first:

  sudo apt install libpci-dev pkg-config

You may need to install git first:

  sudo apt install git

Clone MesaFlash:

  git clone https://github.com/LinuxCNC/mesaflash.git

From the top level directory, switch to the source directory:

  cd mesaflash

In the source directory to build MesaFlash:

  make

To get command line syntax from a local make:

  ./mesaflash --help

To build and install MesaFlash:

  sudo make install

To run an installed MesaFlash:

  mesaflash --help

Distributions
===============
**mesaflash** package is available on [Fedora](https://src.fedoraproject.org/rpms/mesaflash) _(version 31 or newer)_ and can be simply installed by using:
```
# dnf install mesaflash
```
Availability of **mesaflash** for other Linux distributions can be checked on [release-monitoring](https://release-monitoring.org/project/105522/) project page.

s6lxek-progutil
===============

A programming utility for the Spartan-6 LX16 Evaluation Kit
Author: Yaman Umuroglu <maltanar@gmail.com>

DISCLAIMER: Neither the author nor this utility are connected to or endorsed by Avnet. Use at your own risk.

Description
============

This is an open-source alternative to the Avnet Programming Utility, which is used for programming Avnet Spartan-6 LX16 Evaluation kits. It supports:

 - re-configuring the FPGA with a new bitstream
 - text-based serial port RX-TX operation
 - sending binary files

The utility is written in Qt and uses UNIX termios functions for serial port access. Currently it has only been tested on 64-bit Ubuntu installations (12.04 and above), though should work in Windows with a small amount of changes.

Installation
=============

Use qmake or Qt Creator to open the src/s6lxek-progutil.pro and compile the project.

To ensure that the program has access to the serial port in Linux systems, you have several options:

1) Copy the udev-rules/s6lxek.rules to the udev rules folder of your system, e.g:

sudo cp udev-rules/s6lxek.rules /etc/udev/rules.d/

2) Run the programming utility itself as superuser

3) Change r/w permissions of the corresponding serial port devnode, e.g:

sudo chmod a+rw /dev/ttyACM0


TODO:
 - code cleanup for the serial port IF code
 - code cleanup for handling the programming protocol
 - handle errors gracefully 
 - add ability to stop transmission of large files
 - receive binary files
 - more send modes
 - GUI overhaul

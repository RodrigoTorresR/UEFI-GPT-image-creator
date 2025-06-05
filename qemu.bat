#!/bin/sh
#sSendin' Out a TEST O S
qemu-system-x86_64 -drive format=raw,file=test.hdd -bios ./OVMF.fd -m 256M -vga std -name TESTOS -machine q35 -net none

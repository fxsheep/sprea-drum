#!/bin/sh
~/toolchains/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -O3 -s -static -fpermissive devmem.c main.cpp JtagInterface.cpp jtaghal.cpp JtagException.cpp TestInterface.cpp SprdMmioDJtagInterface.cpp -o jtag
scp jtag pi@192.168.1.114:/home/pi/sheep

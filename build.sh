#!/bin/sh
g++ -O3 -s -static devmem.c main.cpp JtagInterface.cpp jtaghal.cpp JtagException.cpp TestInterface.cpp SprdMmioDJtagInterface.cpp -o jtag -fpermissive

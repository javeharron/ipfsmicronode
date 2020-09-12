# ipfsmicronode
A USB-based micronode for Interplanetary File System (IPFS) based on Linux operating systems. The system enables the user to automatically repost the contents of a USB drive onto IPFS. 

1) First, you will need a Linux computer with a C compiler.
2) Put the files you want on a USB drive. 

3) Install the following packages:

   sudo apt-get install libusb-1.0-0-dev
   sudo apt-get install libudev-dev
   install IPFS as mentioned in https://www.gigenet.com/blog/an-introductory-guide-to-the-ipfs/

3) In command line, navigate to directory. To build:
   make

4) Run IPFS daemon in the Linux machine.

5) Run
   ./udblk



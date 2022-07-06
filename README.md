# Vichy
## A minimal implementation of scull (Simple Character Utility for Loading Localities)

Vichy is a minimalistic implementation for the Simple Character Utility for Loading Localities (scull), which is a character driver that treats an area of memory as if it was a device. You can find more information about scull in chapter 3 of [Linux Device Drivers, 3rd edition](https://lwn.net/Kernel/LDD3/). The source code for scull can be found [here](http://gauss.ececs.uc.edu/Courses/c4029/code/drivers/Scull/scull.html).

Vichy supports the following subset of operations supported on scull:
- Open
- Release
- Read
- Write

Vichy supports global and persistent devices analogous to scull0 - scull3 (vichy has two devices by default, vichy0 and vichy1). The other types of scull devices (scullpipe, sculluid) are not supported yet.

Another difference is the memory layout. In scull, each device is a linked list of structures, each of which points to a memory area of 4MB at most, though an array of a 1000 pointers, each pointing to a memory area of 4000 bytes. In vichy, each structure in the linked list making up the device points to just one memory area of 4000 bytes. This changes the implementation of data operations (read, write, trim) slightly.

## Setup and Installation
- Compile vichy
  - /vichy$ sudo make
- Loading the module
  - /vichy$ sudo ./vichy_load
- Unloading the module
  - /vichy$ sudo ./vichy_unload 

## Testing
- Load
  - Run ```$less /proc/devices | grep vichy``` on a terminal and confirm that vichy has been assigned a major number.
  - Run ```ls -l /dev | grep vichy``` and check if vichy0 and vichy1 have been created.
- Data management
  - Compile and run vichy_test.c or run any other read/write tests.   

# Simple-File-System
The design and implementation of a Simple File System that can be mounted by the user under a directory in the user's machine. 

Note, this is **not fully functional**


### Design Choice
- All the in memory data structures, as well as the on-disk structures can be found in *sfs_api.h*. They have all been implemented as structs and used throughout the *sfs_api.c* code.
- I have decided to do a one file approach, where all the source code can be found in *sfs_api.c*.
- I wrote the code with the assumption that a file's name could not be more than 16 characters in length.
- I have set the max block number to 26938.
- I have set the block size to 1024.
- I have written my code under the assumption that my system can hold a maximum of 100 files.
- Test 0 had an error as void wasn't specified in the *main* method signature, which I just wanted to bring attention to. 
  - The same goes for disk_emu.h and .c
  - It is for these reasons that I have decided to include all the files in the submitted zip.

#ifndef SFS_API_H
#define SFS_API_H
#include <stdlib.h>

// You can add more into this file.

struct iNode {
    int mode;
    int linkCount;
    int size;
    int directPointers[12];
    int indirectPointer;
};

struct iNodeTable {
    struct iNode iNodeTableEntries[101];
};

struct directoryEntry {
    char used;
    int iNode;
    char fileName[16];
};

struct directoryTable {
    struct directoryEntry directoryEntries[100];
};

struct openFile {
    int iNode;
    int rwPointer;
};

struct FDT {
    struct openFile fileDescriptorTable[100];
};

struct superBlock {
    int magicNum;
    int blockSize;
    int fileSystemSize;
    int iNodeTableLength;
    int rootDir;
};

struct FBM {
    char bitMap[26903];
};

void mksfs(int);

int sfs_getnextfilename(char*);

int sfs_getfilesize(const char*);

int sfs_fopen(char*);

int sfs_fclose(int);

int sfs_fwrite(int, const char*, int);

int sfs_fread(int, char*, int);

int sfs_fseek(int, int);

int sfs_remove(char*);

#endif

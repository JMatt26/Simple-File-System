#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "disk_emu.h"
#include <string.h>


int BSIZE = 1024;
int MBLOCK = 26938;
int FBM_SIZE = 26903;

// In-Memory Caches
struct FBM fbm;
struct FDT fdt;
struct iNodeTable iNodeTable;
struct directoryTable directoryTable;

char *filename = "hardDrive";

/**
 * Method to read from disk into memory
 * @param startAddress
 * @param numOfBlocks
 * @param readInfo
 * @return
 */
int readFromDisk(int startAddress, int numOfBlocks, void *structure, int sizeOfStructure) {

    char *buf = (char *)malloc(numOfBlocks * BSIZE);
    memset(buf, 0 , numOfBlocks * BSIZE);
    int numRead = read_blocks(startAddress, numOfBlocks, buf);
    memcpy(structure, buf, sizeOfStructure);

    return numRead;
}

/**
 * Method to write from to the disk
 * @param startAddress
 * @param numOfBlocks
 * @param readInfo
 * @return
 */
int writeToDisk(int startAddress, int numOfBlocks, void *structure, int sizeOfStructure) {

    char *buf = (char *)malloc(numOfBlocks * BSIZE);
    memset(buf, 0, numOfBlocks * BSIZE);
    memcpy(buf, structure, sizeOfStructure);
    int numWrote = write_blocks(startAddress, numOfBlocks, buf);

    return numWrote;
}

/**
 * Method to initialize the super block
 * @param superBlock
 * @return
 */
void initializeSuperBlock(struct superBlock *superBlock) {
    superBlock -> magicNum = 1;
    superBlock -> blockSize = BSIZE;
    superBlock -> fileSystemSize = MBLOCK;
    superBlock -> iNodeTableLength = 7;
    superBlock -> rootDir = 0;
}

/**
 * Initializes all directory entries and updates the cache
 * @param dT
 */
void initializeDirectoryEntries(struct directoryTable *dT) {
    for (int i = 0; i < 100; i++) {
        struct directoryEntry dEntry;
        dEntry.iNode = -1;
        dEntry.used = 0;
        dEntry.fileName[0] = '\0';
        dT -> directoryEntries[i] = dEntry;
    }
}

/**
 * Initializes the directory iNode, and all other iNodes to empty
 * @param iT
 */
void initializeINodes(struct iNodeTable *iT) {
    // directory iNode
    struct iNode dirINode;
    dirINode.mode = 1;
    dirINode.linkCount = 3;
    dirINode.size = 0;
    dirINode.directPointers[0] = 8;
    dirINode.directPointers[1] = 9;
    dirINode.directPointers[2] = 10;
    iT -> iNodeTableEntries[0] = dirINode;

    for (int i = 1; i < 101; i++) {
        struct iNode node;
        node.mode = 0;
        node.linkCount = 0;
        node.size = 0;
        for (int j = 0; j < 12; j++) {
            node.directPointers[j] = -1;
        }
        node.indirectPointer = -1;
        iT -> iNodeTableEntries[i] = node;
    }
}

/**
 * Initializes the bitmap
 * @param bm
 */
void initializeFBM(struct FBM *bm) {
    // first 3 blocks in bitmap assigned to directory entries
    for (int i = 0; i < 3; i++) {
        bm -> bitMap[i] = 1;
    }

    for( int i = 3; i < FBM_SIZE; i++) {
        bm -> bitMap[i] = 0;
    }
}

/**
 * Initialize File Descriptor Table
 * @param fdTable
 */
void initializeFDT(struct FDT *fdTable) {
    for (int i = 0; i < 100; i++) {
        fdTable -> fileDescriptorTable[i].iNode = -1;
        fdTable -> fileDescriptorTable[i].rwPointer = -1;
    }
}

/**
 * Allocates a data block according to the File Bit Map
 * @return The index of the available data block
 */
int allocateDataBlock(void) {
    int dataBlockIndex = -1;

    for (int i = 3; i < FBM_SIZE; i++) {
        if (fbm.bitMap[i] == 0) {
            fbm.bitMap[i] = 1;
            dataBlockIndex = i + 8;     // Addition by 8 since FBM[0] = Disk[8]
            break;
        }
    }

    if (dataBlockIndex == -1) {
        perror("No Data Block could be allocated.");
        return -1;
    } else {
        return dataBlockIndex;
    }
}

/**
 * Modifies the inode for the pointer used
 * @param table
 * @param iNodeIndex
 * @param dataBlockIndex
 */
void modifyINODE(struct iNodeTable *table, int iNodeIndex, int dataBlockIndex, int blockNum) {
    struct iNode node = table->iNodeTableEntries[iNodeIndex];

    // Assign direct pointer
    if (blockNum < 12) {
        node.directPointers[blockNum] = dataBlockIndex;
        node.linkCount += 1;
        table->iNodeTableEntries[iNodeIndex] = node;
    } else {
        // Assign indirect pointer if directs are all used

        /*
         * PSEUDOCODE
         *
         * - Assign a block, B, from the data blocks to the indirect pointer
         * - This indirect block can point to 256 blocks, so, when an available block is
         * found from the FBM, we will write the index of the corresponding data block to B.
         * - The failing condition would be if B already has 256 indices written to it
         */
    }
}


void mksfs(int option) {

    if (option == 1) {

        if (init_fresh_disk(filename, BSIZE, MBLOCK) == -1) {
            perror("Error in initializing new file system");
        }

        // Set up superBlock
        struct superBlock superBlock;
        initializeSuperBlock(&superBlock);
        writeToDisk(0, 1, &superBlock, sizeof(superBlock));

        // Set up directory
        initializeDirectoryEntries(&directoryTable);
        writeToDisk(8, 3, &directoryTable, sizeof(directoryTable));

        // Set up INodeTable
        initializeINodes(&iNodeTable);
        writeToDisk(1, 7, &iNodeTable, sizeof(iNodeTable));

        // Set up Free Bit Map
        initializeFBM(&fbm);
        writeToDisk(26910, 27, &fbm, sizeof(fbm));

        // Set up FDT
        initializeFDT(&fdt);

    } else {
        if (init_disk(filename, BSIZE, MBLOCK) == -1) {
            perror("Error in initializing existing file system");
        }

        // Read INode Table into memory
        readFromDisk(1, 7, &iNodeTable, sizeof(iNodeTable));

        // Read DirectoryTable into memory
        readFromDisk(8, 3, &directoryTable, sizeof(directoryTable));

        // Read Free Bit Map into memory
        readFromDisk(26910, 27, &fbm, sizeof(fbm));
    }
}

int sfs_fopen(char* file) {
    int fdNum = -1;

    // finds next available fdt entry
    for (int i = 0; i < 100; i++) {
        struct openFile entry = fdt.fileDescriptorTable[i];
        if (entry.iNode == -1) {
            fdNum = i;
            break;
        }
    }

    if (fdNum == -1) {
        perror("File Descriptor Table is full. File cannot be opened");
    } else {

        // checks if file is in directory
        for (int i = 0; i < 100; i++) {
            struct directoryEntry entry = directoryTable.directoryEntries[i];
            if (entry.used == 1) {
                if (strcmp(entry.fileName, file) == 0) {
                    struct openFile openedFile = fdt.fileDescriptorTable[fdNum];
                    openedFile.iNode = entry.iNode;
                    openedFile.rwPointer = iNodeTable.iNodeTableEntries[openedFile.iNode].size;
                    fdt.fileDescriptorTable[fdNum] = openedFile;
                }
            }
        }

        if (fdt.fileDescriptorTable[fdNum].iNode == -1) {

            int iNodeTableEntry = -1;
            int dirNum = -1;

            for (int i = 1; i < 101; i++) {
                if (iNodeTable.iNodeTableEntries[i].mode == 0) {
                    iNodeTableEntry = i;
                    break;
                }
            }

            if (iNodeTableEntry == -1) {
                perror("No more available spots in I-Node Table");
            } else {

                struct iNode node = iNodeTable.iNodeTableEntries[iNodeTableEntry];
                node.mode = 1;
                node.size = 0;
                node.linkCount = 0;
                iNodeTable.iNodeTableEntries[iNodeTableEntry] = node;

                for (int i = 0; i < 100; i++) {
                    struct directoryEntry entry = directoryTable.directoryEntries[i];
                    if (entry.used == 0) {
                        dirNum = i;
                        break;
                    }
                }

                if (dirNum == -1) {
                    perror("Directory Table is full. File cannot be added");
                } else {
                    directoryTable.directoryEntries[dirNum].used = 1;
                    directoryTable.directoryEntries[dirNum].iNode = iNodeTableEntry;
                    strcpy(directoryTable.directoryEntries[dirNum].fileName, file);

                    fdt.fileDescriptorTable[fdNum].iNode = iNodeTableEntry;
                    fdt.fileDescriptorTable[fdNum].rwPointer = 0;

                    writeToDisk(8, 3, &directoryTable, sizeof(directoryTable));
                    writeToDisk(1, 7, &iNodeTable, sizeof(iNodeTable));

                }
            }
        }
    }

    return fdNum;
}

int sfs_fclose(int fd) {
    if (fdt.fileDescriptorTable[fd].iNode != -1) {
        fdt.fileDescriptorTable[fd].iNode = -1;
        fdt.fileDescriptorTable[fd].rwPointer = -1;
        return 0;
    } else {
        perror("File could not be found in FDT");
        return -1;
    }
}

int sfs_fwrite(int fd, const char* buf, int length) {
    int iNodeIndex = fdt.fileDescriptorTable[fd].iNode;
    int blockNum = fdt.fileDescriptorTable[fd].rwPointer / BSIZE;
    while (length > 0) {
        int dataBlockIndex;
        if (iNodeTable.iNodeTableEntries[iNodeIndex].directPointers[blockNum] == -1) {
            dataBlockIndex = allocateDataBlock();
            modifyINODE(&iNodeTable, iNodeIndex, dataBlockIndex, blockNum);
        } else {
            dataBlockIndex = iNodeTable.iNodeTableEntries[iNodeIndex].directPointers[blockNum];
        }

        writeToDisk(dataBlockIndex, 1, (char *) buf, BSIZE); // write data
        length = length - (BSIZE);
    }
    iNodeTable.iNodeTableEntries[iNodeIndex].size = length; // update size of file once writing is done
    fdt.fileDescriptorTable[fd].rwPointer = length;         // update pointer once writing is done

    // update modifications on disk
    writeToDisk(26910, 27, &fbm, sizeof(fbm));
    writeToDisk(1, 7, &iNodeTable, sizeof(iNodeTable));

    return length;
}

int sfs_fread(int fd, char* buf, int length) {
    int iNodeIndex = fdt.fileDescriptorTable[fd].iNode;
    int blockNum = fdt.fileDescriptorTable[fd].rwPointer / BSIZE;
    while (length > 0) {
        int dataBlockIndex = iNodeTable.iNodeTableEntries[iNodeIndex].directPointers[blockNum];

        /*
         * PSEUDOCODE for Reading the IndirectPointers
         * - If the value of blockNum is >= 12, then we have to read from the indirect pointer.
         * - Take block B that the indirect pointer points to and read the data from it
         * - Get the indices of the data blocks that B points to and read from the respective one
         */

        readFromDisk(dataBlockIndex, 1, buf, length);
        length = length - BSIZE;
    }

    int bytesRead = 0;
    for (int i = 0; buf[i] != '\0'; i++) {
        bytesRead++;
    }
    return bytesRead;
}

int sfs_fseek(int fd, int location) {
    if (fdt.fileDescriptorTable[fd].iNode == -1) {
        perror("File could not be found in FDT");
        return -1;
    } else {
        fdt.fileDescriptorTable[fd].rwPointer = location;
        return 0;
    }
}

int sfs_remove(char* file) {

    int fileINode = -1;
    // clears directory entry
    for (int i = 0; i < 100; i++) {
        struct directoryEntry entry = directoryTable.directoryEntries[i];
        if (entry.used == 1) {
            if (strcmp(entry.fileName, file) == 0) {
                entry.used = 0;
                fileINode = entry.iNode;
                entry.iNode = -1;
                memset(entry.fileName, 0, sizeof(entry.fileName));
            }
        }
    }

    if (fileINode == -1) {
        perror("File could not be found");
        return -1;
    }

    struct iNode node = iNodeTable.iNodeTableEntries[fileINode];

    for (int i = 0; i < 12; i++) {
        int dataBlockIndex = node.directPointers[i];
        int bitMapIndex = dataBlockIndex - 8;
        fbm.bitMap[bitMapIndex] = 0;
    }

    /*
     * PSEUDOCODE for clearing the Indirect Pointers
     * - Load the block B that the indirect pointer points to
     * - For every index that B points to, set its respective bit map index to 0
     * - Once all the indices B points to have been cleared, set B to 0
     */

    node.size = 0;
    node.mode = 0;
    node.linkCount = 0;
    node.indirectPointer = -1;
    for (int i = 0; i < 12; i++) {
        node.directPointers[i] = -1;
    }
    iNodeTable.iNodeTableEntries[fileINode] = node;

    // Updates the disk
    writeToDisk(1,7, &iNodeTable, sizeof(iNodeTable));
    writeToDisk(26910, 27, &fbm, sizeof(fbm));

    return 0;
}




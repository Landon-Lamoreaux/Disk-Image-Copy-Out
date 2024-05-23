#include <iostream>
#include "sfs_superblock.h"
#include "sfs_inode.h"
#include <cstring>
#include "sfs_dir.h"
#include <fstream>


extern "C" {
#include "driver.h"
}

using namespace std;

void GetFileBlock(sfs_inode_t * node, size_t blockNumber, void* data);

int main(int argv, char** argc) {
    int i = 0;
    int j;
    int fileNum;
    ofstream fout;
    bool remain;

    if (argv != 3) {
        cout << "Incorrect number of arguments" << endl;
        return -1;
    }

    /* declare a buffer that is the same size as a filesystem block */
    char raw_superblock[128];

    sfs_superblock *super = (sfs_superblock *) raw_superblock;
    sfs_inode inode[128/sizeof(sfs_inode)];

    string diskImage = argc[1];
    char* disk = const_cast<char *>(diskImage.c_str());

    /* open the disk image and get it ready to read/write blocks */
    driver_attach_disk_image(disk, 128);

    /* CHANGE THE FOLLOWING CODE SO THAT IT SEARCHES FOR THE SUPERBLOCK */

    while(true) {

        // Read a block from the disk image.
        driver_read(super,i);

        // Is it the filesystem superblock?
        if(super->fsmagic == VMLARIX_SFS_MAGIC && !strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR))
        {
            break;
        }
        i++;
    }

    sfs_dirent data[128/sizeof(sfs_dirent)];
    string fileName = argc[2];

    driver_read(inode, super->inodes);
    int extra = inode[0].size % 128 == 0 ? 0 : 1;
    for (i = 0; i < inode[0].size / 128 + extra; i++)
    {
        GetFileBlock(&inode[0], i, data);
        j = 0;
        while(j < 128/sizeof(sfs_dirent) && !((string)data[j].name).empty())
        {
            if ((string)data[j].name == fileName)
            {
                // Jumping out of the loop when we find the right block.
                goto gotBlock;
            }
            j++;
        }
    }
    cout << "Couldn't find the file." << endl;
    return -1;

    // Jump point.
    gotBlock:

    sfs_inode node[128/sizeof(sfs_inode)];

    driver_read(node, data[j].inode / 2 + super->inodes);

    fileNum = data[j].inode % 2 == 0 ? 0 : 1;
    remain = node[fileNum].size % 128 != 0;

    fout.open(argc[2], ios::trunc);

    char rawData[128];
    string fileData = "";

    // Writing all the data to the file.
    for (i = 0; i < node[fileNum].size / 128; i++) {
        GetFileBlock(&node[fileNum], i, rawData);
        fout.write(rawData, 128);
    }

    // If there is anything remaining, output it to the file.
    if(remain)
    {
        GetFileBlock(&node[fileNum], i, rawData);
        fout.write(rawData, node[fileNum].size % 128);
    }

    // Close the output file.
    fout.close();

    // Close the disk image.
    driver_detach_disk_image();

    return 0;
}


void GetFileBlock(sfs_inode_t * node, size_t blockNumber, void* data)
{
    uint32_t ptrs[128];

    // direct
    if (blockNumber < 5)
    {
        //printf("pass\n");
        driver_read(data, node->direct[blockNumber]);
    }
        // indirect
    else if (blockNumber < (5 + 32))
    {
        driver_read(ptrs, node->indirect);
        driver_read(data, ptrs[blockNumber - 5]);
    }
        // double indirect
    else if (blockNumber < (5 + 32 + (32 * 32)))
    {
        driver_read(ptrs, node->dindirect);
        int tmp = (blockNumber - 5 - 32) / 32;
        driver_read(ptrs, ptrs[tmp]);
        tmp = (blockNumber - 5 - 32) % 32;
        driver_read(data, ptrs[tmp]);
    }
        // triple indirect
    else if (blockNumber < (5 + 32 + (32 * 32) + (32 * 32 * 32)))
    {
        driver_read(ptrs, node->tindirect);
        int tmp = (blockNumber - 5 - 32 - (32 * 32)) / (32 * 32);
        driver_read(ptrs, ptrs[tmp]);
        tmp = ((blockNumber - 5 - 32 - (32 * 32)) / 32) % 32;
        driver_read(ptrs, ptrs[tmp]);
        tmp = (blockNumber - 5 - 32 - (32 * 32)) % 32;
        driver_read(data, ptrs[tmp]);
    }
    else
    {
        printf("Error in block fetch, out of range\n");
        exit(1);
    }
}
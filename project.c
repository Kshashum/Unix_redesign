/* Author: Krishna Chaitanya Shashum, Mugdha Danda

* UTD ID: KXS190079, MXD200016

* CS 5348.001 Operating Systems

* Prof. S Venkatesan

* Project - 2 part (2)*
 
***************
* Compilation :-$ gcc fsaccess.c -o fsaccess

* Run using :- $ ./fsaccess
 
****************
 
 User Input:
     - initfs (file path) (# of total system blocks) (# of System i-nodes)
     - cpin (v6 file path) (external file path)
     - cpout (output file path) (v6 file path)
     - mkdir (directory name)
     - cd (path)
     - rm (path)
     - ls (path)
     - q

 File name is limited to 14 characters.
 
*******************************************************************
 This program allows user to do four things:
   1. initfs: Initilizes the file system and redesigning the Unix file system to accept large 
      files of up tp 4GB, expands the free array to 152 elements, expands the i-node array to 
      200 elemnts, doubles the i-node size to 64 bytes and other new features as well.
  2. cpin: Given an external file, it creates an v6file to the file system with same contents as external.
  3.cpout:  If the v6-file exists, creates externalfile and makes the externalfile's contents equal to v6-file.
  4.q: save all work and exit the program.
  5.rm : removes the file
  6.cd : changes the directory
  7.ls : lists out the files in the directory
  8.mkdir : creates a new directory
   
 
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define FREE_SIZE 152
#define I_SIZE 200
#define BLOCK_SIZE 1024
#define ADDR_SIZE 11
#define INPUT_SIZE 256

// Superblock Structure

typedef struct
{
    unsigned short isize;
    unsigned short fsize;
    unsigned short nfree;
    unsigned int free[FREE_SIZE];
    unsigned short ninode;
    unsigned short inode[I_SIZE];
    char flock;
    char ilock;
    unsigned short fmod;
    unsigned short time[2];
} superblock_type;

superblock_type superBlock;

// I-Node Structure

typedef struct
{
    unsigned short flags;
    unsigned short nlinks;
    unsigned short uid;
    unsigned short gid;
    unsigned int size;
    unsigned int addr[ADDR_SIZE];
    unsigned short actime[2];
    unsigned short modtime[2];
} inode_type;

inode_type inode_root;
inode_type newinode;
inode_type curinode;

typedef struct
{
    short inode;
    unsigned char filename[14];
} dir_type;

dir_type root;
dir_type newdir;

int fileDescriptor; //file descriptor
const unsigned short inode_alloc_flag = 0100000;
const unsigned short dir_flag = 040000;
const unsigned short plain_flag = 000000;
const unsigned short dir_large_file = 010000;
const unsigned short dir_access_rights = 000777; // User, Group, & World have all access privileges
const unsigned short INODE_SIZE = 64;            // inode has been doubled

int initfs(char *path, unsigned short total_blcks, unsigned short total_inodes);
void add_block_to_free_list(int blocknumber);
void create_root();
int preInitialization();
int getInode();
int getDataBlock();
int cpin(char *filename, char *externalfile);
void cpout(char *pathname1, char *pathname2);
void ls(char *path);
int makeDir(char *filename);
int rm(char *path);
int cd(char *path);

int main()
{

    char input[INPUT_SIZE];
    char *splitter;
    unsigned int numBlocks = 0, numInodes = 0;
    char *filepath;
    printf("Size of super block = %d , size of i-node = %d\n", sizeof(superBlock), sizeof(inode_root));
    printf("Enter command:\n");

    while (1)
    {

        scanf(" %[^\n]s", input);
        splitter = strtok(input, " ");

        if (strcmp(splitter, "initfs") == 0)
        {

            preInitialization();
            splitter = NULL;
        }
        else if (strcmp(splitter, "cpin") == 0)
        {
            cpin(strtok(NULL, " "), strtok(NULL, " "));
            splitter = NULL;
        }
        else if (strcmp(splitter, "cpout") == 0)
        {
            cpout(strtok(NULL, " "), strtok(NULL, " "));
            splitter = NULL;
        }
        else if (strcmp(splitter, "mkdir") == 0)
        {
            makeDir(strtok(NULL, " "));
            splitter = NULL;
        }
        else if (strcmp(splitter, "ls") == 0)
        {
            ls(strtok(NULL, " "));
            splitter = NULL;
        }
        else if (strcmp(splitter, "cd") == 0)
        {
            cd(strtok(NULL, " "));
            splitter = NULL;
        }
        else if (strcmp(splitter, "rm") == 0)
        {
            rm(strtok(NULL, " "));
            splitter = NULL;
        }
        else if (strcmp(splitter, "q") == 0)
        {

            lseek(fileDescriptor, BLOCK_SIZE, 0);
            write(fileDescriptor, &superBlock, BLOCK_SIZE);
            return 0;
        }
        else
        {
            perror("Enter a valid command");
        }
    }
}

int preInitialization()
{

    char *n1, *n2;
    unsigned int numBlocks = 0, numInodes = 0;
    char *filepath;

    filepath = strtok(NULL, " ");
    n1 = strtok(NULL, " ");
    n2 = strtok(NULL, " ");

    if (access(filepath, F_OK) != -1)
    {
        fileDescriptor = open(filepath, O_RDWR, 0600);
        if (fileDescriptor == -1)
        {

            printf("\n filesystem already exists but open() failed with error [%s]\n", strerror(errno));
            return 1;
        }
        printf("filesystem already exists and the same will be used.\n");
        //retrieving superblock and rootinode
        lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
        read(fileDescriptor, &superBlock, BLOCK_SIZE);
        lseek(fileDescriptor, 2 * BLOCK_SIZE, SEEK_SET);
        read(fileDescriptor, &inode_root, INODE_SIZE);
        lseek(fileDescriptor, 2 * BLOCK_SIZE, SEEK_SET);
        read(fileDescriptor, &curinode, INODE_SIZE);
    }
    else
    {

        if (!n1 || !n2)
            printf(" All arguments(path, number of inodes and total number of blocks) have not been entered\n");

        else
        {
            numBlocks = atoi(n1);
            numInodes = atoi(n2);

            if (initfs(filepath, numBlocks, numInodes))
            {
                printf("The file system is initialized\n");
            }
            else
            {
                printf("Error initializing file system. Exiting... \n");
                return 1;
            }
        }
    }
}

int initfs(char *path, unsigned short blocks, unsigned short inodes)
{

    unsigned int buffer[BLOCK_SIZE / 4];
    int bytes_written;

    unsigned short i = 0;
    superBlock.fsize = blocks;
    unsigned short inodes_per_block = BLOCK_SIZE / INODE_SIZE;

    if ((inodes % inodes_per_block) == 0)
        superBlock.isize = inodes / inodes_per_block;
    else
        superBlock.isize = (inodes / inodes_per_block) + 1;

    if ((fileDescriptor = open(path, O_RDWR | O_CREAT, 0700)) == -1)
    {
        printf("\n open() failed with the following error [%s]\n", strerror(errno));
        return 0;
    }

    for (i = 0; i < FREE_SIZE; i++)
        superBlock.free[i] = 0; //initializing free array to 0 to remove junk data. free array will be stored with data block numbers shortly.

    superBlock.nfree = 0;
    superBlock.ninode = I_SIZE;

    for (i = 0; i < I_SIZE; i++)
        superBlock.inode[i] = i + 1; //Initializing the inode array to inode numbers

    superBlock.flock = 'a'; //flock,ilock and fmode are not used.
    superBlock.ilock = 'b';
    superBlock.fmod = 0;
    superBlock.time[0] = 0;
    superBlock.time[1] = 1970;

    lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
    write(fileDescriptor, &superBlock, BLOCK_SIZE); // writing superblock to file system

    // writing zeroes to all inodes in ilist
    for (i = 0; i < BLOCK_SIZE / 4; i++)
        buffer[i] = 0;

    for (i = 0; i < superBlock.isize; i++)
        write(fileDescriptor, buffer, BLOCK_SIZE);

    int data_blocks = blocks - 2 - superBlock.isize;
    int data_blocks_for_free_list = data_blocks - 1;

    // Create root directory
    create_root();

    for (i = 2 + superBlock.isize + 1; i < data_blocks_for_free_list; i++)
    {
        add_block_to_free_list(i);
    }

    return 1;
}

// Add Data blocks to free list
void add_block_to_free_list(int block_number)
{
    unsigned short free_list_data[BLOCK_SIZE / 4] = {0};
    if (superBlock.nfree == FREE_SIZE)
    {

        int i;
        free_list_data[0] = FREE_SIZE;

        for (i = 0; i < 256; i++)
        {
            if (i < FREE_SIZE)
            {
                free_list_data[i + 1] = superBlock.free[i];
            }
            else
            {
                break;
            }
        }

        superBlock.nfree = 0;
        lseek(fileDescriptor, (block_number)*BLOCK_SIZE, 0);
        write(fileDescriptor, free_list_data, BLOCK_SIZE); // Writing free list to header block
    }
    else
    {

        lseek(fileDescriptor, (block_number)*BLOCK_SIZE, 0);
        write(fileDescriptor, free_list_data, BLOCK_SIZE); // writing 0 to remaining data blocks to get rid of junk data
    }

    superBlock.free[superBlock.nfree] = block_number; // Assigning blocks to free array
    ++superBlock.nfree;
}

// Create root directory
void create_root()
{

    int root_data_block = 2 + superBlock.isize; // Allocating first data block to root directory
    int i;

    root.inode = 1; // root directory's inode number is 1.
    root.filename[0] = '.';
    root.filename[1] = '\0';

    inode_root.flags = inode_alloc_flag | dir_flag | dir_large_file | dir_access_rights; // flag for root directory
    inode_root.nlinks = 0;
    inode_root.uid = 0;
    inode_root.gid = 0;
    inode_root.size = INODE_SIZE;
    inode_root.addr[0] = root_data_block;

    for (i = 1; i < ADDR_SIZE; i++)
    {
        inode_root.addr[i] = 0;
    }

    inode_root.actime[0] = 0;
    inode_root.modtime[0] = 0;
    inode_root.modtime[1] = 0;

    lseek(fileDescriptor, 2 * BLOCK_SIZE, 0);
    write(fileDescriptor, &inode_root, INODE_SIZE); //
    lseek(fileDescriptor, 2 * BLOCK_SIZE, 0);
    read(fileDescriptor, &curinode, INODE_SIZE);

    lseek(fileDescriptor, root_data_block * BLOCK_SIZE, 0); //I changed this here for consistency(mul by block size)
    write(fileDescriptor, &root, 16);

    root.filename[0] = '.';
    root.filename[1] = '.';
    root.filename[2] = '\0';

    write(fileDescriptor, &root, 16);
}
/*
internal function to get a free inode from the inode array in the superblock
*/
int getInode()
{
    int inode_num;
    //if ninode is 0, then there are none avaliable
    if (superBlock.ninode == 0)
    {
        printf("There are no free inodes to be allocated\n");
        return -1;
    }
    else
    {
        //reduce the ninode and return the inode number
        inode_num = superBlock.inode[--superBlock.ninode];
        return inode_num;
    }
}
/*
intenal function used to get free data blocks
*/
int getDataBlock()
{
    int block_num;
    int i;
    unsigned short temparray[INPUT_SIZE];
    //if nfree equal to 1, then load the content of data block in free array of superblock and return that block
    if (superBlock.nfree == 1)
    {
        block_num = superBlock.free[--superBlock.nfree];
        lseek(fileDescriptor, block_num * BLOCK_SIZE, 0);
        read(fileDescriptor, &temparray, BLOCK_SIZE);
        superBlock.nfree = temparray[0];
        for (i = 0; i < 152; i++)
        {
            superBlock.free[i] = temparray[i + 1];
        }
    }
    else
    {
        block_num = superBlock.free[--superBlock.nfree];
    }
    return block_num;
}

void ls(char *path)
{
    //change the working directory
    cd(path);
    int i, j, flag;
    dir_type dir;
    //open each data block of addr register of the current inode
    for (i = 0; i < ADDR_SIZE; i++)
    {
        if (flag == 1)
        {
            return;
        }
        //each data block contains 64 data entries
        for (j = 0; j < 64; j++)
        {
            lseek(fileDescriptor, curinode.addr[i] * BLOCK_SIZE + j * 16, 0);
            if (read(fileDescriptor, &dir, 16) != 16)
            {
                printf("error in read\n");
            }
            //check until inode number == 0, that represents end of entries
            // printf("%d \n", dir.inode);
            if (dir.inode != 0 && dir.inode != -2)
            {
                printf("%s\n", dir.filename);
            }
            else if (dir.inode == 0)
            {
                flag = 1;
                break;
            }
        }
    }
}
int makeDir(char *path)
{
    char *filename;
    char pathholder[1000];
    const char s[2] = "/";
    char *splitter;
    strcpy(pathholder, path);
    splitter = strtok(path, s);
    int z = 0;
    //last one represent the filename
    while (splitter != NULL)
    {
        z++;
        filename = splitter;
        splitter = strtok(NULL, s);
    }
    //if the string length of path holder and filename are the same then there is no need to change the directory
    if (strlen(pathholder) != strlen(filename))
    {
        //we change the directory, we are removing the last /filename part
        pathholder[strlen(pathholder) - strlen(filename) - 1] = 0;
        if (cd(pathholder) == -1)
        {
            return -1;
        };
        strcpy(pathholder, "..");
    }
    //get a new inode for the directory entry
    int inodenum = getInode();
    int k;
    //update the flags of the inode
    newinode.flags = inode_alloc_flag | dir_flag | dir_access_rights;
    newinode.nlinks = 2;
    newinode.uid = '0';
    newinode.gid = '0';
    newinode.size = BLOCK_SIZE;
    //get a free data block
    int block_num = getDataBlock();
    for (k = 0; k < ADDR_SIZE; k++)
    {
        newinode.addr[k] = 0;
    }
    newinode.addr[0] = block_num;
    //update the entries in the data block of the directory inode
    lseek(fileDescriptor, newinode.addr[0] * BLOCK_SIZE, SEEK_SET);
    // . refers to itself
    newdir.inode = inodenum;
    newdir.filename[0] = '.';
    newdir.filename[1] = '\0';
    write(fileDescriptor, &newdir, 16);
    // .. refers to parent
    lseek(fileDescriptor, curinode.addr[0] * BLOCK_SIZE, SEEK_SET);
    dir_type dir;
    read(fileDescriptor, &dir, 16);
    newdir.inode = dir.inode;
    newdir.filename[0] = '.';
    newdir.filename[1] = '.';
    newdir.filename[2] = '\0';
    lseek(fileDescriptor, newinode.addr[0] * BLOCK_SIZE + 16, SEEK_SET);
    write(fileDescriptor, &newdir, 16);
    //update the inode and root
    newdir.inode = inodenum;
    strncpy(newdir.filename, filename, 14);
    lseek(fileDescriptor, (2) * BLOCK_SIZE + (inodenum - 1) * 64, SEEK_SET);
    write(fileDescriptor, &newinode, INODE_SIZE);
    int i, j, flag;
    for (i = 0; i < ADDR_SIZE; i++)
    {
        if (flag == 1)
        {
            printf("creation of directory is completed\n");
            while (z > 1)
            {
                cd(pathholder);
                z--;
            }
            return 1;
        }
        //each data block has 64 directory entries, cause BLOCK_SIZE/16=64
        for (j = 0; j < 64; j++)
        {
            //look for a directory entry where inode is equal to zero, so we can the newdir in its place
            lseek(fileDescriptor, curinode.addr[i] * BLOCK_SIZE + j * 16, 0);
            if (read(fileDescriptor, &dir, 16) != 16)
            {
                printf("error in read\n");
            }
            //check if the file already exits
            else if (strcmp(dir.filename, newdir.filename) == 0)
            {
                printf("file already exits\n");
                return -1;
            }
            //if the inode of the directory entry is 0, then we can add newdir in its place
            else if (dir.inode == 0 || dir.inode == -2)
            {
                lseek(fileDescriptor, curinode.addr[i] * BLOCK_SIZE + j * 16, 0);
                write(fileDescriptor, &newdir, sizeof(newdir));
                flag = 1;
                break;
            }
        }
    }
}
/*
changes the current working directory
*/
int cd(char *path)
{
    //return back to the root if path ==/
    if (path != NULL && strcmp(path, "/") == 0)
    {
        lseek(fileDescriptor, 2 * BLOCK_SIZE, SEEK_SET);
        read(fileDescriptor, &curinode, INODE_SIZE);
        printf("returned back to the root!!\n");
        return 1;
    }
    const char s[2] = "/";
    char *splitter;
    int i, j, flag, notfoundflag = 0;
    dir_type dir;
    splitter = strtok(path, s);
    while (splitter != NULL)
    {
        if (notfoundflag == 1)
        {
            break;
        }
        else
        {

            for (i = 0; i < ADDR_SIZE; i++)
            {
                if (flag == 1)
                {
                    flag = 0;
                    //if it is plain file we can not access it!!
                    if ((curinode.flags & dir_flag) == plain_flag)
                    {
                        lseek(fileDescriptor, 2 * BLOCK_SIZE, SEEK_SET);
                        read(fileDescriptor, &curinode, INODE_SIZE);
                        printf("can not change directory to a plain file, returning back to the root!!\n");
                    }
                    break;
                }
                if (i == ADDR_SIZE - 1 && flag == 0)
                {
                    printf("directory not found\n");
                    notfoundflag = 1;
                    break;
                }
                for (j = 0; j < 64; j++)
                {
                    lseek(fileDescriptor, curinode.addr[i] * BLOCK_SIZE + j * 16, 0);
                    read(fileDescriptor, &dir, 16);
                    if (strcmp(dir.filename, splitter) == 0)
                    {
                        lseek(fileDescriptor, (2) * BLOCK_SIZE + (dir.inode - 1) * 64, SEEK_SET);
                        read(fileDescriptor, &curinode, 64);
                        flag = 1;
                        break;
                    }
                    else if (dir.inode == 0)
                    {
                        break;
                    }
                }
            }
            splitter = strtok(NULL, s);
        }
    }
    if (notfoundflag == 1)
    {
        return -1;
    }
    else
    {
        return dir.inode;
    }
}
int getInodeByFileName(const char *filename)
{
    dir_type dir;
    int inodeNumber = -1;
    int i;

    lseek(fileDescriptor, (BLOCK_SIZE * curinode.addr[0]), SEEK_SET);

    for (i = 0; i < 64; i++)
    {
        read(fileDescriptor, &dir, 16);
        if (strcmp(filename, dir.filename) == 0)
        {
            if (dir.inode == 0)
            {
                printf("File %s not found\n", filename);
                return inodeNumber;
            }
            else
            {
                inodeNumber = dir.inode;
            }
        }
    }
    if (inodeNumber == -1)
    {
        printf("File %s not found\n", filename);
    }

    return inodeNumber;
}
int rm(char *path)
{
    //we first change the directory according to the path
    char *filename;
    char pathholder[1000];
    const char s[2] = "/";
    char *splitter;
    strcpy(pathholder, path);
    splitter = strtok(path, s);
    int z = 0;
    //last one represent the filename
    while (splitter != NULL)
    {
        z++;
        filename = splitter;
        splitter = strtok(NULL, s);
    }
    //if the string length of path holder and filename are the same then there is no need to change the directory
    if (strlen(pathholder) != strlen(filename))
    {
        //we change the directory, we are removing the last /filename part
        pathholder[strlen(pathholder) - strlen(filename) - 1] = 0;
        if (cd(pathholder) == -1)
        {
            return -1;
        };
        strcpy(pathholder, "..");
    }
    int inodeNum = getInodeByFileName(filename);
    inode_type fileInode, dirInode;
    dir_type dir;
    int i;
    int blockNum, freeBlockNum = 0;
    if (inodeNum == -1)
    {
        return -1;
    }

    lseek(fileDescriptor, (BLOCK_SIZE * 2 + INODE_SIZE * (inodeNum - 1)), SEEK_SET);
    read(fileDescriptor, &fileInode, INODE_SIZE);
    if (fileInode.flags & dir_flag)
    {
        printf("Can not delete a directory!!\n");
        return 0;
    }
    blockNum = (fileInode.size / BLOCK_SIZE) + 1;

    for (i = 0; i < ADDR_SIZE; i++)
    {
        freeBlockNum = fileInode.addr[i];
        add_block_to_free_list(freeBlockNum);
        fileInode.addr[i] = 0;
    }

    fileInode.flags = 0;
    superBlock.inode[superBlock.ninode++] = inodeNum;

    //Updating the inode
    lseek(fileDescriptor, (BLOCK_SIZE * 2 + INODE_SIZE * (inodeNum - 1)), SEEK_SET);
    write(fileDescriptor, &fileInode, INODE_SIZE);
    int records = ((BLOCK_SIZE - 2) / 16);
    lseek(fileDescriptor, (BLOCK_SIZE * curinode.addr[0]), SEEK_SET);

    for (i = 0; i < records; i++)
    {
        read(fileDescriptor, &dir, 16);
        if (dir.inode == inodeNum)
        {
            printf("Successfully removed the file\n");
            lseek(fileDescriptor, (-1) * sizeof(dir), SEEK_CUR);
            dir.inode = -2;
            memset(dir.filename, 0, sizeof(dir.filename));
            write(fileDescriptor, &dir, sizeof(dir));
            while (z > 1)
            {
                cd(pathholder);
                z--;
            }
            return 1;
        }
    }
    printf("Unsuccessful in removing the file");
    return -1;
}

/*
copies a file from the external os to the file system
*/
int cpin(char *path, char *externalfile)
{
    //we first change the directory according to the path
    char *filename;
    char pathholder[1000];
    const char s[2] = "/";
    char *splitter;
    strcpy(pathholder, path);
    splitter = strtok(path, s);
    int z = 0;
    //last one represent the filename
    while (splitter != NULL)
    {
        z++;
        filename = splitter;
        splitter = strtok(NULL, s);
    }
    //if the string length of path holder and filename are the same then there is no need to change the directory
    if (strlen(pathholder) != strlen(filename))
    {
        //we change the directory, we are removing the last /filename part
        pathholder[strlen(pathholder) - strlen(filename) - 1] = 0;
        if (cd(pathholder) == -1)
        {
            return -1;
        };
        strcpy(pathholder, "..");
    }
    //to create a new file we need a new directory entry
    dir_type newdir;
    //some temp variables
    int i, j;
    int flag = 0;
    int fdexternal, blocks_req;
    struct stat stats;
    char buf[BLOCK_SIZE];
    //then we need to get a free inode from the getInode function
    int inode = getInode();
    //set the new directory entry inode to the obtained inode and the directory entry filename to the filename entered by the user
    newdir.inode = inode;
    strncpy(newdir.filename, filename, 14);
    //now check if the external file is exist
    fdexternal = open(externalfile, O_RDONLY, 0700);

    if (strlen(filename) > 14)
    {
        printf("please keep the filename to 14 char!!\n");
        return -1;
    }

    //if there is an error
    if (fdexternal == -1)
    {
        printf("\n open() failed with the following error [%s]\n", strerror(errno));
        return -1;
    }
    //if the file exists
    else
    {
        //stats can help us in determining how to many block should be allocated to new file
        stat(externalfile, &stats);
        //check for the req blocks
        blocks_req = (stats.st_size / BLOCK_SIZE) + 1; //we do not want to leave the decimal part thus we are adding 1
        //since we are assuming its a small file, thus we can get the data blocks from the getDatablocks function
        //now we get the required data blocks from the getDataBlock function and then add it to the inode of the directory entry
        for (i = 0; i < blocks_req; i++)
        {
            //newinode is the place holder for the inode of the new directory entry
            newinode.addr[i] = getDataBlock();
        }
        //load each block from one external file and copy it to the newly created file
        for (i = 0; i < blocks_req; i++)
        {
            lseek(fdexternal, i * BLOCK_SIZE, 0);
            read(fdexternal, &buf, BLOCK_SIZE);
            lseek(fileDescriptor, BLOCK_SIZE * (newinode.addr[i]), 0);
            write(fileDescriptor, &buf, BLOCK_SIZE);
        }
    }
    //change the flags for the inode
    newinode.flags = inode_alloc_flag | 000000 | 000000 | dir_access_rights; //since it's plain file, and small file
    newinode.nlinks = 1;
    newinode.uid = '0';
    newinode.gid = '0';
    newinode.size = stats.st_size;
    //update the inode
    lseek(fileDescriptor, (2) * BLOCK_SIZE + (inode - 1) * 64, SEEK_SET);
    write(fileDescriptor, &newinode, INODE_SIZE);
    //update the root
    dir_type dir;
    for (i = 0; i < ADDR_SIZE; i++)
    {
        if (flag == 1)
        {
            printf("copy complete!!\n");
            printf("updation of root and inode complete\n");
            while (z > 1)
            {
                cd(pathholder);
                z--;
            }
            break;
        }
        //each data block has 64 directory entries, cause BLOCK_SIZE/16=64
        for (j = 0; j < 64; j++)
        {
            //look for a directory entry where inode is equal to zero, so we can the newdir in its place
            lseek(fileDescriptor, curinode.addr[i] * BLOCK_SIZE + j * 16, 0);
            if (read(fileDescriptor, &dir, 16) != 16)
            {
                printf("error in read\n");
            }
            //check if the file already exits
            if (strcmp(dir.filename, newdir.filename) == 0)
            {
                printf("file already exists");
                return -1;
            }
            //if the inode of the directory entry is 0, then we can add newdir in its place
            else if (dir.inode == 0 || dir.inode == -2)
            {
                lseek(fileDescriptor, curinode.addr[i] * BLOCK_SIZE + j * 16, 0);
                write(fileDescriptor, &newdir, sizeof(newdir));
                flag = 1;
                break;
            }
        }
    }
    return 1;
}
/*
copies a file from the v6 file system to an external file
*/
void cpout(char *out, char *path)
{
    //we first change the directory according to the path
    char *filename;
    char pathholder[1000];
    const char s[2] = "/";
    char *splitter;
    strcpy(pathholder, path);
    splitter = strtok(path, s);
    //last one represent the filename
    int z = 0;
    while (splitter != NULL)
    {
        filename = splitter;
        splitter = strtok(NULL, s);
        z++;
    }
    //if the string length of path holder and filename are the same then there is no need to change the directory
    if (strlen(pathholder) != strlen(filename))
    {
        pathholder[strlen(pathholder) - strlen(filename) - 1] = 0;
        if (cd(pathholder) == -1)
        {
            return;
        };
        strcpy(pathholder, "..");
    }
    int fd1, i, j;
    int flag = 0;
    unsigned short buffer[BLOCK_SIZE];
    //find the inode of the file in the root
    //iterate over all the data blocks in the addr
    dir_type dir;
    for (i = 0; i < ADDR_SIZE; i++)
    {
        //if found break the loop
        if (flag == 1)
        {
            while (z > 1)
            {
                cd(pathholder);
                z--;
            }
            break;
        }
        else if (i == ADDR_SIZE - 1 && flag == 0)
        {
            printf("file not found\n");
            return;
        }
        //each data block has 64 directory entries, cause each directory entry is of size 16, so BLOCK_SIZE/16=64
        for (j = 0; j < 64; j++)
        {
            lseek(fileDescriptor, curinode.addr[i] * BLOCK_SIZE + j * 16, 0);
            if (read(fileDescriptor, &dir, 16) != 16)
            {
                printf("error in read\n");
            }
            //check if the directory name matches
            if (strcmp(dir.filename, filename) == 0)
            {
                flag = 1;
                break;
            }
        }
    }
    //get the inode from the memory
    lseek(fileDescriptor, 2 * BLOCK_SIZE + (dir.inode - 1) * 64, 0);
    read(fileDescriptor, &newinode, 64);
    //now create a new file
    fd1 = creat(out, 0775);
    fd1 = open(out, O_RDWR);
    for (i = 0; i < (newinode.size / BLOCK_SIZE) + 1; i++)
    {
        lseek(fileDescriptor, newinode.addr[i] * BLOCK_SIZE, SEEK_SET);
        read(fileDescriptor, &buffer, newinode.size);
        lseek(fd1, BLOCK_SIZE * i, SEEK_SET);
        write(fd1, &buffer, newinode.size);
    }
    printf("cpout complete\n");
    return;
}

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#define INT_AS_STRING 6
#define FILE_NAME_LEN 16
#define CHAR_WRITE_SPACE 2
#define FAT_OFFSET 1560 // Fat location in logical file 16 + 6 + 2 + 256*6(1536) = 1560 

typedef struct openFiles{
    int FATS[64];
    int logicals[64];
    int offsets[64];
    int numOpen;
}openFiles;

typedef struct superBlock{
    char driveName[FILE_NAME_LEN];
    int firstLogical;
    int firstFAT;
    int FATEntryPerBlock;
    int firstBlub;
    int totalFiles;
    char freefiles[256];
//    char freeFAT[8192];
}superBlock;

typedef struct logicalFile{
    char fileName[FILE_NAME_LEN];
    int fileSize;
    char descriptor; //Used as a boolean.  I think this is the smallest addressable memory without doing bit shift math
//    time_t created;
//    time_t modified;
    int filePointers[256];
    int FATblock;
}logicalFile;

typedef struct FATentry{
    int blockNumber;
    char descriptor;    
}FATentry;


int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int fs_open(char *name);
int fs_close(int fildes);
int fs_create(char *name);
int fs_delete(char *name);
int fs_mkdir(char *name);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);
int find_free_file();
void getSuper();
void getLogical(int block);
void getRootLogical();
int write_super();
int write_logical_file(int block, char* fileName, int fileSize, char isDirectory);
logicalFile* create_logical_file(char* fileName, int fileSize, char isDirectory);
int createFAT();
int umount_fs(char *disk_name);
int overwriteLogical(int block, logicalFile *f);

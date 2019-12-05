#include "disk.h"
#include "filesystem.h"
#include "disk.c"
#include <stdlib.h>

//extern int handle;

superBlock super;
logicalFile currentRoot;
logicalFile currentDir;
logicalFile openFile;
int openFileOffset;

void main (){
    make_fs("Dummy");
}

/*
make_fs makes a real file, opens that real file, writes the 'file system' information
to the real file including:
super block (at block 0)
root (at block 1)
FAT (at block 257)
*/
int make_fs(char *disk_name) {

    if (make_disk(disk_name)) {
        fprintf(stderr, "Failed to create disk %s", disk_name);
        return -1;        
    }

    if (open_disk(disk_name)) {
        fprintf(stderr, "Failed to open disk %s", disk_name);
        return -1;
    }
    
    if (write_super(disk_name)) {
        fprintf(stderr, "Failed to write Superblock");
        //return -1;
    }

    if (write_logical_file(1, "root", sizeof(logicalFile), '1')) {
        fprintf(stderr, "Failed to write root directory");
        return -1;
    }

    createFAT();

    close_disk(disk_name);

    return 0;
}

/*
mount_fs opens a real file and loads the 'file system' information from it
this frunction is the virualization of the real file into a drive
after opening it grabs the super block and then the root
super block has all the inforation to other important blocks
*/
int mount_fs(char *disk_name) {
    
    open_disk(disk_name);
    getSuper();
    getLogical(1);

    return 0;
}

/*
Because of my very probable misuse of mmap I don't actually need to do anything to
unmount other than close.  The many attempts I used at writing to the disk using the 
function provided in disk.c all ended with lots of junk data being written.
I tried ways of manipulating strings, I tried flushing buffers, I could find no way to
ever get it to write a block without including junk.
*/
int umount_fs(char *disk_name) {
    close(disk_name);
}

/*
fs_open returns the block of the logical file that is searched for.  It also updates a global structure
so the various metadata, including the first frame on the FAT, of the currently opened file can be quickly accessed.
I don't like using globals, but with all the functions already having stubs I was having trouble thinking of a way to get
the data I thought useful
*/
int fs_open(char *name) {
    int fsd = -1;
    for (int i = 0; i < 256; i++) {
        if (currentDir.filePointers[i] == -1) {
            break;
        }
        else {
            char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*currentDir.filePointers[i]);
            if (strncmp(name, mapping, sizeof(name))) {
                mapping+=FILE_NAME_LEN;
                if (mapping[0] == 'v') {
                    snprintf(mapping, 2, "%c", 'o');
                    mapping += FAToffset;
                    fsd = currentDir.filePointers[i];
                }
            }
            munmap(mapping, BLOCK_SIZE*currentDir.filePointers[i]);
        }   
    }
    return fsd;
}

/*
fs_close basically justs sets the indicator / descriptor in the logical file from opened to closed
along with a few trvial error checking things.
It also resets the offset that lseek may have changed
*/
int fs_close(int fildes) {
    if (fildes > 256 || fildes < 2) {
        return -1;
    }
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*fildes);
    mapping+=FILE_NAME_LEN;
    if (mapping[0] != 'o') {
        return -1;
    }
    else {
        snprintf(mapping, 2, "%c", 'v');
        return 0;
    }
    openFileOffset = 0;
}

/*
fs_create creates the logical file for a new file on disk
*/
int fs_create(char *name) {
    int block = find_free_file();
    if (block == -1) {
        return -1;
    }
    else {
        write_logical_file(block, name, 0, 'e');
    }
    return 0;
}

/*
fs_delete does two main things, sets the descriptor of the logical file to deleted
and also 0's out whatever was in the blocks pointed to be the FAT entries for the file
*/
int fs_delete(char *name) {
    int deleted = -1;
    for (int i = 0; i < 256; i++) {
        if (currentDir.filePointers[i] == -1) {
            break;
        }
        else {
            char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*currentDir.filePointers[i]);
            if (strncmp(name, mapping, sizeof(name))) {
                mapping+=FILE_NAME_LEN;
                if (mapping[0] == 'v') {
                    snprintf(mapping, 2, "%c", '0');
                    deleted = 1;
                }
            }
            munmap(mapping, BLOCK_SIZE*currentDir.filePointers[i]);
        }   
    }
    return deleted;
}

/*
fs_mkdir is the same exact thing as a file except with the directory flag.  I'm sure this isn't a great way to
do this but I had a lot of space and not a lot of good ideas so this ended up mostly brute forced
*/
int fs_mkdir(char *name) {

    int block = find_free_file();
    if (block == -1) {
        return -1;
    }
    else {
        write_logical_file(block, name, sizeof(logicalFile), '1');
    }
    return 0;
}

/*
fs_read makes a mapping of the block pointed to by the FAT
offsets the pointer of that mapping
reads in n characters from that offset 
*/
int fs_read(int fildes, void *buf, size_t nbyte) {
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*openFile.FATblock);
    mapping += openFileOffset;
    memcpy(buf, mapping, nbyte);
    munmap(mapping, BLOCK_SIZE*openFile.FATblock);
}

/*
fs_write makes a mapping of the block pointed to be the FAT
offsets the pointer of that mapping
writes n characters from that offset
*/
int fs_write(int fildes, void *buf, size_t nbyte) {
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*openFile.FATblock);
    mapping += openFileOffset;
    memcpy(mapping, buf, nbyte);
    munmap(mapping, BLOCK_SIZE*openFile.FATblock);
}

/*
fs_get_filesize just takes the metadata right from the global
*/
int fs_get_filesize(int fildes) {
    printf("\n\n The size of file %s is %ld", openFile.fileName, openFile.fileSize);
}

/*
fs_lseek sets a global offset variable based on the offset provided
*/
int fs_lseek(int fildes, off_t offset) {
    if (fildes < 1 || fildes > 256 || offset < 0 || offset > openFile.fileSize) {
        return -1;
    }
    else {
        openFileOffset = offset;
        return 0;
    }
}

/*
fs_truncate figures out what block pointed to by the FAT you want to partially overwrite.
After that it 0's out the block from the specified point onwards.
*/
int fs_truncate(int fildes, off_t length) {
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*openFile.FATblock);
    void * buf = '0';
    for (int i = length; i < BLOCK_SIZE; i++) {
        memcpy(mapping, buf, 1);
    }
    munmap(mapping, BLOCK_SIZE*openFile.FATblock);
}

/*
find_free_file searches the superblock for an unused logical file location.
superblock holds a little array.  Files are allocated in order.
*/
int find_free_file() {
    for (int i = 0; i < 256; i++) {
        if (super.freefiles[i] == 1) {
            return i + 1;
        }
    }
    return -1;
}

/*
getSuper initializes the global super block from block 0 on the real file / fake disk
this is called as part of the mounting process
*/
void getSuper() {

    //superBlock super;
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
    
    for (int i = 0; i < 10; i++) {
        super.driveName[i] = mapping++;
    }
    super.firstLogical = atoi(mapping);
    mapping+=INT_AS_STRING;
    super.firstFAT = atoi(mapping);
    mapping+=INT_AS_STRING;
    super.FATEntryPerBlock = atoi(mapping);
    mapping+=INT_AS_STRING;
    super.firstBlub = atoi(mapping);
    
    munmap(mapping, BLOCK_SIZE);
}

/*
getLogical retrieves the logical file at the block specified
because there are only 256 files possible each file is assigned it's on block which is a huge waste of space
This also means this function should never recieve a number x where 0 > x || x > 256 
*/
void getLogical(int block) {
    
    logicalFile logicFile;
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*block);
    
    for (int i = 0; i < 10; i++) {
        logicFile.fileName[i] = mapping++;
    }

    logicFile.fileSize = atoi(mapping);
    mapping+=INT_AS_STRING;
    logicFile.descriptor = mapping[0];
    mapping+=2;
    for (int i = 0; i < 256; i++) {
        logicFile.filePointers[i] = atoi(mapping);
        mapping+=INT_AS_STRING;
    }    
    logicFile.FATblock = atoi(mapping);
    
    munmap(mapping, BLOCK_SIZE);
}

/*
write_super just prints the superblock info to block 0 of the disk file
*/
int write_super(char*disk_name) {

    char str[100];
    superBlock super = {""};
    strcpy(super.driveName, disk_name);
    super.firstLogical = 1;
    super.firstFAT = 257;
    super.FATEntryPerBlock = 128;
    super.firstBlub = 8192;

    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
    
    snprintf(mapping, sizeof(super.driveName), "%s", super.driveName);
    mapping+=sizeof(super.driveName);
    snprintf(mapping, INT_AS_STRING, "%d", super.firstLogical);
    mapping+=INT_AS_STRING;
    snprintf(mapping, INT_AS_STRING, "%d", super.firstFAT);
    mapping+=INT_AS_STRING;
    snprintf(mapping, INT_AS_STRING, "%d", super.FATEntryPerBlock);
    mapping+=INT_AS_STRING;
    snprintf(mapping, INT_AS_STRING, "%d", super.firstBlub);
    mapping+=INT_AS_STRING;
    /*
    memcpy(mapping, &super.driveName, sizeof(super.driveName));
    mapping+=sizeof(super.driveName);
    memcpy(mapping, itoa(super.firstLogical), sizeof(super.firstLogical));
    mapping+=sizeof(super.firstLogical);
    memcpy(mapping, itoa(super.firstFAT), sizeof(super.firstFAT));
    mapping+=sizeof(super.firstFAT);
    memcpy(mapping, itoa(super.FATEntryPerBlock), sizeof(super.FATEntryPerBlock));
    mapping+=sizeof(super.FATEntryPerBlock);
    memcpy(mapping, itoa(super.firstBlub), sizeof(super.firstBlub));
    mapping+=sizeof(super.firstBlub);
    */

    munmap(mapping,BLOCK_SIZE);  
    return 0;  
}

/*
write_logical_file writes a logical file to a specified block on the disk
*/
int write_logical_file(int block, char* fileName, int fileSize, char descriptor) {
    logicalFile* f = create_logical_file(fileName, fileSize, descriptor);
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*block);

    snprintf(mapping, sizeof(f->fileName), "%s", f->fileName);
    mapping+=sizeof(f->fileName);
    snprintf(mapping, INT_AS_STRING, "%d", f->fileSize);
    mapping+=INT_AS_STRING;
    snprintf(mapping, 2, "%c", f->descriptor);
    mapping+=2;
    for (int i = 0; i < 256; i++) {
        snprintf(mapping, INT_AS_STRING, "%d", f->filePointers[i]);
        mapping+=INT_AS_STRING;
    }
    snprintf(mapping, INT_AS_STRING, "%d", f->FATblock);
    mapping+=INT_AS_STRING;
/*
    unsigned char *buffer=(char*)malloc(sizeof(logicalFile));
    memcpy(buffer,(const unsigned char*)f,sizeof(logicalFile));

    printf("\n--  %s | %ld | sizeof f %ld --\n", buffer, sizeof(buffer), sizeof(logicalFile));
    printf("\n--  %s --\n", f->fileName);

//    printf("\n--  %ld --\n", f->created);

    printf("\n--  %d --\n", f->fileSize);

//    printf("\n--  %ld --\n", f->modified);

    printf("\n--  %c --\n", f->isDirectory);
*/
 //   block_write(1, f);
    //block_write(block, buffer);

    munmap(mapping, BLOCK_SIZE * block);
    return 0;
}

/*
create_logical__file creates a logical file which is immediately put onto the disk.  I don't 
think the program in it's current form really uses this.  It's more of a relic from a previous attempt
at a solution.
That isn't to say this function isn't called, just that it might not be providing any real benefit
*/
logicalFile* create_logical_file(char* fileName, int fileSize, char descriptor) {
    
    logicalFile *f = (logicalFile*)malloc(sizeof(logicalFile));
    if (f)
    {
        strncpy(f->fileName, fileName, sizeof(f->fileName)-1);
        f->fileName[10] = '\0';

        f->fileSize = fileSize;

        f->descriptor = descriptor;

//        f->created = (time_t) time;

//        f->modified = (time_t) time;

        f->FATblock = -1;

        for (int i = 0; i < 256; i++) {
            f->filePointers[i] = -1;
        }
    }
    return f;
};

/*
createFAT creates the entire FAT allocation table and writes it to the disk
each block takes 128 FAT entires, 8192 entries total, meaning it uses 64 blocks of disk
*/
int createFAT() {

    FATentry entry;
    entry.blockNumber = -1;
    entry.descriptor = 'f';
    for (int i = 257; i < 321; i++) {

        char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*i);

        for (int j = 0; j < 128; j++) {
            
            snprintf(mapping, INT_AS_STRING, "%d", entry.blockNumber);
            mapping+=INT_AS_STRING;
            snprintf(mapping, 2, "%c", entry.descriptor);
            mapping+=2;
        }

        munmap (mapping, BLOCK_SIZE * i);
    }
    return 0;
}
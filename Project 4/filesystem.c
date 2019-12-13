#include "disk.h"
#include "filesystem.h"
#include "disk.c"
#include <stdlib.h>

//extern int handle;

superBlock super;
logicalFile currentRoot;
logicalFile currentDir;
logicalFile openFile;

openFiles opener;
char *freefatmap;
/*
void main (){
    make_fs("Dummy");
    mount_fs("Dummy");
}*/

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

    strcpy(super.driveName, disk_name);
    super.firstLogical = 1;
    super.firstFAT = 257;
    super.FATEntryPerBlock = 128;
    super.firstBlub = 8192;
    super.totalFiles = 1;
    super.freefiles[0] = 'i';
    for (int i = 1; i < 256; i++) {
        super.freefiles[i] = '1';
    }

    if (write_super(disk_name)) {
        fprintf(stderr, "Failed to write Superblock");
        //return -1;
    }

    createFAT();
    freefatmap = mmap(0, BLOCK_SIZE*4, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*400);
    for (int i = 0; i < 8192; i++) {
        snprintf(freefatmap, CHAR_WRITE_SPACE, "%c", '1');
        freefatmap+=CHAR_WRITE_SPACE;
    }

    if (write_logical_file(1, "root", sizeof(logicalFile), '1')) {
        fprintf(stderr, "Failed to write root directory");
        return -1;
    }

    munmap (freefatmap, BLOCK_SIZE);

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
    getRootLogical(); 
    currentDir = currentRoot;
    
    for (int i = 0; i < 64; i++) {
        opener.FATS[i] = 0;
        opener.offsets[i] = 0;
        opener.logicals[i] = 0;
    }
    opener.numOpen = 0;
    freefatmap = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*400);

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
    close_disk();
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
            //logic error do nothing
        }
        else {
            printf("====ELSE====");
            getLogical(i);
            printf("%s %s %d %d", openFile.fileName, name, strncmp(openFile.fileName, name, sizeof(name)), openFile.FATblock);
            int compared = strncmp(openFile.fileName, name, sizeof(name));
            if (compared == 0) {

                if (openFile.descriptor == 'e') {
                    openFile.descriptor = 'o';
                    opener.logicals[opener.numOpen] = i;
                    opener.FATS[opener.numOpen] = openFile.FATblock;
                    opener.numOpen++;
                    fsd = i;
                }
            }
            /*char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*currentDir.filePointers[i]);
            if (strncmp(name, mapping, sizeof(name))) {
                mapping+=FILE_NAME_LEN;
                mapping+=INT_AS_STRING;
                if (mapping[0] == 'v') {
                    snprintf(mapping, CHAR_WRITE_SPACE, "%c", 'o');
                    mapping += FAT_OFFSET;
                    opener.logicals[opener.numOpen] = currentDir.filePointers[i];
                    opener.FATS[opener.numOpen] = atoi(mapping);
                    opener.numOpen++;
                    fsd = currentDir.filePointers[i];
                }
            }
            munmap(mapping, BLOCK_SIZE*currentDir.filePointers[i]);*/
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
    mapping+=FILE_NAME_LEN; //filename offset
    mapping+=INT_AS_STRING; // filesixze offset
    if (mapping[0] != 'o') {
        return -1;
    }
    else {
        snprintf(mapping, CHAR_WRITE_SPACE, "%c", 'v');
        return 0;
    }
    for (int i = 0; i < 64; i++) {
        if (opener.logicals[i] == fildes) {
            opener.FATS[i] = 0;
            opener.logicals[i] = 0;
            opener.offsets [i] = 0;
            opener.numOpen--;
        }
    }
}

/*
fs_create creates the logical file for a new file on disk
*/
int fs_create(char *name) {

    if (super.totalFiles == 256) {
        fprintf(stderr, "File limit reached");
        return -1;
    }

    int block = find_free_file();
    if (block == -1) {
        return -1;
        fprintf(stderr, "no free logical file space");
    }
    else {
        fprintf(stderr, "***** %d %s *****", block, name);
        write_logical_file(block, name, 0, 'e');
        super.totalFiles++;
        super.freefiles[block-1] = 'i';
        currentDir.filePointers[block-1] = 1;
        currentRoot.filePointers[block-1] = 1;
        overwriteLogical(1,&currentRoot);
        write_super();
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
                mapping+=INT_AS_STRING;
                if (mapping[0] == 'v') {
                    snprintf(mapping, CHAR_WRITE_SPACE, "%c", '0');
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
    getLogical(fildes);
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*openFile.FATblock);
    for (int i = 0; i < 64; i++) {
        if (opener.logicals[i] == fildes) {
            mapping+=opener.offsets[i];
        }
    }
    memcpy(buf, mapping, nbyte);
    munmap(mapping, BLOCK_SIZE);
}

/*
fs_write makes a mapping of the block pointed to be the FAT
offsets the pointer of that mapping
writes n characters from that offset
*/
int fs_write(int fildes, void *buf, size_t nbyte) {
    getLogical(fildes);
    write_logical_file(fildes, openFile.fileName, openFile.fileSize = nbyte, 'o');
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*openFile.FATblock);
    for (int i = 0; i < 64; i++) {
        if (opener.logicals[i] == fildes) {
            mapping+=opener.offsets[i];
            memcpy(mapping, buf, nbyte);
            opener.offsets[i] += nbyte;
        }
    }
    munmap(mapping, BLOCK_SIZE);
}

/*
fs_get_filesize just takes the metadata right from the global
*/
int fs_get_filesize(int fildes) {
    getLogical(fildes);
    printf("\n\n The size of file %s is %d", openFile.fileName, openFile.fileSize);
}

/*
fs_lseek sets a global offset variable based on the offset provided
*/
int fs_lseek(int fildes, off_t offset) {
    if (fildes < 1 || fildes > 256 || offset < 0 || offset > openFile.fileSize) {
        return -1;
    }
    else {
        for (int i = 0; i < 64; i++) {
            if (opener.logicals[i] == fildes) {
                opener.offsets[i] +=offset;
            }
        }
        return 0;
    }
}

/*
fs_truncate figures out what block pointed to by the FAT you want to partially overwrite.
After that it 0's out the block from the specified point onwards.
*/
int fs_truncate(int fildes, off_t length) {
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*openFile.FATblock);
    void * buf = (void *) '0';
    for (int i = length; i < BLOCK_SIZE; i++) {
        memcpy(mapping, buf, 1);
    }
    munmap(mapping, BLOCK_SIZE);
}

/*
find_free_file searches the superblock for an unused logical file location.
superblock holds a little array.  Files are allocated in order.
*/
int find_free_file() {

    for (int i = 1; i < 256; i++) {
        fprintf(stderr, "$$$ %c $$$", super.freefiles[i]);
        if (super.freefiles[i] == '1') {
            return (i+1);
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
    
    for (int i = 0; i < FILE_NAME_LEN; i++) {
        super.driveName[i] = mapping[i];
    }
    mapping+=FILE_NAME_LEN;
    super.firstLogical = atoi(mapping);
    mapping+=INT_AS_STRING;
    super.firstFAT = atoi(mapping);
    mapping+=INT_AS_STRING;
    super.FATEntryPerBlock = atoi(mapping);
    mapping+=INT_AS_STRING;
    super.firstBlub = atoi(mapping);
    mapping+=INT_AS_STRING;
    super.totalFiles = atoi(mapping);
    mapping+=INT_AS_STRING;

    for (int i = 0; i < 256; i++) {
        super.freefiles[i] = mapping[i*CHAR_WRITE_SPACE];
    }
/*    for (int i = 0; i < 8192; i++) {
        super.freeFAT[i] = mapping;
        mapping += CHAR_WRITE_SPACE;
    }*/
    munmap(mapping, BLOCK_SIZE);
}

/*
getLogical retrieves the logical file at the block specified
because there are only 256 files possible each file is assigned it's on block which is a huge waste of space
This also means this function should never recieve a number x where 0 > x || x > 256 
*/
void getLogical(int block) {
    
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*block);

//    for (int i = 0; i < BLOCK_SIZE; i++){
//        fprintf(stderr, "%c", mapping[i]);}

    for (int i = 0; i < FILE_NAME_LEN; i++) {
        openFile.fileName[i] = mapping[i];
    }
    mapping+=FILE_NAME_LEN;

    openFile.fileSize = atoi(mapping);
    mapping+=INT_AS_STRING;
    openFile.descriptor = mapping[0];
    mapping+=CHAR_WRITE_SPACE;
    for (int i = 0; i < 256; i++) {
        openFile.filePointers[i] = atoi(mapping);
        mapping+=INT_AS_STRING;
    }    
    openFile.FATblock = atoi(mapping);
    
    munmap(mapping, BLOCK_SIZE);
}

void getRootLogical() {
    
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE);

//    for (int i = 0; i < BLOCK_SIZE; i++){
//        fprintf(stderr, "%c", mapping[i]);}

    for (int i = 0; i < FILE_NAME_LEN; i++) {
        currentRoot.fileName[i] = mapping[i];
    }
    mapping+=FILE_NAME_LEN;

    currentRoot.fileSize = atoi(mapping);
    mapping+=INT_AS_STRING;
    currentRoot.descriptor = mapping[0];
    mapping+=CHAR_WRITE_SPACE;
    for (int i = 0; i < 256; i++) {
        currentRoot.filePointers[i] = atoi(mapping);
        mapping+=INT_AS_STRING;
    }    
    currentRoot.FATblock = atoi(mapping);
    
    munmap(mapping, BLOCK_SIZE);
}


/*
write_super just prints the superblock info to block 0 of the disk file
*/
int write_super() {

    //char str[100];

/*    for (int i = 0; i < 8192; i++) {
        super.freeFAT[i] = '1';
    }*/
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);

    snprintf(mapping, FILE_NAME_LEN, "%s", super.driveName);
    mapping+=FILE_NAME_LEN;
    snprintf(mapping, INT_AS_STRING, "%d", super.firstLogical);
    mapping+=INT_AS_STRING;
    snprintf(mapping, INT_AS_STRING, "%d", super.firstFAT);
    mapping+=INT_AS_STRING;
    snprintf(mapping, INT_AS_STRING, "%d", super.FATEntryPerBlock);
    mapping+=INT_AS_STRING;
    snprintf(mapping, INT_AS_STRING, "%d", super.firstBlub);
    mapping+=INT_AS_STRING;
    snprintf(mapping, INT_AS_STRING, "%d", super.totalFiles);
    mapping+=INT_AS_STRING;

    for (int i = 0; i < 256; i++) {
        snprintf(mapping, CHAR_WRITE_SPACE, "%c", super.freefiles[i]);
        mapping+=CHAR_WRITE_SPACE;
    }

/*
    for (int i = 0; i < 8192; i++) {
        snprintf(mapping, CHAR_WRITE_SPACE, "%c", super.freeFAT[i]);
        mapping+=CHAR_WRITE_SPACE;
    }
*/
    
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
    snprintf(mapping, CHAR_WRITE_SPACE, "%c", f->descriptor);
    mapping+=CHAR_WRITE_SPACE;
    for (int i = 0; i < 256; i++) {
        snprintf(mapping, INT_AS_STRING, "%d", f->filePointers[i]);
        mapping+=INT_AS_STRING;
    }
    for (int i = 0; i < 8192; i++) {
        if (freefatmap[i*CHAR_WRITE_SPACE] == '1') {
            f->FATblock = i+8192;
            snprintf(&freefatmap[i*CHAR_WRITE_SPACE], CHAR_WRITE_SPACE, "%c", '0');
            break;
        }
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
            snprintf(mapping, CHAR_WRITE_SPACE, "%c", entry.descriptor);
            mapping+=CHAR_WRITE_SPACE;
        }

        munmap (mapping, BLOCK_SIZE * i);
    }
    return 0;
}

void printFS() {

    printf("\n~~~~~~~~~~\n");
    printf("%s\n", currentRoot.fileName);
    for (int i = 0; i < 256; i++) {
        if (currentRoot.filePointers[i] != -1) {
            getLogical(i+1);
            printf("-%s\n", openFile.fileName);
        }
    }
}

void printMenu() {
    printf("\n==============\n");
    printf("1   make_fs(char *disk_name)\n");
    printf("2   int mount_fs(char *disk_name)\n");
    printf("3   int fs_open(char *name)\n");
    printf("4   int fs_close(int fildes)\n");
    printf("5   int fs_create(char *name)\n");
    printf("6   int fs_delete(char *name);\n");
    printf("7   int fs_mkdir(char *name);\n");
    printf("8   int fs_read(int fildes, void *buf, size_t nbyte)\n");
    printf("9   int fs_write(int fildes, void *buf, size_t nbyte)\n");
    printf("10  int fs_get_filesize(int fildes)\n");
    printf("11  int fs_lseek(int fildes, off_t offset)\n");
    printf("12  int fs_truncate(int fildes, off_t length)\n");
    printf("13  exit\n\n\n");  
}

void main() {

    int desc;
    superBlock super = {""};
    int buffsize = 50;
    void * buf;
    char userinput[buffsize];
    int usernum = 0;
    
    while (1) {
        printMenu();
    scanf("%d", &usernum);

    switch(usernum) {
        case 1: make_fs("TESTDRIVEa");
        break;
        case 2: mount_fs("TESTDRIVEa");
        printFS();
        break;
        case 3: desc = fs_open("test");
        printf("\n''%d''\n", desc);
        break;
        case 4: fs_close(desc);
        break;
        case 5: fs_create("test");
        fs_create("test1");
        fs_create("test2");
        printFS();
        break;
        case 6: fs_delete("file1");
        break;
        case 7: fs_mkdir("dir1");
        printFS();
        break;
        case 8: fs_read(desc, buf, 20);
        fprintf(stderr, "_%s_", buf);
        break;
        case 9: fs_write(desc, "I am testing this.", 10);
        fs_write(desc, " additional", 11);
        break;
        case 10: fs_get_filesize(desc);
        break;
        case 11: fs_lseek(desc, 2);
        break;
        case 12: fs_truncate(desc, 3);
        break;
        case 13: exit(0);
    }
    }


}


int overwriteLogical(int block, logicalFile *f) {
    
    char* mapping = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, BLOCK_SIZE*block);

    snprintf(mapping, sizeof(f->fileName), "%s", f->fileName);
    mapping+=sizeof(f->fileName);
    snprintf(mapping, INT_AS_STRING, "%d", f->fileSize);
    mapping+=INT_AS_STRING;
    snprintf(mapping, CHAR_WRITE_SPACE, "%c", f->descriptor);
    mapping+=CHAR_WRITE_SPACE;
    for (int i = 0; i < 256; i++) {
        snprintf(mapping, INT_AS_STRING, "%d", f->filePointers[i]);
        mapping+=INT_AS_STRING;
    }
    snprintf(mapping, INT_AS_STRING, "%d", f->FATblock);
    mapping+=INT_AS_STRING;

    munmap(mapping, BLOCK_SIZE * block);
    return 0;
}
/*
TestFS Code
Developers: Tanya Raj & Divyaank Tiwari
Reference:  https://dl.acm.org/doi/10.5555/ 862565
Date: April 21st 2024
*/
#define MAGIC_NO 0x58494e55
#define BLOCK_SIZE 512
#define MAX_BLOCKS 256
/* For our simple file system there are only 64-4=60 files possible at max*/
#define MAX_INODES 64
#define INODE_BLOCK_NO 8
#define FIRST_DATA_BLOCK 70
/*Keeping the first and second block reserved hence 8 and 9 blcok no are reserved*/
#define INODE_BLOCK_NO_ROOT 10
/*Limits the file size to 9*512*/
#define DIRECT_BLOCKS 16 
/*Block 0 is used for superblock so we start indoes from block 8*/
#define DIR_NAMELEN 64
#define FS_INODE_FREE 0
#define FS_INODE_INUSE 1
#define FS_BLOCK_FREE 0
#define FS_BLOCK_INUSE 1
#define FS_CLEAN 0
#define FS_DIRTY 1
struct fs_superblock{
    uint32_t inode_map[MAX_INODES];
    uint32_t block_map[MAX_BLOCKS];
};
/*one inode per block restrictions*/
struct fs_inode{
    uint32_t type; /*file type*/
    uint32_t mode; /*permissions*/
    uint32_t uid; /*user id*/
    uint32_t gid; /*group id*/
    uint32_t size; /*file size*/
    uint32_t access_time; /*access time*/
    uint32_t change_time; /*change time*/
    uint32_t mod_time; /*modification time*/
    uint32_t direct_addr[DIRECT_BLOCKS]; /*The blocks containing file data*/
};
struct fs_file{
    uint32_t inode;
    char name[DIR_NAMELEN];
};

//File operations supported
//Filling the superblock and getting the inode




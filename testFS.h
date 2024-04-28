/*
TestFS Code
Developers: Tanya Raj & Divyaank Tiwari
Reference:  https://dl.acm.org/doi/10.5555/ 862565
Date: April 21st 2024
*/
/*This define is used to format our print statements to include module names*/
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#define MAGIC_NO 0x58494e55
#define BLOCK_SIZE 512
#define MAX_BLOCKS 256
/* For our simple file system there are only 64-4=60 files possible at max*/
#define MAX_INODES 64
/*Block 0 is used for superblock so we start indoes from block 8*/
#define INODE_BLOCK_NO 8
/*Data blocks start from block number 80*/
#define FIRST_DATA_BLOCK 80
/*Keeping the first and second block reserved hence 8 and 9 blcok no are reserved (For Root directory)*/
#define INODE_BLOCK_NO_ROOT 10
/*Limits the file size to 16*512*/
#define DIRECT_BLOCKS 16 
#define MAX_FILES (BLOCK_SIZE/sizeof(struct fs_file))
#define DIR_NAMELEN 64
#define FS_INODE_FREE 0
#define FS_INODE_INUSE 1
#define FS_BLOCK_FREE 0
#define FS_BLOCK_INUSE 1
#define FS_CLEAN 0
#define FS_DIRTY 1
struct fs_superblock{
    uint32_t magic;
    uint32_t inode_map[MAX_INODES];
    uint32_t block_map[MAX_BLOCKS];
};
/*one inode per block restrictions*/
//This struct has inode and information asscoiated with that inode in direct_addr
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
/*The struct to store file data i.e inode and its name*/
struct fs_file{
    uint32_t inode;
    char name[DIR_NAMELEN];
};

struct fs_dir{
    struct fs_file files[MAX_FILES];
}

//File operations supported

//Filling the superblock and getting the inode
int fs_superblock_initialize(struct super_block *sb, void *data, int silent);
//Get inode
struct inode *fs_get_inode(struct super_block *sb, unsigned long inode);
/*Functions to support superblock, inode, file, dir, address space operations*/
static struct file_system_type fs_file_system_type;
static struct super_operations fs_super_ops;
static struct inode_operation fs_inode_ops;
static struct file_operations fs_file_ops;
static struct file_operations fs_dir_ops;
static struct address_space_operations fs_addrops;

#define FS_SB(sb) (sb->s_fs_info);



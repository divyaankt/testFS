/*
TestFS Code
Developers: Tanya Raj & Divyaank Tiwari
Reference:  https://dl.acm.org/doi/10.5555/ 862565
Date: April 21st 2024
*/
/*This define is used to format our print statements to include module names*/
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include <errno.h>
#include "testFS.h"
/*TODO: Read comments here.
 For all todos on this file you need to define the variables in testFS.c itself since it is static variables.
 I have tried to change the structure from the github code
 Many variables have been ommitted to make the code clean.
*/
int fs_superblock_initialize(struct super_block *sb, void *data, int silent){
    pr_info("Initialize the superblock\n");
    sb->s_blocksize = FS_BLOCK_SIZE;
    sb->s_magic = MAGIC_NO;
    sb->s_type = &fs_file_system_type; //Our filesystem type
    sb->s_op = &fs_super_ops; /*Not yet defined to be defined in testFS.c*/
    /*from the list of inodes in our superblock iget_locked gets the ino=1 inode*/
    struct inode *fs_root_inode = iget_locked(sb,1);

    if(!fs_root_inode){
        errno = EACCES;
        pr_err("Error getting inode\n");
        return errno;
    }
    //Allocated an inode from cache
    if(!(fs_root_inode->i_state & I_NEW)){
        return 0;
    }
    //If we got a fresh inode we need to fill it.
    //TODO: Not yet defined to be defined in testFS.c. It is initialized in testFS.h
    fs_root_inode->i_op = &fs_inode_ops;
    fs_root_inode->i_mode = htole32(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR |
                            S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH);
    fs_root_inode->i_uid = 0;
    fs_root_inode->i_gid = 0;
    fs_root_inode->i_size = FS_BLOCK_SIZE;
    fs_root_inode->i_ctime.tv_sec = 
    fs_root_inode->i_ctime.tv_nsec = 0;
    fs_root_inode->i_atime.tv_sec = 
    fs_root_inode->i_atime.tv_nsec = 0;
    fs_root_inode->i_mtime.tv_sec = 
    fs_root_inode->i_mtime.tv_nsec = 0;
    //TODO: Not yet defined to be defined in testFS.c. It is initialized in testFS.h
    fs_root_inode->i_fop = &fs_file_ops;
    //TODO: Not yet defined to be defined in testFS.c. It is initialized in testFS.h
    fs_root_inode->i_mapping->a_ops = &fs_addrops;
    unlock_new_inode(fs_root_inode);
    sb->s_root = d_make_root(fs_root_inode);
    if(!sb->s_root){
        iget_failed(fs_root_inode);
        pr_err("No memory available for root inode\n");
        errno = ENOMEM;
        return errno;
    }
    return 0;
}
int fs_block_alloc(struct super_block *sb){
    struct fs_superblock *fsb = (struct fs_superblock *)sb->s_private;
    if(fsb->fs_nbfree==0){
        pr_err("testfs: Out of space\n");
        return 0;
    }
    //Block 0 is always for the roort directory so we start from block 1
    for(int i=1;i<FS_MAX_BLOCKS;i++){
        if(fsb->block_map[i] == FS_BLOCK_FREE){
            fsb->block_map[i] = FS_BLOCK_INUSE;
            fsb->fs_nbfree--;
             mark_super_dirty(sb);
            return FS_FIRST_DATA_BLOCK+i;
        }
    }
    return 0;

}
 

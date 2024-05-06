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
#include <linux/cred.h>
#include "testFS.h"
/*TODO: Read comments here.
 For all todos on this file you need to define the variables in testFS.c itself since it is static variables.
 I have tried to change the structure from the github code
 Many variables have been ommitted to make the code clean.
*/
int fs_superblock_initialize(struct super_block *sb, void *data, int silent){
    struct inode *fs_root_inode;
    struct fs_superblock *fsb;
    struct fs_superblock *csb;
    struct buffer_head *bh;
    struct fs_inode *fsi;
    int i=0;
    pr_info("Initialize the superblock\n");
    sb_set_blocksize(sb, FS_BLOCK_SIZE);
    sb->s_magic = MAGIC_NO;
    sb->s_op = &fs_super_ops; /*Not yet defined to be defined in testFS.c*/
    //Adding the superblock  filesystem info to the superblock
    bh = sb_bread(sb,0);
    pr_info("Initialize the superblock1\n");
    csb = (struct fs_superblock *)bh->b_data;
    fsb = kzalloc(sizeof(struct fs_superblock),GFP_KERNEL);
    if(!fsb){
        return -ENOMEM;
    }
    for(i=0;i<FS_INODES_COUNT;i++){
        fsb->inode_map[i] = fsb->inode_map[i];
    }
    for(i=0;i<FS_MAX_BLOCKS;i++){
        fsb->block_map[i] = csb->block_map[i];
    }
    fsb->fs_nifree = csb->fs_nifree;
    fsb->fs_nbfree = csb->fs_nbfree;
    sb->s_fs_info = fsb;
    /*from the list of inodes in our superblock iget_locked gets the ino=1 inode*/
    fs_root_inode = iget_locked(sb,1);

    if(!fs_root_inode){
        pr_err("Error getting inode\n");
        return -EACCES;
    }
    //Allocated an inode from cache
    if(!(fs_root_inode->i_state & I_NEW)){
        return 0;
    }
    //If we got a fresh inode we need to fill it.
    //TODO: Not yet defined to be defined in testFS.c. It is initialized in testFS.h
    fs_root_inode->i_op = &fs_inode_ops;
    fs_root_inode->i_sb = sb;
    fs_root_inode->i_mode = le32_to_cpu(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH);
    i_uid_write(fs_root_inode, le32_to_cpu(0));
    i_gid_write(fs_root_inode, le32_to_cpu(0));
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
    fsi = kzalloc(sizeof(struct fs_inode),GFP_KERNEL);
    pr_info("Root inode struct\n");
    fsi->mode = (uint32_t)fs_root_inode->i_mode;
    fsi->n_links = 1;
    fsi->access_time=fsi->mod_time=fsi->change_time= 0;
    fsi->uid = (uint32_t)i_uid_read(fs_root_inode);
    fsi->gid = (uint32_t)i_gid_read(fs_root_inode);
    fsi->n_blocks = 1;
    fsi->size = FS_BLOCK_SIZE;
    memset(fsi->direct_addr,0,FS_DIRECT_BLOCKS*sizeof(uint32_t));
    fs_root_inode->i_private = fsi;
    unlock_new_inode(fs_root_inode);
    sb->s_root = d_make_root(fs_root_inode);
    if(!sb->s_root){
        iget_failed(fs_root_inode);
        pr_err("No memory available for root inode\n");
        return -ENOMEM;
    }
    pr_info("Exiting Superblock intialization\n");
    return 0;
}
//Get a free block from the filesystem space
int fs_block_alloc(struct super_block *sb){
    pr_info("Enter:fs_block_alloc\n");
    struct fs_superblock *fsb = (struct fs_superblock *)sb->s_fs_info;
    int i=1;
    if(fsb->fs_nbfree==0){
        pr_err("testfs: Out of space\n");
        return 0;
    }
    //Block 0 is always for the roort directory so we start from block 1
    for(i=1;i<FS_MAX_BLOCKS;i++){
        if(fsb->block_map[i] == FS_BLOCK_FREE){
            fsb->block_map[i] = FS_BLOCK_INUSE;
            fsb->fs_nbfree--;
            return FS_FIRST_DATA_BLOCK+i;
        }
    }
    pr_info("Exit:fs_block_alloc\n");
    return 0;

}
//This function gives up the superblock information back to the kernel
static void testfs_put_super(struct super_block *sb){
    pr_info("Enter:testfs_put_super\n");
    struct fs_superblock *fsb = (struct fs_superblock *)sb->s_fs_info;
    if(fsb){
        kfree(fsb);
    }
    pr_info("Exit:testfs_put_super\n");
}
//Writes a dirty inode
static int testfs_write_inode(struct inode *inode, struct writeback_control *wbc){
    pr_info("Enter:testfs_write_inode\n");
    struct fs_inode *disk_inode;
    struct fs_inode *fsi = (struct fs_inode *)inode->i_private;
    struct super_block *sb = inode->i_sb;
    struct fs_superblock *sbi = sb->s_fs_info;
    struct buffer_head *bh;
    int i=0;
    uint32_t ino = inode->i_ino;
    if(ino>= FS_INODES_COUNT){
        return 0;
    }
    bh = sb_bread(sb,FS_INODE_BLOCK_NO+ino);
    if (!bh)
        return -EIO;
    disk_inode = (struct fs_inode *)bh->b_data;
    disk_inode->mode = (uint32_t)inode->i_mode;
    disk_inode->uid = (uint32_t)i_uid_read(inode);
    disk_inode->gid = (uint32_t)i_gid_read(inode);
    disk_inode->size = inode->i_size;
    disk_inode->change_time = inode->i_ctime.tv_sec;
    disk_inode->access_time = inode->i_atime.tv_sec;
    disk_inode->mod_time = inode->i_mtime.tv_sec;
    disk_inode->n_blocks = inode->i_blocks;
    disk_inode->n_links = inode->i_nlink;
    for(i=0;i<FS_DIRECT_BLOCKS;i++){
        disk_inode->direct_addr[i] = fsi->direct_addr[i];
    }
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    pr_info("Exit:testfs_write_inode\n");
    return 0;
}
//Syncs the file system when fsync is called
static int testfs_syncfs(struct super_block *sb, int wait){
    pr_info("Enter:testfs_syncfs\n");
    struct fs_superblock *fsb = sb->s_fs_info;
    struct fs_superblock *disk_fsb;
    struct buffer_head *bh;
    int i=0;
    bh = sb_bread(sb,0);
    if(!bh)
        return -EIO;
    disk_fsb = (struct fs_superblock *)bh->b_data;
    for(i=0;i<FS_INODES_COUNT;i++){
        disk_fsb->inode_map[i] = fsb->inode_map[i];
    }
    for(i=0;i<FS_MAX_BLOCKS;i++){
        disk_fsb->block_map[i] = fsb->block_map[i];
    }
    disk_fsb->fs_nifree= fsb->fs_nifree;
    disk_fsb->fs_nbfree = fsb->fs_nbfree;
    mark_buffer_dirty(bh);
    if(wait){
        sync_dirty_buffer(bh);
    }
    brelse(bh);
    pr_info("Exit:testfs_syncfs\n");
    return 0;
}
//Statfs
static int testfs_statfs(struct dentry *dentry, struct kstatfs *stat){
    pr_info("enter:testfs_statfs\n");
    struct super_block *sb = dentry->d_sb;
    struct fs_superblock *fsb = sb->s_fs_info;
    stat->f_type=MAGIC_NO;
    stat->f_bsize= FS_BLOCK_SIZE;
    stat->f_blocks = FS_MAX_BLOCKS;
    stat->f_bfree = fsb->fs_nbfree;
    stat->f_bavail = fsb->fs_nbfree;
    stat->f_files = FS_MAX_FILES - fsb->fs_nifree;
    stat->f_ffree = fsb->fs_nifree;
    stat->f_namelen = FS_DIR_NAMELEN;
    return 0;
}
struct super_operations fs_super_ops = {
    .put_super = testfs_put_super,
    .write_inode = testfs_write_inode,
    .sync_fs = testfs_syncfs,
    .statfs = testfs_statfs,
};
 

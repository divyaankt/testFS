#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
/*
TestFS Code
Developers: Tanya Raj & Divyaank Tiwari
Reference:  https://dl.acm.org/doi/10.5555/ 862565
Date: April 28th 2024
*/
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mpage.h>
#include "testFS.h"
int testfs_file_get_block(struct inode *inode, sector_t blkno, struct buffer_head *bh, int create){
    struct super_block *sb = inode->i_sb;
    struct fs_inode *fsi =  kzalloc(sizeof(struct fs_inode),GFP_KERNEL);
    uint32_t blk=0;
    //Check if file size doesn't exceed the number of possible blocks
    if(blkno>=FS_DIRECT_BLOCKS){
        return -EFBIG;
    }
    if(create){
        blk = fs_block_alloc(sb);
        if(blk>0){
            fsi->direct_addr[blkno] = blk;
            fsi->n_blocks++;
            fsi->size = inode->i_size;
            inode->i_private = fsi;
            mark_inode_dirty(inode);
        }
        else{
            pr_err("FS: testfs_file_get_block: out of space\n");
            return -ENOSPC;
        }
    }
    //Mapping the block change to the bufferhead
    map_bh(bh,sb,blk);
    brelse(bh);
    return 0;
}
static int testfs_readpage(struct file *file, struct page *page){
    //Same get block?
    return mpage_readpage(page,testfs_file_get_block);
}
static int testfs_writepage(struct page *page, struct writeback_control *wbc){
    return block_write_full_page(page,testfs_file_get_block, wbc);
}
static int testfs_begin(struct file *file, struct address_space *mapping, loff_t pos, unsigned int len, unsigned int flags, struct page **pagep, void **fsdata){
    if(pos+len>FS_DIRECT_BLOCKS*FS_BLOCK_SIZE){
        return -ENOSPC;
    }
    return block_write_begin(mapping, pos, len, flags, pagep,testfs_file_get_block);
}
struct address_space_operations fs_addrops ={
    .readpage = testfs_readpage,
    .writepage = testfs_writepage,
    .write_begin = testfs_begin,
    .write_end = generic_write_end,
};
struct file_operations fs_file_ops={
    .llseek = generic_file_llseek,
    .owner = THIS_MODULE,
    .read_iter = generic_file_read_iter,
    .write_iter = generic_file_write_iter,
    .fsync = generic_file_fsync,
};
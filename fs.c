/*
TestFS Code
Developers: Tanya Raj & Divyaank Tiwari
Reference:  https://dl.acm.org/doi/10.5555/ 862565
Date: April 21st 2024
*/
/*This define is used to format our print statements to include module names*/
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include "testFS.h"
/*Mount the FS partition*/
struct dentry *fs_mount(struct file_system_type *fs_type, int flags, const char *device, void *data){
    struct dentry *dentry =mount_bdev(fs_type, flags, device, data, fs_superblock_initialize);
    if (IS_ERR(dentry))
        pr_err("'%s' mount failure\n", device);
    else
        pr_info("'%s' mount success\n", device);

    return dentry;
}
/*Unmount the FS partition*/
void fs_unmount(struct super_block *sb){
    kill_block_super(sb);
    pr_info("unmounted disk\n");
}
static struct file_system_type testfs_file_system_type ={
    .name = "testFS",
    .owner = THIS_MODULE, /*Always THIS_MODULE*/
    .mount = fs_mount,
    .kill_sb = fs_unmount,
    .fs_flags = FS_REQUIRES_DEV, /*This flag tells that the FS is a block device fs.*/
    .next = NULL,
};
/*This function is called when the module is loaded. It is kernel requirement to register filesystem*/
static int  __init testfs_mod_init(void){
    int ret = register_filesystem(&testfs_file_system_type);
    if(ret){
        pr_err("Register Filesystem failes\n");
        return ret;
    }

    pr_info("TestFS Module Loaded\n");
    return ret;
} 
/*This function will unload the module ---> All operations necessary to unload the module need sto be implemented here*/
static void __exit testfs_mod_exit(void){
    int ret = unregister_filesystem(&testfs_file_system_type);
    if(ret){
        pr_err("Unregister filesystem failed\n");
    }

    pr_info("TestFS Module Unloaded\n");

}
/*This is a variable of file_system_type which is used to register our filesystem to our block device.*/
/*calling module_init and module_exit kernel level functions to point to our init and exit functions*/
module_init(testfs_mod_init);
module_exit(testfs_mod_exit);
/*It is an identifier for our fs for the VFS.*/

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Tanya Raj");
MODULE_DESCRIPTION("a test file system");
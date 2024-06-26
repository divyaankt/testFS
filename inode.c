#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
/*
TestFS Code
Developers: Tanya Raj & Divyaank Tiwari
Reference:  https://dl.acm.org/doi/10.5555/ 862565
Reference: https://github.com/sysprog21/simplefs/tree/master
Date: April 28th 2024
*/
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "testFS.h"
ino_t allocate_inode(struct super_block *sb){
    //In case of new file or file creation, new inode needs to be allocated
    struct fs_superblock *fsb= sb->s_fs_info;
    int i=2;
    if(fsb->fs_nifree==0){
        printk("testfs: Out of inodes\n");
        return 0;
    }
    for(i=2; i<FS_MAX_FILES;i++){
        if(fsb->inode_map[i]==FS_INODE_FREE){
            fsb->inode_map[i] = FS_INODE_INUSE;
            fsb->fs_nifree--;
            return i;
        }
    }
    return 0;
}
int testfs_find_inode(struct inode *inode, char *name){
    //In case of name lookups we need to find the inode that matched the name of the directory
    struct fs_inode *fsi = (struct fs_inode *)inode->i_private;
    struct super_block *sb = inode->i_sb;
    //Need to see how a bufferhead functions??
    struct buffer_head *bh;
    struct fs_file *ent;
    int i=0;
    int j =0;
    for(i=0;i<fsi->n_blocks;i++){
        bh = sb_bread(sb,fsi->direct_addr[i]);
        ent = (struct fs_file *)bh->b_data;
        for(j=0;j<FS_DIRECT_BLOCKS;j++){
            if(strcmp(ent->name,name)==0){
                brelse(bh);
                return ent->inode;
            }
            ent++;
        }
    }
    brelse(bh);
    return 0;
}

int testfs_addto_dir(struct inode *dir,const char *name, int ino){
    //Once a new inode is allocated to a file or directory it need to be added to the root directory inode
    struct fs_inode *fsi = (struct fs_inode *)dir->i_private;
    struct buffer_head *bh;
    struct super_block *sb = dir->i_sb;
    struct fs_file *ent;
    int i=0;
    int j=0;
    //Each inode can have FS_DIRECT_BLOCKS(data blocks) it can point to (1 inode ---> Many direct blocks)
     for(i=0;i<fsi->n_blocks;i++){
        //Gets the inode's ith block's address
        bh = sb_bread(sb,fsi->direct_addr[i]);
        //Gets the file pointed by the data block (each data block can have FS_DIRECT_BLOCKS file entries)
        ent = (struct fs_file *)bh->b_data;
        for(j=0;j<FS_DIRECT_BLOCKS;j++){
            if(ent->inode==0){
                ent->inode = ino;
                strcpy(ent->name,name);
                mark_buffer_dirty(bh);
                mark_inode_dirty(dir);
                brelse(bh);
                return 0;
            }
            ent++;
        }
    }
    brelse(bh);
    /*TODO: IMPLEMENT TO ADD ADDITIONAL BLOCK IN CASE NO OF FILES IN ONE DIR >16 (LATER)*/
    //Allocate new block if there is no space in the current block (TO BE IMPLEMENTED LATER)
    if(fsi->n_blocks<FS_DIRECT_BLOCKS){

    }
    return 0;
}
static int testfs_remove_from_dir(struct inode *dir, struct dentry *dentry){
    struct super_block *sb = dir->i_sb;
    struct buffer_head *bh;
    char *name = (char *)dentry->d_name.name;
    struct fs_inode *fsi = (struct fs_inode *)dir->i_private;
    struct fs_file *ent;
    int blk = 0;
    int i=0;
    for(blk = 0;blk<fsi->n_blocks;blk++){
        bh = sb_bread(sb, fsi->direct_addr[blk]);
        ent = (struct fs_file *)bh->b_data;
        for(i=0;i<FS_DIRECT_BLOCKS;i++){
            if(strcmp(name,ent->name)==0){
                ent->inode=0;
                ent->name[0]='\0';
                mark_buffer_dirty(bh);
                inode_dec_link_count(dir);
                mark_inode_dirty(dir);
                break;
            }
            ent++;
        }
        brelse(bh);
    }
    return 0;
}
static struct dentry *testfs_lookup(struct inode *inode, struct dentry *dentry, unsigned int flags){
    struct inode *currnode = NULL;
    ino_t ino;
    if(dentry->d_name.len>FS_DIR_NAMELEN){
        return ERR_PTR(-ENAMETOOLONG);
    }
    ino = testfs_find_inode(inode, (char *)dentry->d_name.name);
    if(ino>0){
        currnode = iget_locked(inode->i_sb,ino);
        unlock_new_inode(currnode);
        if (!inode) {
            return ERR_PTR(-EACCES);
        }
    }
    d_add(dentry,currnode);
    return NULL;
}

static  int testfs_link(struct dentry *old_dentry, struct inode *inode, struct dentry *new_dentry){ 
    //Get the old inode pointed by the old dentry
    struct inode *oldinode = d_inode(old_dentry);
    //Increment the no. of links (atomic operation)
    inode_inc_link_count(oldinode);
    //mark the inode dirty to be written back to the disk
    mark_inode_dirty(oldinode);
    //This lets the system know this inode is being refrenced by other system processes
    ihold(oldinode);
    //This will create the mapping from dentry to inode, making the dentry useful
    d_instantiate(new_dentry, oldinode);
    return 0;
}

static int testfs_unlink(struct inode* dir, struct dentry *dentry){
    struct inode *inode = d_inode(dentry);
    testfs_remove_from_dir(dir,dentry);
    inode_dec_link_count(inode);
    mark_inode_dirty(inode);
    return 0;
}

int testfs_mkdir(struct user_namespace *ns, struct inode *dir, struct dentry *dentry, umode_t mode){
    //Check if this dir already exists
    int ino =0;
    ino = testfs_find_inode(dir, (char *)dentry->d_name.name);
    struct super_block *sb ;
    struct fs_superblock *fsb;
    struct inode *inode;
    struct fs_inode *fsi;
    struct buffer_head *bh;
    struct fs_file *ent;
    if(ino){
        return -EEXIST;
    }
    sb = dir->i_sb;
    fsb= sb->s_fs_info;
    ino = allocate_inode(sb);
    inode =  iget_locked(sb,ino);
    unlock_new_inode(inode);
    if(!inode){
        fsb->inode_map[ino] = FS_INODE_FREE;
        fsb->fs_nifree++;
        iput(inode);
        return -ENOSPC;
    }
    testfs_addto_dir(dir,(char *)dentry->d_name.name,ino);
    //Initializing the inode with the required details
    inode->i_mode = dir->i_mode;
    i_uid_write(inode, i_uid_read(dir));
    i_gid_write(inode, i_gid_read(dir));
    inode->i_sb = sb;
    set_nlink(inode,2);
    inode->i_atime = current_time(inode);
    inode->i_ctime = current_time(inode);
    inode->i_mtime = current_time(inode);
    inode->i_op = &fs_inode_ops;
    inode->i_fop = &fs_file_ops;
    inode->i_mapping->a_ops = &fs_addrops;

    fsi = kzalloc(sizeof(struct fs_inode),GFP_KERNEL);
    fsi->mode = mode;
    fsi->n_links = 2;
    fsi->access_time=fsi->mod_time=fsi->change_time= 0;
    fsi->uid = (uint32_t)i_uid_read(dir);
    fsi->gid = (uint32_t)i_gid_read(dir);
    fsi->n_blocks = 1;
    fsi->size = FS_BLOCK_SIZE;
    memset(fsi->direct_addr,0,FS_DIRECT_BLOCKS*sizeof(uint32_t));

    //Now we need to get the data block pointed by the inode and add the . and .. files to it.
    int blkno = fs_block_alloc(sb);
    fsi->direct_addr[0] = blkno; 
    inode->i_private = fsi;
    bh = sb_bread(sb,blkno);
    //This will get us a data block of sixe FS_BLOCK_SIZE from our file system buffer
    memset(bh->b_data,0,FS_BLOCK_SIZE);
    //We will write data to this data block that is containing the files for this directory.
    ent = (struct fs_file *)bh->b_data;
    ent->inode = ino;
    strcpy(ent->name,".");
    ent++;
    ent->inode = ino;
    strcpy(ent->name,"..");
    mark_buffer_dirty(bh);
    brelse(bh);
    //Insert the inode to hash table for faster lookups
    insert_inode_hash(inode); 
    d_instantiate(dentry,inode);
    //Increment the no. of links (atomic operation)
    inode_inc_link_count(dir);
    //mark the inodes dirty to be written back to the disk
    mark_inode_dirty(dir);
    mark_inode_dirty(inode);
    //This lets the system know this inode is being refrenced by other system processes
    ihold(dir);
    return 0;
}
static int testfs_rmdir(struct inode *dir, struct dentry *dentry){
    struct super_block *sb = dir->i_sb;
    struct inode *inode = d_inode(dentry);
    struct fs_superblock *fsb;
    struct fs_inode *fsi;
    int ino=0;
    int i=0;
    if(inode->i_nlink > 2){
        return -ENOTEMPTY;
    }
    ino = testfs_find_inode(dir,(char *)dentry->d_name.name);
    if(!ino){
        return -ENOTDIR;
    }
    testfs_remove_from_dir(dir,dentry);
    fsb = (struct fs_superblock *)sb->s_fs_info;
    fsi = (struct fs_inode *)dir->i_private;
    for(i=0;i<FS_DIRECT_BLOCKS;i++){
        if(fsi->direct_addr[i]!=0){
            int blkno = fsi->direct_addr[i];
            fsb->block_map[blkno] = FS_BLOCK_FREE;
            fsb->fs_nbfree++;
        }
    }
    fsb->inode_map[ino] = FS_INODE_FREE;
    fsb->fs_nifree++;
    return 0;
}
static int testfs_create(struct user_namespace *ns, struct inode *dir, struct dentry *dentry, umode_t mode, bool excl){
    //Check if this dir already exists
    struct super_block *sb;
    struct fs_superblock *fsb;
    struct inode *inode;
    struct fs_inode *fsi;
    ino_t ino = testfs_find_inode(dir, (char *)dentry->d_name.name);
    if(ino){
        return -EEXIST;
    }
    sb = dir->i_sb;
    fsb = sb->s_fs_info;
    ino = allocate_inode(sb);
    inode =  iget_locked(sb,ino);
    unlock_new_inode(inode);
    if(!inode){
        fsb->inode_map[ino] = FS_INODE_FREE;
        fsb->fs_nifree++;
        iput(inode);
        return -ENOSPC;
    }
    testfs_addto_dir(dir,(char *)dentry->d_name.name,ino);
    //Initializing the inode with the required details
    inode->i_mode = dir->i_mode;
    set_nlink(inode,2);
    i_uid_write(inode, i_uid_read(dir));
    i_gid_write(inode,i_gid_read(dir));
    inode->i_sb = sb;
    inode->i_atime = current_time(inode);
    inode->i_ctime = current_time(inode);
    inode->i_mtime = current_time(inode);
    inode->i_op = &fs_inode_ops;
    inode->i_fop = &fs_file_ops;
    inode->i_mapping->a_ops = &fs_addrops;
    fsi = kzalloc(sizeof(struct fs_inode),GFP_KERNEL);
    fsi->mode = mode;
    fsi->n_links = 2;
    fsi->access_time=fsi->mod_time=fsi->change_time= 0;
    fsi->uid = (uint32_t)i_uid_read(dir);
    fsi->gid = (uint32_t)i_gid_read(dir);
    fsi->n_blocks = 1;
    fsi->size = FS_BLOCK_SIZE;
    memset(fsi->direct_addr,0,FS_DIRECT_BLOCKS*sizeof(uint32_t));
    inode->i_private = fsi;
    //Insert the inode to hash table for faster lookups
    insert_inode_hash(inode); 
    d_instantiate(dentry,inode);
    //Increment the no. of links (atomic operation)
    inode_inc_link_count(dir);
    //mark the inode dirty to be written back to the disk
    mark_inode_dirty(inode);
    mark_inode_dirty(dir);
    //This lets the system know this inode is being refrenced by other system processes
    ihold(dir);
    return 0;
}
struct inode_operations fs_inode_ops={
    .create = testfs_create,
    .lookup = testfs_lookup,
    .unlink = testfs_unlink,
    .link = testfs_link,
    .mkdir = testfs_mkdir,
    .rmdir = testfs_rmdir,
};

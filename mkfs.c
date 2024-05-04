#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "testFS.h"
static struct fs_superblock *write_superblock(int fd){
    struct fs_superblock *sb = malloc(sizeof(struct fs_superblock));
    if(!sb){
        return NULL;
    }

    sb->magic = MAGIC_NO;
    //The last inode is for lost and found and the first 2 are reserved
    sb->fs_nifree = FS_MAX_FILES-4;
    sb->fs_nbfree = FS_MAX_BLOCKS-2;
    sb->inode_map[0] = FS_INODE_INUSE;
    sb->inode_map[1] = FS_INODE_INUSE;
    sb->inode_map[2] = FS_INODE_INUSE;
    sb->inode_map[3] = FS_INODE_INUSE;
    for(int i=4; i<FS_MAX_FILES;i++){
        sb->inode_map[i] = FS_INODE_FREE;
    }
    sb->block_map[0] = FS_BLOCK_INUSE;
    sb->block_map[1] = FS_BLOCK_INUSE;
    for (int i = 2 ; i < FS_MAX_BLOCKS ; i++) {
        sb->block_map[i] = FS_BLOCK_FREE;
    }
    int error = write(fd,sb,sizeof(struct fs_superblock));
    if(error!=sizeof(struct fs_superblock)){
        free(sb);
        return NULL;
    }
    return sb;
}
static int write_inodes(int fd, struct fs_superblock *sb){
    char *block = malloc(FS_BLOCK_SIZE);
    if(!block)
        return -1;
    memset(block,0,FS_BLOCK_SIZE);
    struct fs_inode *inode =(struct fs_inode *)block;
    inode->mode = htole32(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR |
                            S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH);
    inode->access_time = current_time(inode);
    inode->mod_time = current_time(inode);
    inode->change_time = current_time(inode);
    inode->gid = 0;
    inode->uid = 0;
    inode->size = FS_BLOCK_SIZE;
    inode->direct_addr[0]= FS_FIRST_DATA_BLOCK;
    inode->n_links = 2; //. and ..
    inode->n_blocks = 1;
    int error = write(fd,block,FS_BLOCK_SIZE);
    if(error!=FS_BLOCK_SIZE){
        free(block);
        return -1;
    }
    return 0;
}
static int write_block(int fd){
    //Writing the root directory
    char *block = malloc(FS_BLOCK_SIZE);
    if(!block)
        return -1;
    lseek(fd, FS_FIRST_DATA_BLOCK*FS_BLOCK_SIZE, SEEK_SET);
    memset(block,0,FS_BLOCK_SIZE);
    write(fd, block, FS_BLOCK_SIZE);
    lseek(fd, FS_FIRST_DATA_BLOCK*FS_BLOCK_SIZE, SEEK_SET);
    struct fs_dir *dir = malloc(sizeof(struct fs_dir));
    strcpy(dir->name,".");
    write(fd,dir,sizeof(struct fs_dir));
    strcpy(dir->name,"..");
    write(fd,dir,sizeof(struct fs_dir));
    return 0;
}
int main(int argc, char **argv){
    if(argc!=2){
        fprintf(stderr, "fs: Need to specify device\n");
        return EXIT_FAILURE;
    }
    int fd = open(argv[1],O_RDWR);
    if(fd<0){
        fprintf(stderr, "fs:Failed to open device\n");
        return EXIT_FAILURE;
    }
    //The filesystem size needs to be atleast 512*BLOCKSIZE
    int error = lseek(fd,(off_t)(FS_MAX_BLOCKS*FS_BLOCK_SIZE), SEEK_SET);
    if(error<0){
        fprintf(stderr, "fs:Device size too small to create a filesystem\n");
        return EXIT_FAILURE;
    }
    lseek(fd, 0, SEEK_SET);
    struct fs_superblock *sb = write_superblock(fd);
    if(!sb){
        fprintf(stderr, "fs:Not able to create a superblock\n");
        perror("write_superblock():");
        return EXIT_FAILURE;
    }
    lseek(fd,FS_INODE_BLOCK_NO*FS_BLOCK_SIZE+1024, SEEK_SET);
    //Write the root inode
    int ret = write_inodes(fd,sb);
    if(ret){
        perror("write_inodes():");
        return EXIT_FAILURE;
    }
    //Filling directory entries for root directory
    ret =write_block(fd);
    if(ret){
        perror("write_block():");
        return EXIT_FAILURE;
    }
}


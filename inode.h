// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pages.h"

typedef struct inode {
    int refs; // reference count
    int32_t  mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
    time_t atime;   /* time of last access */
    time_t mtime;   /* time of last modification */
    time_t ctime;   /* time of last status change */
    uid_t uid;     /* user ID of owner */
    gid_t gid;     /* group ID of owner */
    
} inode;

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode(int inum);
int grow_inode(inode* node, int size);
int shrink_inode(inode* node, int size);
int inode_get_pnum(inode* node, int fpn);

#endif

// based on cs3650 starter code

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
// #include <dirent.h>
#include <bsd/string.h>
#include <assert.h>
#include <stdlib.h>


#include "pages.h"
#include "inode.h"
#include "directory.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

int PAGE_SIZE = 4096;

// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
	printf("checking access of %s\n", path);
    int rv = 0;
	if (tree_lookup(path) < 0){	
		rv = -ENOENT;
	}
    printf("access(%s, %04o) -> %d\n", path, mask, rv);
    return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
	int inum  = tree_lookup(path);
	int rv = 0;
	if (inum < 0){
		rv = -ENOENT;
	}
	else {
	inode* node = get_inode(inum);
	st->st_mode = node->mode;
	st->st_size = node->size;
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_nlink = node->refs;
	}

    printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
	return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
	puts("Read Dir");
    struct stat st;
	slist* contents = directory_list(path);
// 	contents = contents->next;
// 	filler(buf, ".", &st, 0);
	
	while(contents != NULL){
		filler(buf, contents->data, &st, 0);
		printf("File Found: %s\n", contents->data);
		contents = contents->next;
	}
    printf("readdir(%s) -> %d\n", path, 0);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int rv = -1;
	
	
	char* fname = get_dir_filename(path);
	
	printf("MKNOD : %s with mode: %d\n", fname, mode);
	
	int inum = alloc_inode();
	inode* node = get_inode(inum);
	
	node->refs = 1;
	node->mode = mode;
	node->ptrs[0] = alloc_page();
	node->ptrs[1] = alloc_page();
	node->iptr = -1;
	node->atime = time(0);
	node->mtime = time(0);
	node->ctime = time(0);
	node->uid = getuid();
	node->gid = getgid();
	node->size = 0;
	
	int pnum = get_parent_dir_inum(path);
	inode* p_node = get_inode(pnum);
	
	rv = directory_put(p_node, fname, inum);
	
	printf("mknod(%s, %04o) -> %d\n", fname, mode, rv);
    return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
    int rv = nufs_mknod(path, mode | 040000, 0);
    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_unlink(const char *path)
{
	printf("unlinking %s", path);
	int inum = tree_lookup(path);
	free_inode(inum);
	
	char* file_name = get_dir_filename(path);
	
	int pnum = get_parent_dir_inum(path);
	inode* p = get_inode(pnum);
	
	directory_delete(p, file_name);
	
    printf("unlink(%s) -> %d\n", path, 0);
    return 0;
}

int
nufs_link(const char *from, const char *to)
{
    int rv = 0;
	int oinum = tree_lookup(from);
	inode* old = get_inode(oinum );
	
	int pnum = get_parent_dir_inum(to);
	
	old->refs += 1;
	
	char* n_filename = get_dir_filename(to);
	
	rv = directory_put(get_inode(pnum),n_filename, oinum); 
	
	old->atime = time(0);
	old->mtime = time(0);
	
    printf("link(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nufs_rmdir(const char *path)
{	
	int rv = nufs_unlink(path);
	
    printf("rmdir(%s) -> %d\n", path, rv);
    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
	puts("rename");
	int rv = 0;
	int oinum = tree_lookup(from);
	inode* old = get_inode(oinum );
	
	
	char* o_filename = get_dir_filename(from);
	char* n_filename = get_dir_filename(to);
	
	int pnum = get_parent_dir_inum(to);
	int onum = get_parent_dir_inum(from);
	
	if (pnum < 0){
		rv = -ENOENT;
	}
	else {
		inode* p = get_inode(pnum);
		inode* o = get_inode(onum);
		directory_delete(o, o_filename);
		directory_put(p, n_filename, oinum);
	}
	
	old->atime = time(0);
	old->mtime = time(0);
	
    printf("rename(%s => %s) -> %d\n", from, to, rv);
    return rv;
}

int
nufs_chmod(const char *path, mode_t mode)
{
    int rv = 0;
	int inum = tree_lookup(path);
	
	if (inum < 0){
		rv = -ENOENT;
	}
	else {
		inode* node  = get_inode(inum);
		node->mode = mode;
		node->atime = time(0);
		node->mtime = time(0);
		
	}
	
	
    printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

int
nufs_truncate(const char *path, off_t size)
{
	int inum = tree_lookup(path);
	inode* node = get_inode(inum);
	
	node->size = size;
	
	node->atime = time(0);
	node->mtime = time(0);
	printf("truncate(%s, %ld bytes) -> %d\n", path, size, 0);
    return 0;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
    int rv = 0;
    printf("open(%s) -> %d\n", path, rv);
    return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	
	int inum = tree_lookup(path);
	inode* node  = get_inode(inum);
	
// 	get the ceiling of the number of pages to read	
	int pages = (size + 4096 - 1) / 4096;
	
	int read = 0;
	
	int start_p = offset / 4096;
	int off_by = offset;
	if (offset > PAGE_SIZE) {
		off_by = offset % 4096;
	}
	
	printf("Reading %d pages, start: %d, off_by: %d\n", pages, start_p, off_by);
	
	
	for (int i = start_p; i < (pages + start_p); i++){
		int rd = PAGE_SIZE;
		
//		only write the remainder on the last page
		if ( (size - read) < PAGE_SIZE){
			rd = size - read;
		}	
		
		printf("On Page: %d, Read: %d, Reading: %d\n", i, read, rd);
		
		int pnum = inode_get_pnum(node, i);
		char* file_data = pages_get_page(pnum);
		
// 		write with offset if its the first page being read to
		if (i != start_p){
			memcpy(buf + read, file_data, rd);
			read += rd;
		}
		else {
			puts("start");
			memcpy(buf + read, file_data + off_by, rd - off_by);
			read += (rd - off_by);
		}
		
	}	
	
	size_t read_size = node->size - offset;

	node->atime = time(0);
	
	printf("read(%s, %ld bytes, @+%ld) -> %ld\n", path, size, offset, read);
	return read;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	
	int inum = tree_lookup(path);
	inode* node= get_inode(inum);
	

	grow_inode(node, size + offset);
	
	int written = 0;
	
	int start_p = offset / 4096;
	
	int off_by = offset;
	if (offset > PAGE_SIZE) {
		off_by = offset % 4096;
	}
	
// 	get the ceiling of the number of pages needed
	int pages  = (size + 4096 - 1) / 4096;
	
	printf("Writing %d pages, start: %d, off_by: %d\n", pages, start_p, off_by);
	for (int i = start_p; i < (pages + start_p); i++){
		int write_amt = PAGE_SIZE;
		
// 		only write the remainder on the last page
		if ( (size - written) < PAGE_SIZE){
			puts("Writing less then page");
			write_amt = size - written;
		}
		printf("On Page: %d, Written: %d,Writing: %d\n", i, written, write_amt);
		
		int pnum = inode_get_pnum(node, i);
		char* file_data = pages_get_page(pnum);
		
// 		write with offset if its the first page being written to
		if (i != start_p){
			strncpy(file_data, buf + written, write_amt);
			written += write_amt;
		}
		else {
			puts("start");
			strncpy(file_data + off_by, buf + written, write_amt - off_by);
			written += (write_amt - off_by);
		}
	}	
	
	node->size = size + offset;
	int rv = size;
	
	node->atime = time(0);
	node->mtime = time(0);
	
	printf("node size: %d\n", node->size);
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
    int rv = 0;
	
	int inum = tree_lookup(path);
	if (inum == -1){
		puts("utimens no file found");
		return -ENOENT;
	}
	
	inode* node = get_inode(inum);
	node->atime = time(0);
	
	
    printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
           path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	return rv;
}

// Extended operations
int
nufs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
           unsigned int flags, void* data)
{
    int rv = -1;
    printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
    return rv;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->link     = nufs_link;
    ops->unlink   = nufs_unlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open	  = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
    ops->ioctl    = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);
    printf("TODO: mount %s as data file\n", argv[(argc -1)]);
    pages_init(argv[--argc]);
    nufs_init_ops(&nufs_ops);
	puts("---INITS DONE---");
    return fuse_main(argc, argv, &nufs_ops, NULL);
}


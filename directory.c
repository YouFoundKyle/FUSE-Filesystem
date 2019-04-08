#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "directory.h"
#include "slist.h"
#include "pages.h"
#include "inode.h"
#include "util.h"
#include "bitmap.h"

// typedef struct dirent {
//     char name[DIR_NAME];
//     int  inum;
//     char _reserved[12];
// } dirent;

// typedef struct inode {
// int refs; // reference count
// int32_t  mode; // permission & type
// int size; // bytes
// int ptrs[2]; // direct pointers
// int iptr; // single indirect pointer
// time_t atime;   /* time of last access */
// time_t mtime;   /* time of last modification */
// time_t ctime;   /* time of last status change */
// uid_t uid;     /* user ID of owner */
// gid_t gid;     /* group ID of owner */
// } inode;


int ROOT_NM = 0;
size_t DIRENT_SIZE = sizeof(dirent); //64
int MAX_ENTRIES = 64; //4096/64


void directory_init(){
	puts("---DIR INIT---");
	printf("INODE SIZE: %ld", sizeof(inode)); 
	void* ibm =  get_inode_bitmap();	
	
	int ROOT_NM = alloc_inode();
	inode* root = get_inode(ROOT_NM);

	root->refs = 1;
// 	Set root to directory
	root->mode = 040755;
	root->size = 0;
	root->ptrs[0] = alloc_page();
	root->ptrs[1] = alloc_page();
	root->iptr = -1;
	root->uid = getuid();
	root->gid = getgid();
	
	root->atime = time(0);
	root->mtime = time(0);
	root->ctime = time(0);
	bitmap_put(ibm,0,1);

// 	add a self pointer to the root
	directory_put(root, ".", ROOT_NM);
	print_inode(root);
}

int directory_lookup(inode* dd, const char* name){
	printf("directory lookup of %s\n", name);
	dirent* files = (dirent*) pages_get_page(dd->ptrs[0]);
	
	for (int i  = 0 ; i < MAX_ENTRIES; i++){
	
		if(streq(files->name, name)){
			return files->inum;
		}
		files = (void*)files + DIRENT_SIZE;
	}
	printf("%s not found\n", name);
	return -ENOENT;
}

int tree_lookup(const char* path){
	
// 	printf("tree lookup of %s\n", path);
	inode* root = get_inode(ROOT_NM);
	int dir_num = 0;
	
// 	split path and skip over initial blank
	slist* p = s_split(path, '/');
	p = p->next;
	
// 	root path was looked up
	if (streq(path, "/")){
		s_free(p);
		return dir_num;
	}
	else{
		int pnum = get_parent_dir_inum(path);
		inode* par = get_inode(pnum);
		char* filename = get_dir_filename(path);
		
		dir_num = directory_lookup(par,filename );
		s_free(p);
		return dir_num;
	}
}

int directory_put(inode* dd, const char* name, int inum){
	printf("putting node # %d , %s\n", inum, name);
	
	dirent* files = (dirent*) pages_get_page(dd->ptrs[0]);
	
	for (int i  = 0 ; i < MAX_ENTRIES; i++){
		if(streq(files[i].name, "")){
			strcpy(files[i].name, name);
			files[i].inum = inum;
			return 0;
		}
	}
	return -ENOENT;
	
}

int directory_delete(inode* dd, const char* name){
	printf("deleting file %s\n", name);
	
	dirent* files = (dirent*) pages_get_page(dd->ptrs[0]);
	
	for (int i  = 0 ; i < MAX_ENTRIES; i++){
		if(streq(files[i].name, name)){
			strcpy(files[i].name, "");
			files[i].inum = -1;
			return 0;
		}
	}
	return -ENOENT;
	
}

slist* directory_list(const char* path){
	
// 	puts("getting directory list...");
	int inum = tree_lookup(path);
	inode* node = get_inode(inum);
	
	dirent* files = (dirent*) pages_get_page(node->ptrs[0]);
	
	slist* dir_cont = NULL;
	for (int i  = 0 ; i < MAX_ENTRIES; i++){
		if(streq(files[i].name, "")){
			break;
		}
		dir_cont = s_cons(files[i].name, dir_cont);
	}
	
	return dir_cont;
	
}

int get_parent_dir_inum(const char* path) {
// 	printf("get parent dir of %s\n", path);
	
	//TODO: free slist
	int pnum = 0;
	slist* p = s_split(path, '/');
	p = p->next;
	
	if (p->next == NULL){
		return pnum;
	}
	
	inode* pnode = get_inode(0);
	while (p->next != NULL && p->next->next != NULL && pnum >= 0){
		pnum = directory_lookup(pnode, p->data);
		pnode = get_inode(pnum);
		p = p->next;
	}
	
	pnum = directory_lookup(pnode, p->data);
	pnode = get_inode(pnum);
	return pnum;
	
}

char* get_dir_filename(const char* path) {

	//TODO: Free slist
	slist* p = s_split(path, '/');
	p = p->next;

	
	while (p->next != NULL ){
		p = p->next;
	}
	
// 	printf("filename: %s\n", p->data);
	return p->data;

}


void print_directory(inode* dd){
	
	
}

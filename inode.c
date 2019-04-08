
#include "inode.h"
#include "pages.h"
#include "bitmap.h"


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

size_t INODE_SIZE = sizeof(inode);
size_t MAX_INODES = 72;

void print_inode(inode* node){

	puts("---INODE---");
	printf("refs: %d\n", node->refs);
	printf("mode: %d\n", node->mode);
	printf("size: %d\n", node->size);
	printf("direct ptr: %d\n", node->ptrs[0]);
	printf("indir ptr: %d\n", node->iptr);
	
	
}

inode* get_inode(int inum){
	inode* inode_start = (inode*) (((uint8_t*) pages_get_page(0)) + 64);
	if (inum == 0){
		return inode_start;		
	}
	
	return inode_start + (inum * INODE_SIZE );
}

int alloc_inode(){
	void* ibm = get_inode_bitmap();
	
	puts("ALLOCATING INODE");
	for (int ii = 0; ii < MAX_INODES; ++ii) {
		if (!bitmap_get(ibm, ii)) {
			bitmap_put(ibm, ii, 1);
			printf("+ alloc_inode() -> %d\n", ii);
			return ii;
		}
	}
	
	puts("CANT ALLOCATE INODE");
	return -1;
	
}

void free_inode(int inum){
	inode* n = get_inode(inum);
	
	if (n->refs >1){
		--n->refs;
	}
	else{
		uint8_t* ibm = get_inode_bitmap();
		bitmap_put(ibm, inum,0);
		n->refs = 0;
		n->mode = 0;
		n->size = 0;
		free_page(n->ptrs[0]);
		free_page(n->ptrs[1]);
	}
	
}

int grow_inode(inode* node, int size){
// 	puts("Growing Node");
	
// 	get the ceiling of the number of pages needed
	int p_need = (size + 4096 - 1) / 4096;
	
	if (p_need > 2){
		p_need = p_need - 2;
		printf("Need +%d pages\n", p_need);
		if (node->iptr == -1){
			node->iptr =  alloc_page();
		}
		int* iptrs = (int*) pages_get_page(node->iptr);
		
		int i = 0;
		while(i < p_need){
			printf("iptr %d is %d\n",i,  iptrs[i]);
			if (iptrs[i] == 0){
				iptrs[i] = alloc_page();
				printf("iptr %d now %d\n",i, iptrs[i]);
			}
			i++;
		}
	}
	return p_need;
}

int shrink_inode(inode* node, int size){

	
}

int inode_get_pnum(inode* node, int fpn){

	if (fpn < 2){
		return node->ptrs[fpn];
	}
	else{
		int pnum = fpn - 2;
		 int* p = pages_get_page(node->iptr);
		 return p[pnum];
	}	
	
}



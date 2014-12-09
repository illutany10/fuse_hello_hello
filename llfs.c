// Header.
#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// User Definition
#define	MAX_NAME_LEN 100

typedef struct inode
{
	struct stat 	STAT;
	char* 			name;
	void*			data;

	struct inode*	parent;
	struct inode*	child;
	struct inode*	next;
	struct inode*	previous;
} inode;

// Global variables
static inode* root_node;

// Function Definition.
// find_node(inode* parent, const char* name)
// function. find the node that has a 'name' in child list of parent node.
static inode* find_node(inode* parent, const char* name){
	inode*	node;	//temporary inode

	//find the node in child list.
	for(node = parent->child; 
		(node != NULL) && strcmp(node->name, name); node = node->next);
	return node;
}

// find_path(const char* path)
// function. find the node that has a 'name' from the path.
static inode* find_path(const char* path)
{
	char* tmp;
	char* temp_path;
	inode* node = root_node;

	// if path is "/", return root
	if(strcmp(path, "/") == 0)
		return node;

	// temporary variable for 'strtok' function
	temp_path = (char*)calloc(strlen(path),sizeof(char));
	strcpy(temp_path, path);
	tmp = strtok(temp_path, "/");

	// if there's no node that has the name in child list, return NULL
	if((node = find_node(node, tmp)) == NULL) {
		free(temp_path);
		return NULL;
	}
	// find node down through directories
	for( ; (tmp = strtok(NULL, "/")) && node != NULL; 
		 node = find_node(node, tmp));

	free(temp_path);
	return node;
}

// find_parent(const char* path)
// get the path of parent by deleting parent name
static char* find_parent(const char* path) {
	int 	i = strlen(path);
	char*	parent = (char*)calloc(i, sizeof(char));

	strcpy(parent, path);
	// until it comes out '/', 
	// check string from the end of the path and delete characters one by one
	for(  ; parent[i] != '/' && i > 0 ; --i )
		parent[i] = '\0';

	return parent;
}

// hello_getattr(const char *path, struct stat *stbuf)
// get the metadata from the node that has the path
static int hello_getattr(const char *path, struct stat *stbuf)
{
    inode* node;

    puts("hello getattr");
    // find the node that has the same path
    node = find_path(path);

    if (node == NULL){
        puts("file not exist");
        return -ENOENT;
    }
    else{
    	puts("file exist");
	    memset(stbuf, 0, sizeof(struct stat));
	    stbuf->st_mode = node->STAT.st_mode;
	    stbuf->st_nlink = node->STAT.st_nlink;
	    stbuf->st_size = node->STAT.st_size;
	    stbuf->st_atime = node->STAT.st_atime;
	    stbuf->st_ctime = node->STAT.st_ctime;
	    stbuf->st_mtime = node->STAT.st_mtime;
    }
    // if node exists, copy the metadata to stbuf
    puts("hello getattr end");
    return 0;
}
// int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
// read the nodes in directory
static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                                 off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
  
    inode* node;
    puts("hello readdir ");
    // find current directory node
    node = find_path(path);
    if(node == NULL)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // fill all names of nodes in the directory
    for (node = node->child; node != NULL; node = node->next) 
        filler(buf, node->name, NULL, 0);
    
    puts("hello readdir end");  
    return 0;
}

// int hello_open(const char *path, struct fuse_file_info *fi)
// open the file
static int hello_open(const char *path, struct fuse_file_info *fi)
{
    inode* node;
    puts("hello open");
    // find the node that has the same path
    node = find_path(path);
    // if no node exists, return error
    if (node == NULL)
        return -ENOENT;

    puts("hello open end");  
    return 0;
}

// int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
// read the file
static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                             struct fuse_file_info *fi)
{
    size_t len;
    inode* node;
    (void) fi;
    puts("hello read");
    // find the node that has the same path
    node = find_path(path);
    // if no node exists, return error
    if(node == NULL)
        return -ENOENT;
    // len is size of the node
    len = node->STAT.st_size;
    // read from the offset
    if (offset < len) 
    {
        // if the size that we want to read is bigger that size of the node
        if (offset + size > len)
            size = len - offset;
        // the data of the node from offset upto size is copied to buf
        memcpy(buf, node->data + offset, size);
    }
    // data to read is not left
    else
        size = 0;
    puts("hello read end");  
    return size;
}

// int hello_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info *fi)
// write the file
static int hello_write(const char* path, const char* buf, size_t size, off_t offset,
                              struct fuse_file_info *fi)
{
    inode* node;
    void* tmp;
    (void) fi;
    puts("hello write");
    // find the node that has the same path
    node = find_path(path);
    // if no node exists, return error
    if(node == NULL)
        return -ENOENT;
    // the size of data that we want to write is bigger than current data size,
    if (size + offset > node->STAT.st_size)
    {
        // allocate new size of memory to temporary variable
        tmp = malloc(node->STAT.st_size + size);
        if (tmp) {
            if (node->data) {
                // copy data to temporary variable and free original data
                memcpy(tmp, node->data, node->STAT.st_size);
                free(node->data);
            }
            // copy data of temporary variable to original data section for new size
            node->data = tmp;
            node->STAT.st_size += size;
        }
        // if malloc doesn't work, return error;
        else{
        	free(tmp);
            return -EFBIG;
        }
    }
    // write new data from buf to data section of node after the offset
    memcpy(node->data + offset, buf, size);
    puts("ramfs write end");
    return size;
}

// int hello_mknode(const char * path, mode_t mode)
// make a node
static int hello_mknode(const char * path, mode_t mode)
{
    time_t	current_time = time(NULL);
    inode*	parent;
    inode* 	node;
    inode*	last_node;
    char*	parent_path = find_parent(path);
    int len = strlen(parent_path);
    int i;
	
	puts("hello mknod");
    // find parent node from path
    parent = find_path(parent_path);
    // if parent is NULL, return error
    if (parent == NULL)
        return -ENOENT;
    // if nlink of parent node isn't 2, it's not directory file.
    if(parent->STAT.st_nlink != 2)
    	return -ENOSPC;
    // if file not exist, return error
    if ( find_path(path) != NULL )
        return -EEXIST;

    // new node is allocated and initialized
  	node = (inode*)calloc(1, sizeof(inode));
  	node->parent = parent;
  	node->STAT.st_nlink = mode & S_IFDIR ? 2 : 1;
  	node->STAT.st_mode = mode;
  	node->STAT.st_mtime = current_time;
  	node->STAT.st_ctime = current_time;
  	node->STAT.st_atime = current_time;
  	node->child = NULL;
  	node->next = NULL;

    // name of new node is from path(after parent path)
  	node->name = (char*)calloc(MAX_NAME_LEN, sizeof(char));

  	for( i = len ; path[i] != '\0' ; i++ )
  		node->name[i-len] = path[i];
  	node->name[i-len] = '\0';

    // if parent node has no child node
  	if(parent->child == NULL){
  		puts("parent's first child");
  		parent->child = node;
  		node->previous = NULL;
  	}
    // if parent node has already a child node
    else {
  		puts("not a first child");
	  	for(last_node = parent->child; last_node->next != NULL; 
	  		last_node = last_node->next);
	  	last_node->next = node;
	  	node->previous = last_node;
	}
	puts("mknod end");
  	return 0;
}

// int hello_mknod(const char * path, mode_t mode, dev_t dev)
// make regular file
static int hello_mknod(const char * path, mode_t mode, dev_t dev)
{
    return hello_mknode(path, mode);
}
// int hello_mkdir(const char * path, mode_t mode)
// make directory file
static int hello_mkdir(const char * path, mode_t mode)
{
    return hello_mknode(path, mode | S_IFDIR);
}

// int realese_link(inode* node)
// cut the link of the node
int realese_link(inode* node)
{   
    // safe check
    if (!node || node->STAT.st_nlink == 0)
        return -EIO;
    // if node is root or 
    // node is directory file and it's not empty
    if (node == root_node ||
        ((node->STAT.st_mode & S_IFDIR) && ( node->child )) ) 
        return -EISDIR;
    // if node is first child of the parent
    if(node == node->parent->child){
    	if(node->next)
	    	node->next->previous = NULL;
		node->parent->child = node->next;
		if(node->data)
			free(node->data);
		free(node->name);
    	free(node);
    } 
    // if node is not first child of the parent
    else {
    	node->previous->next = node->next;
    	if(node->next)
	    	node->next->previous = node->previous;
		if(node->data)
			free(node->data);
		free(node->name);
    	free(node);
    }
    return 0;
}

// int hello_unlink(const char* path)
// remove regular file
static int hello_unlink(const char* path)
{
    inode* node;
    puts("hello unlink");
    // find the node that has the same path
    node = find_path(path);
    // if no node exists, return error
    if (node == NULL)
        return -ENOENT;

    puts("hello unlink end");
    return realese_link(node);
}

// int hello_rmdir(const char* path)
// remove directory file
static int hello_rmdir(const char* path)
{
    inode* node;
    // find the node that has the same path
    node = find_path(path);
    // if no node exists, return error
    if (node == NULL)
        return -ENOENT;

    return realese_link(node);
}

// int hello_utimens(const char *path, const struct timespec ts[2])
// change the access time and modification time
static int hello_utimens(const char *path, const struct timespec ts[2])
{
    // find the node that has the same path
	inode* node = find_path(path);
	time_t current_time = time(NULL);
    puts("utimens");
    // if no node exists, return error
    if(node == NULL)
    	return -ENOENT;
    node->STAT.st_mtime = current_time;
    node->STAT.st_atime = current_time;
	puts("utimens end");
	return 0;
}

// void* hello_init(struct fuse_conn_info *conn)
// initiate the tree, especially root node
static void* hello_init(struct fuse_conn_info *conn)
{
    (void) conn;
    time_t current_time = time(NULL);
    root_node = (inode*) calloc(1, sizeof(inode));
    root_node->parent = NULL;
    root_node->child = NULL;
    root_node->next = NULL;
    root_node->previous = NULL;
  	root_node->STAT.st_nlink = 2;
  	root_node->STAT.st_mode = (S_IFDIR | 755);
  	root_node->STAT.st_mtime = current_time;
  	root_node->STAT.st_ctime = current_time;
  	root_node->STAT.st_atime = current_time;
  	root_node->name = "";

    return NULL;
}

// fuse operations
static struct fuse_operations hello_oper = {
    .init 			= hello_init,
    .getattr    	= hello_getattr,
    .readdir 		= hello_readdir,
    .open    		= hello_open,
    .read    		= hello_read,
    .write 			= hello_write,
    .mknod 			= hello_mknod,
    .mkdir 			= hello_mkdir,
    .unlink 		= hello_unlink,
    .rmdir 			= hello_rmdir,
    .utimens 		= hello_utimens
};
  
int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &hello_oper, NULL);
}

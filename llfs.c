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

static inode* root_node;

static inode* find_node(inode* parent, const char* name){
	inode* node;
	for(node = parent->child; 
		(node != NULL) && strcmp(node->name, name); node = node->next);
	return node;
}

static inode* find_path(const char* path)
{
	char* tmp;
	char* temp_path;
	inode* node = root_node;
	if(strcmp(path, "/") == 0)
		return node;

	temp_path = (char*)calloc(strlen(path),sizeof(char));
	strcpy(temp_path, path);
	tmp = strtok(temp_path, "/");

	if((node = find_node(node, tmp)) == NULL) {
		free(temp_path);
		return NULL;
	}
	for( ; (tmp = strtok(NULL, "/")) && node != NULL; 
		 node = find_node(node, tmp));

	free(temp_path);
	return node;
}

static char* find_parent(const char* path) {
	int 	i = strlen(path);
	char*	parent = (char*)calloc(i, sizeof(char));

	strcpy(parent, path);

	for(  ; parent[i] != '/' && i > 0 ; --i )
		parent[i] = '\0';

	return parent;
}

static int hello_getattr(const char *path, struct stat *stbuf)
{
    int res;
    inode* node;

    puts("hello getattr");
    res = 0;
    node = find_path(path);

    if (node == NULL)
        return -ENOENT;
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
    puts("hello getattr end");
    return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                                 off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
  
    inode* node;
    puts("hello readdir ");
    node = find_path(path);
    if(node == NULL)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (node = node->child; node != NULL; node = node->next) 
        filler(buf, node->name, NULL, 0);
    
    puts("hello readdir end");  
    return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
    inode* node;
    puts("hello open");
    node = find_path(path);

    if (node == NULL)
        return -ENOENT;
  
    // you could test access permitions here
    // take a look at man open for possible errors
    puts("hello open end");  
    return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                             struct fuse_file_info *fi)
{
    size_t len;
    inode* node;
    (void) fi;
    puts("hello read ");
    node = find_path(path);

    if(node == NULL)
        return -ENOENT;
  
    len = node->STAT.st_size;

    if (offset < len) 
    {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, node->data + offset, size);
    }
    else
        size = 0;
    puts("hello read end");  
    return size;
}

static int hello_write(const char* path, const char* buf, size_t size, off_t offset,
                              struct fuse_file_info *fi)
{
    inode* node;
    void* tmp;
    (void) fi;
    puts("hello write");
    node = find_path(path);

    if(node == NULL)
        return -ENOENT;

    if (size + offset > node->STAT.st_size)
    {
        tmp = malloc(node->STAT.st_size + size);
        if (tmp) {
            if (node->data) {
                memcpy(tmp, node->data, node->STAT.st_size);
                free(node->data);
            }
            node->data = tmp;
            node->STAT.st_size += size;
        }
        else{
        	free(tmp);
            return -EFBIG;
        }
    }
    memcpy(node->data + offset, buf, size);
    puts("ramfs write end");
    return size;
}

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

    parent = find_path(parent_path);
    if (parent == NULL)
        return -ENOENT;
    if(parent->STAT.st_nlink != 2)
    	return -ENOSPC;
    if ( find_path(path) != NULL )
        return -EEXIST;

  	node = (inode*)calloc(1, sizeof(inode));
  	node->parent = parent;
  	node->STAT.st_nlink = mode & S_IFDIR ? 2 : 1;
  	node->STAT.st_mode = mode;
  	node->STAT.st_mtime = current_time;
  	node->STAT.st_ctime = current_time;
  	node->STAT.st_atime = current_time;
  	node->child = NULL;
  	node->next = NULL;
  	node->name = (char*)calloc(MAX_NAME_LEN, sizeof(char));

  	for( i = len ; path[i] != '\0' ; i++ )
  		node->name[i-len] = path[i];
  	node->name[i-len] = '\0';

  	if(parent->child == NULL){
  		puts("parent's first child");
  		parent->child = node;
  		node->previous = NULL;
  	} else {
  		puts("not a first child");
	  	for(last_node = parent->child; last_node->next != NULL; 
	  		last_node = last_node->next);
	  	last_node->next = node;
	  	node->previous = last_node;
	}
	puts("mknod end");
  	return 0;
}

static int hello_mknod(const char * path, mode_t mode, dev_t dev)
{
    return hello_mknode(path, mode);
}

static int hello_mkdir(const char * path, mode_t mode)
{
    return hello_mknode(path, mode | S_IFDIR);
}

int realese_link(inode* node)
{    
    if (!node || node->STAT.st_nlink == 0)
        return -EIO;

    if ( S_ISDIR(node->STAT.st_mode) && ( node->child || node == root_node ) )
        return -EISDIR;

    if(node == node->parent->child){
    	if(node->next)
	    	node->next->previous = NULL;
		node->parent->child = node->next;
		if(node->data)
			free(node->data);
		free(node->name);
    	free(node);
    } else {
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

static int hello_unlink(const char* path)
{
    inode* node;
    puts("hello unlink");
    node = find_path(path);

    if (node == NULL)
        return -ENOENT;

    puts("hello unlink end");
    return realese_link(node);
}

static int hello_rmdir(const char* path)
{
    inode* node;

    node = find_path(path);

    if (node == NULL)
        return -ENOENT;

    return realese_link(node);
}

static int hello_utimens(const char *path, const struct timespec ts[2])
{
	inode* node = find_path(path);
	time_t current_time = time(NULL);
    puts("utimens");
    if(node == NULL){
    	puts("utimes end abnormally");
    	return -errno;
    }
    node->STAT.st_mtime = current_time;
    node->STAT.st_atime = current_time;
	puts("utimens end normally");
	return 0;
}


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

static struct fuse_operations hello_oper = {
    .init 		= hello_init,
    .getattr    = hello_getattr,
    .readdir 	= hello_readdir,
    .open    	= hello_open,
    .read    	= hello_read,
    .write 		= hello_write,
    .mknod 		= hello_mknod,
    .mkdir 		= hello_mkdir,
    .unlink 	= hello_unlink,
    .rmdir 		= hello_rmdir,
    .utimens 	= hello_utimens
};
  
int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &hello_oper, NULL);
}

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

#define MAX_FS_VALUE 100
#define MAX_NAME_LEN 100


typedef struct {
	struct stat	STAT;
	char*		name;
	void*		data;

} inode;

static inode 	inodes[MAX_FS_VALUE];
static char		inode_flag[MAX_FS_VALUE];

static int inode_index = 1;

static int mk_root(){
	time_t cr_time = time(NULL);
	puts("mk_root");
	strcpy(inodes[0].name, "/");
	inodes[0].STAT.st_mode = S_IFDIR | 0755;
	inodes[0].STAT.st_nlink = 2;
	inodes[0].STAT.st_ino = 0;
	inode_flag[0] = 1;
	inodes[0].STAT.st_atime = cr_time;
	inodes[0].STAT.st_ctime = cr_time;
	inodes[0].STAT.st_mtime = cr_time;

	return 0;
}

static int init_inode()
{
	int i;
	puts("initialize");
	memset(inodes, 0, sizeof(inodes));
	
	for(i = 0; i < MAX_FS_VALUE; i ++) {

		inodes[i].name = (char*)calloc(sizeof(char),MAX_NAME_LEN);
		strcpy(inodes[i].name, "");
		inodes[i].data = NULL;
	}
	mk_root();
	puts("memset end");
	return 0;
}

static int find_empty_inode(){
	int cnt;
	for(cnt = 0 ; (inode_flag[inode_index]) && cnt < MAX_FS_VALUE; 
		inode_index = (inode_index + 1) % MAX_FS_VALUE, cnt ++);
	
	if(cnt == MAX_FS_VALUE){
		puts("file system is full");
		return -1;
	}
	return inode_index;
}

static int find_inode(const char *path){
	int cnt;
	for(cnt = 0 ; (cnt < MAX_FS_VALUE && strcmp(inodes[cnt].name, path) != 0) ;
		cnt ++ );
	if(cnt == MAX_FS_VALUE){
		puts("no file");
		return -1;
	}
	return cnt;
}



static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	int index = find_inode(path);
	puts("getAttr");
	puts(path);


	memset(stbuf, 0, sizeof(struct stat));

	if(index == -1){
		puts("file not exist");
		return -ENOENT;
	} else {
		puts("file exist");

		stbuf->st_mode = inodes[index].STAT.st_mode;
		stbuf->st_nlink = inodes[index].STAT.st_nlink;	
		stbuf->st_size = inodes[index].STAT.st_size;
		stbuf->st_atime = inodes[index].STAT.st_atime;
		stbuf->st_mtime = inodes[index].STAT.st_mtime;
		stbuf->st_ctime = inodes[index].STAT.st_ctime;
	}

	if (res == -1){
		puts("getAttr end (lstat = -1)");
		return -errno;
	}
	puts("getAttr end");
	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	int cnt;
	puts("readdir");	
	if (strcmp(path, "/") != 0)
		return -ENOENT;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for(cnt = 1; cnt < MAX_FS_VALUE; cnt ++){
		if(inode_flag[cnt] == 1)
			filler(buf, inodes[cnt].name + 1, NULL, 0);		
	}
	puts("readdir end");
	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{	
    int index;
    puts("hello open");
    puts(path);

    index = find_inode(path);
    if(index == -1){
    	puts("open end abnormally");
    	return -ENOENT;
    }
    puts("hello open end normally");
    return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
	struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	int index = find_inode(path);
	puts("hello read");
	if(index == -1){
		puts("read error");
		return -ENOENT;
	}
	len = inodes[index].STAT.st_size;

	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, inodes[index].data + offset, size);
	} else
		size = 0;
	puts("hello read end normally");
	return size;
}

static int hello_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int index = find_inode(path);
	void* dt;
	puts("hello write");
	if(index == -1){
		puts("write error");
		return -ENOENT;
	}
	if(offset != 0)
		puts("yaho");
	if(size + offset > inodes[index].STAT.st_size)
	{
		dt = malloc(inodes[index].STAT.st_size + size);
		if( dt )
		{
			if(inodes[index].data)
			{
				memcpy(dt, inodes[index].data, inodes[index].STAT.st_size);
				free(inodes[index].data);
			}
			inodes[index].data = dt;
			inodes[index].STAT.st_size += size;
		}
		else
			return -EFBIG;
	}
	memcpy(inodes[index].data + offset, buf, size);
	puts("write end");

    return size;
} 

static int hello_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int index = find_empty_inode();
	time_t cr_time = time(NULL);
	puts("mknod");
	
	if(index == -1){
		puts("mk_inode end abnormally");
		return -1;
	} else {
		strcpy(inodes[index].name, path);
		inodes[index].STAT.st_mode = mode;
		inodes[index].STAT.st_nlink = 1;
		inodes[index].STAT.st_ino = index;
		inode_flag[inode_index] = 1;
		inodes[index].STAT.st_atime = cr_time;
		inodes[index].STAT.st_mtime = cr_time;
		inodes[index].STAT.st_ctime = cr_time;
		inodes[index].STAT.st_size = 0;

		printf("inode_index = %d, mode is %d\n", inode_index, mode);
		inode_index ++;
		puts("mk_inode end");
	}
	return 0;
}

static int hello_mkdir(const char *path, mode_t mode)
{
	int index = find_empty_inode();
	time_t cr_time = time(NULL);
	puts("mkdir");
	
	if(index == -1){
		puts("mk_dir end abnormally");
		return -1;
	} else {
		strcpy(inodes[index].name, path);
		inodes[index].STAT.st_mode = mode | S_IFDIR;
		inodes[index].STAT.st_nlink = 2;
		inodes[index].STAT.st_ino = index;
		inode_flag[inode_index] = 1;
		inodes[index].STAT.st_atime = cr_time;
		inodes[index].STAT.st_mtime = cr_time;
		inodes[index].STAT.st_ctime = cr_time;
		inodes[index].STAT.st_size = 0;

		printf("inode_index = %d, mode is %d\n", inode_index, mode);
		inode_index ++;
		puts("mk_dir end");
	}

	return 0;
}

static int hello_utimens(const char *path, const struct timespec ts[2])
{
	int index = find_inode(path);
    puts("utimens");
    if(index == -1){
    	puts("utimes end abnormally");
    	return -errno;
    }
    inodes[index].STAT.st_mtime = time(NULL);
	puts("utimens end normally");
	return 0;
}

static int hello_unlink(const char* path)
{
	int index = find_inode(path);
	puts("hello unlink");
	if (index == -1)
		return -ENOENT;
  	else if(index == 0 )
  		return -EISDIR;
  	else{
  		puts("unlink ing");
  		inode_flag[index] = 0;
  		strcpy(inodes[index].name, "");
  		free(inodes[index].data);

  		memset(&inodes[index].STAT, 0, sizeof(struct stat));
  	}

   return 0;
}

static struct fuse_operations hello_oper = { 
	.getattr = hello_getattr,
	.readdir = hello_readdir, 
	.open = hello_open, 
	.read = hello_read, 
	.write = hello_write,
	.mknod = hello_mknod,
	.mkdir = hello_mkdir,
	.unlink = hello_unlink,
	.utimens = hello_utimens,
};

int main(int argc, char *argv[])
{
	init_inode();
	return fuse_main(argc, argv, &hello_oper, NULL);
}
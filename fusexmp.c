/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusexmp.c `pkg-config fuse --cflags --libs` -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>
#include <stdlib.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

typedef struct node{
	char* name;
	char* type;				// dir -> directory ; reg -> Regular file
	//void* data;				// No data for dir
	//size_t file_size;		// 0 for Directory
	int n_child;
	//struct node* parent;	// Pointer to the parent
	struct node* child;		// Pointer to the first child
	struct node* next;		// Pointer to sibling
}node; 

typedef struct qnode{
	node* fsnode;
	void* qdata;
	struct qnode* next;
}qnode;

typedef struct queue
{
	qnode* head;
	qnode* tail;
	int cnt;
}queue; 

char* rootpath;
node* root;
char* imagename = "img2";
queue* traverse;
queue* printqueue;
queue* readqueue;
queue* expandqueue;

node* createnode()
{
	node* newnode;
	newnode = malloc(sizeof(node));
	newnode->child = NULL;
	newnode->next = NULL;
	//newnode->file_size = 0;
	//newnode->data = malloc(1);
	newnode->n_child = 0;
	return newnode;
}

/*********Queue functions********/


void initqueue(queue* q)
{
	q->head = NULL;
	q->tail = NULL;
	q->cnt = 0;
}

void enqueue(queue* q,node* fsdata)
{
	//	printf("\n In enqueue");
	qnode* newqnode;
	newqnode = malloc(sizeof(qnode));
	newqnode->fsnode = fsdata;
	newqnode->next = NULL;
	
	if(NULL == q->head)
	{
		q->head = newqnode;
		q->tail = newqnode;
		q->cnt++;
	}
	
	else
	{
		q->tail->next = newqnode;
		q->tail = q->tail->next;
		q->cnt++;
	}
}

int isempty(queue* q)
{
	 if(q->head==NULL)
		return 1;
	return 0;
}

void dequeue(queue* q)
{
	qnode* temp;
	if(q->head == NULL)
		return;
		
	if(q->head->next == NULL)
	{
		temp = q->head;
		q->head =NULL;
		q->tail = NULL;
		q->cnt--;
		free(temp);
	}
	else
	{
		temp = q->head;
		q->head = q->head->next;
		q->cnt--;
		free(temp);
	}
	return;
}

void print_queue(queue* q)
{
	qnode* temp;
	temp = q->head;
	printf("\n");
	while(temp != NULL)
	{
		printf("%s->",temp->fsnode->name);
		temp = temp->next;
	}
	printf("NULL\n");
}

void createqueue()
{
	traverse = malloc(sizeof(queue));
	initqueue(traverse);
	enqueue(traverse,root);
	
	printqueue = malloc(sizeof(queue));
	initqueue(printqueue);
	
	//printf("\n\n Head name : %s",traverse->head->fsnode->name);
	
	node* itr;
	while(!isempty(traverse))
	{
		itr = traverse->head->fsnode;
		enqueue(printqueue,itr);
		dequeue(traverse);
		
		itr = itr->child;
		while(itr)
		{
			enqueue(traverse,itr);
			itr = itr->next;
		}
	}
	print_queue(printqueue);

}

void writefile()
{
	printf("\n In writefile");
	int name_len,type_len,data_len;
	int yes = 1, no = 0;
    FILE* fp;
	fp = fopen(imagename,"w");
	if(fp == NULL)
		printf("Error: File open error ");
		
	createqueue();
	//Write the number of nodes in the file
	int cnt1 = printqueue->cnt;
	fwrite(&cnt1,sizeof(int),1,fp);
		
	//Write data to the file
	qnode* q_itr;
	node* itr;
	q_itr = printqueue->head;
	while(q_itr != NULL)
	{
		itr = q_itr->fsnode;
		
		//Name
		name_len = strlen(itr->name);
		fwrite(&name_len,sizeof(int),1,fp);
		fwrite (itr->name, name_len + 1, 1, fp);
		
		//Type
		type_len = strlen(itr->type);
		fwrite(&type_len,sizeof(int),1,fp);
		fwrite (itr->type, type_len + 1, 1, fp);
		
		//Data
		data_len = strlen("data");
		fwrite(&data_len,sizeof(int),1,fp);
		fwrite ("data", data_len+1, 1, fp);
		
		//Child
		if(itr->child == NULL)
			fwrite(&no,sizeof(int),1,fp);
		else
			fwrite(&yes,sizeof(int),1,fp);
		
		//Next
		if(itr->next == NULL)
			fwrite(&no,sizeof(int),1,fp);
		else
			fwrite(&yes,sizeof(int),1,fp);
			
		//Number of children
		fwrite(&itr->n_child,sizeof(int),1,fp);
		
		q_itr = q_itr->next;
	}
	fclose(fp);
}

/*********Node functions*********/

void printlist(node* root)
{
	if(root->child != NULL)
	{
		printf("Files in the current directory :\n");
		
		node* itr;
		itr = root->child;
		
		while(itr != NULL)
		{
			printf("%s \t",itr->name);
			itr = itr->next;
		}
	}
	printf("\n");
	return;
}


static int addchild(node* parent,char* fname,char* type)
{
	printf("\n Adding node %s\n",fname);
	node* child;
	
		
	child = createnode(); 
	child->name = strdup(fname);
	child->type = strdup(type);
	//if(!strcmp(child->type,"dir"))
		//child->data = strdup(type);
	
	if(parent->child == NULL)
	{
		parent->child = child;
		printf("\n Added node %s\n",fname);
		parent->n_child++;
		return 0;
	}
	else
	{
		node* itr;
		itr = parent->child;
		while(itr->next != NULL)
		{
			itr = itr->next;
		}
		itr->next = child;
		parent->n_child++;
	}
	
	printf("\n Added node %s\n",fname);
	return 0;
}

node* lookup(char *path)
{
	node *child,*itr;
	
	
	if (strcmp(path, "/") == 0)
		return root;
	
	child = root->child;
	itr = child;
	
	//char *searchpath,*ptr;
	char *next;
	char *temp;
	char p[100];
	//int i=0;
	int len1,len2,n;
	
	//searchpath = "";
	
	next = path;
	
	while(next != NULL)
	{
		
		temp = strdup(next);
		next = strpbrk(temp + 1, "\\/");
		printf("temp : %s\n",temp);
		printf("next : %s\n",next);
		if(next!= NULL)
		{
			len1 = strlen(temp);
			len2 = strlen(next);
			n = len1 - len2;
			strncpy(p,temp,n);
			p[n] = '\0';
			printf("p : %s\n",p);	
			while(itr != NULL)
			{	
				if(strstr(itr->name,p))
				{	
					itr = itr->child;
					break;
				}
				itr = itr->next;
			}			
		}
		else
		{
			strcpy(p,temp);		
			while(itr != NULL)
			{	
				if(strstr(itr->name,p))
				{	
					return itr;
				}
				itr = itr->next;
			}		
		}
		
	}
	return NULL;
}

//Part of code taken from stack overflow(Citation)
void split_path_file(char** p,char *pf) {
    char *slash = pf, *next;
    while ((next = strpbrk(slash + 1, "\\/"))) 
   		slash = next;
    *p = strndup(pf, slash - pf );
    int len = strlen(*p);
	if(len == 0)
		*p = "/";
}

//Get the name of the file and strip the path
void get_file_name(char **f,char *pf) {
    char *slash = pf, *next;
    while ((next = strpbrk(slash + 1, "\\/"))) 
    {
   		slash = next;
		
	}
	*f = strdup(slash);
}

/***********Functions to update the structure*********/
static void removedir(char *path)
{
	node* parent,*child,*itr;
	char *parent_name;
	split_path_file(&parent_name,(char*)path);
	
	child = lookup((char*)path);
	
	printf("\n Trying to delete\n");
		
	parent = lookup(parent_name);
	
	//First node in the subdirectory list
	if(parent->child == child)
	{
		parent->child = child->next;
		free(child);
		parent->n_child--;
	}
	else
	{
		itr = parent->child;
		while(itr->next != NULL)
		{
			if(itr->next == child)
			{
				itr->next = child->next;
				free(child);
				return;
			}
			itr = itr->next;
		}
		parent->n_child--;
	}
	
}

void unlink1(const char *path)
{
	node* parent,*child,*itr;
	char *parent_name;
	split_path_file(&parent_name,(char*)path);
	
	child = lookup((char*)path);
			
	printf("\n Trying to delete\n");
		
	parent = lookup(parent_name);
	
	//First node in the subdirectory list
	if(parent->child == child)
	{
		parent->child = child->next;
		free(child);
		parent->n_child--;
	}
	else
	{
		itr = parent->child;
		while(itr->next != NULL)
		{
			if(itr->next == child)
			{
				itr->next = child->next;
				free(child);
				return;
			}
			itr = itr->next;
		}
		parent->n_child--;
	}
	
}
/***********FUSE functions**************/

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	printf("\n\n\n Current root path = %s \n\n\n",rootpath);
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	char* subpath;
	subpath = strstr(path,rootpath);
	
	if(subpath == NULL)
		return 0;
	else if(subpath == path)
	{
		char* newpath;
		newpath = strdup(path);
		int len = strlen(rootpath);
		newpath = newpath + len;
		printf("\n New path = %s\n",newpath);
				
		node* parent;
		char *parent_name;
		split_path_file(&parent_name,newpath);
		parent = lookup(parent_name);
		//printf("\n In mkdir parent : %s \n",parent->name);
		//printf("\n Parent address : %d",(int*)parent);
		addchild(parent,newpath,"reg");
		writefile();
	}
	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	res = mkdir(path, mode);
	if (res == -1)
		return -errno;
		
	char* subpath;
	subpath = strstr(path,rootpath);
	
	if(subpath == NULL)
		return 0;
	else if(subpath == path)
	{
		char* newpath;
		newpath = strdup(path);
		int len = strlen(rootpath);
		newpath = newpath + len;
		printf("\n New path = %s\n",newpath);
				
		node* parent;
		char *parent_name;
		split_path_file(&parent_name,newpath);
		parent = lookup(parent_name);
		//printf("\n In mkdir parent : %s \n",parent->name);
		//printf("\n Parent address : %d",(int*)parent);
		addchild(parent,newpath,"dir");
		writefile();
	}
	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;
		
	char* subpath;
	subpath = strstr(path,rootpath);
	
	if(subpath == NULL)
		return 0;
	else if(subpath == path)
	{
		char* newpath;
		newpath = strdup(path);
		int len = strlen(rootpath);
		newpath = newpath + len;
		unlink1(newpath);
	}

	
	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;
		
	char* subpath;
	subpath = strstr(path,rootpath);
	
	if(subpath == NULL)
		return 0;
	else if(subpath == path)
	{
		char* newpath;
		newpath = strdup(path);
		int len = strlen(rootpath);
		newpath = newpath + len;
		removedir(newpath);
	}

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;
	
	char* subpath;
	subpath = strstr(from,rootpath);
	
	if(subpath == NULL)
		return 0;
	else if(subpath == from)
	{
		char* newpath;
		newpath = strdup(from);
		int len = strlen(rootpath);
		newpath = newpath + len;
		printf("\n New path = %s\n",newpath);
				
		node* curr_file;
		curr_file = lookup(newpath);

		if(curr_file == NULL)
			return -ENOENT;
		
		char* topath;
		topath = strstr(from,rootpath);
	
		if(topath == to)
		{
			char* newpath2;
			newpath2 = strdup(to);
			int len = strlen(rootpath);
			newpath2 = newpath2 + len;
			printf("\n New path = %s\n",newpath2);
			curr_file->name = strdup(newpath2);
		}
	}
	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}


void xmp_destroy(void *userdata)
{
    printf("\n In destroy\n");
	int name_len,type_len,data_len;
	int yes = 1, no = 0;
    FILE* fp;
	fp = fopen(imagename,"w");
	if(fp == NULL)
		printf("Error: File open error ");
		
	createqueue();
	//Write the number of nodes in the file
	int cnt1 = printqueue->cnt;
	fwrite(&cnt1,sizeof(int),1,fp);
		
	//Write data to the file
	qnode* q_itr;
	node* itr;
	q_itr = printqueue->head;
	while(q_itr != NULL)
	{
		itr = q_itr->fsnode;
		
		//Name
		name_len = strlen(itr->name);
		fwrite(&name_len,sizeof(int),1,fp);
		fwrite (itr->name, name_len + 1, 1, fp);
		
		//Type
		type_len = strlen(itr->type);
		fwrite(&type_len,sizeof(int),1,fp);
		fwrite (itr->type, type_len + 1, 1, fp);
		
		//Data
		data_len = strlen("data");
		fwrite(&data_len,sizeof(int),1,fp);
		fwrite ("data", data_len+1, 1, fp);
		
		//Child
		if(itr->child == NULL)
			fwrite(&no,sizeof(int),1,fp);
		else
			fwrite(&yes,sizeof(int),1,fp);
		
		//Next
		if(itr->next == NULL)
			fwrite(&no,sizeof(int),1,fp);
		else
			fwrite(&yes,sizeof(int),1,fp);
			
		//Number of children
		fwrite(&itr->n_child,sizeof(int),1,fp);
		
		q_itr = q_itr->next;
	}
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.destroy	= xmp_destroy,
#ifdef HAVE_UTIMENSAT
	.utimens	= xmp_utimens,
#endif
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

void root_init()
{
	//Initialise the root
	root = createnode();
	root->name = "/";
	root->type = "dir";
	//root->data = "dir";
	root->next = NULL;
	root->child = NULL;
}

int main(int argc, char *argv[])
{
	umask(0);
	rootpath = "/home/sarvesh/dfs/fuseroot";
	root_init();
	
	/*strcat(rootpath,"/");
	if(argc == 2)
		strcat(rootpath,argv[1]);
	else
		strcat(rootpath,argv[2]);
	*/
	
	return fuse_main(argc, argv, &xmp_oper, NULL);
}

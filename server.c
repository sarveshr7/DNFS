/* FUSE ramdisk
 * Author: Sarvesh Rangnekar
 * Unity Id : sdrangne
 * gcc -Wall server.c `pkg-config fuse --cflags --libs` -o server
 */
 
 #define FUSE_USE_VERSION 26
 

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
/*
struct stat {
               dev_t     st_dev;      ID of device containing file 
               ino_t     st_ino;      inode number 
               mode_t    st_mode;     protection 
               nlink_t   st_nlink;    number of hard links
               uid_t     st_uid;      user ID of owner 
               gid_t     st_gid;      group ID of owner
               dev_t     st_rdev;     device ID (if special file)
               off_t     st_size;     total size, in bytes 
               blksize_t st_blksize;  blocksize for filesystem I/O 
               blkcnt_t  st_blocks;   number of 512B blocks allocated 
               time_t    st_atime;    time of last access 
               time_t    st_mtime;    time of last modification 
               time_t    st_ctime;    time of last status change 
           };*/
           
typedef struct node{
	char* name;
	char* type;				// dir -> directory ; reg -> Regular file
	void* data;				// No data for dir
	size_t file_size;		// 0 for Directory
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

node* root;
static size_t max_fs_size;
static size_t fs_size;
queue* traverse;
queue* printqueue;
queue* readqueue;
queue* expandqueue;
char* imagename;
int img_avl;
char* rootpath;

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


/************ FS Node functions**********/
node* createnode()
{
	node* newnode;
	newnode = malloc(sizeof(node));
	newnode->child = NULL;
	newnode->next = NULL;
	newnode->file_size = 0;
	newnode->data = malloc(1);
	newnode->n_child = 0;
	return newnode;
}

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
	fs_size = fs_size + sizeof(node);
	if(fs_size > max_fs_size)
		return -ENOMEM;
		
	child = createnode(); 
	child->name = strdup(fname);
	child->type = strdup(type);
	if(!strcmp(child->type,"dir"))
		child->data = strdup(type);
	
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

static void get_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, rootpath);
    strncat(fpath, path, PATH_MAX); 
}

static int ramdisk_getattr(const char *path, struct stat *stbuf)
{
	int retstat = 0;
	int res = 0;
	node* current;
	
	printf("\n\nIn getattr file name : %s\n\n",path);
	


	current = lookup((char*)path);
	if(current == NULL)
		return -ENOENT;
		
	char fpath[PATH_MAX];
	get_fullpath(fpath,path);
	retstat = lstat(fpath, stbuf);
	if(retstat == -1)
		return -ENOENT;
	
	/*printf("In getattr name : %s\n",current->name);
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(current->type, "dir") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} 
	else if (strcmp(current->type, "reg") == 0) {
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = current->file_size;
	}
	
	else
		res = -ENOENT;
	*/
	return retstat;
}

static int ramdisk_mkdir(const char* path,mode_t mode)
{
	int retstat;
	node* parent;
	char *parent_name;
	char fpath[PATH_MAX];
	get_fullpath(fpath,path);
	printf("Full Path name: %s ",fpath);
	split_path_file(&parent_name,(char*)path);
	
	parent = lookup(parent_name);
	
	//printf("\n In mkdir parent : %s \n",parent->name);
	//printf("\n Parent address : %d",(int*)parent);
	addchild(parent,(char*)path,"dir");
	retstat = mkdir(fpath, mode);
	//printf("\n\n");
	//printlist(parent);
	//printf("\n\n");
	
	/************Update Image****************/
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
		data_len = strlen(itr->data);
		fwrite(&data_len,sizeof(int),1,fp);
		fwrite (itr->data, data_len+1, 1, fp);
		
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
	return 0;
}

static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	
	char* fname,*fpath;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	node* curr_dir;
	curr_dir = lookup((char*)path);
	//filler(buf,"sarvesh",NULL,0);
	//if(root->child != NULL)
	//	filler(buf,(root->child)->name + 1,NULL,0);
	node* itr;
	
	//printf("\n In readdir path %s \n\n",path);
	
	itr = curr_dir->child;
	//printf("\n In readdir current dir %s \n\n",curr_dir->name);
	while(itr != NULL)
	{
		fpath = strdup(itr->name);
		get_file_name(&fname,fpath);
		filler(buf, fname+1, NULL, 0);
	//	printf("\n In readdir %s \n\n",fname);
		itr = itr->next;
		free(fname);
	}
	return 0;
}

static int ramdisk_rmdir(const char *path)
{
	node* parent,*child,*itr;
	char *parent_name;
	split_path_file(&parent_name,(char*)path);
	
	child = lookup((char*)path);
	
	//No such file or directory
	if(child == NULL)
		return -ENOENT;
	
	//Not a directory
	if(strcmp(child->type,"dir"))
		return -ENOTDIR;
		
	//Directory is not empty
	if(child->child != NULL)
		return -ENOTEMPTY;
		
	printf("\n Trying to delete\n");
		
	parent = lookup(parent_name);
	
	//First node in the subdirectory list
	if(parent->child == child)
	{
		parent->child = child->next;
		free(child);
		fs_size = fs_size - sizeof(node);
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
				fs_size = fs_size - sizeof(node);
				return 0;
			}
			itr = itr->next;
		}
		parent->n_child--;
	}
	
	return 0;
}

static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int fd;
	char fpath[PATH_MAX];
	get_fullpath(fpath,path);
	node* parent;
	char *parent_name;
	split_path_file(&parent_name,(char*)path);
	parent = lookup(parent_name);
	addchild(parent,(char*)path,"reg");
	fd = creat(path, mode);
	if (fd < 0)
		return -EBADF;
		
	fi->fh = fd;
	return 0;
}

static int ramdisk_open(const char *path, struct fuse_file_info *fi)
{
	int fd;
	node *file;
	char* pathname;
	char fpath[PATH_MAX];
	get_fullpath(fpath,path);
	
	printf("In open \n");
	printf("Full path : %s",fpath);


	pathname = strdup(path);
	file = lookup(pathname);
	if (file == NULL)
		return -ENOENT;
	fd = open(fpath, fi->flags);
    if (fd < 0)
		return -ENOENT;
    
    fi->fh = fd;
	return 0;
}

static int ramdisk_resize(const char *path, size_t new_size)
{
	node* curr_file;
			
	curr_file = lookup((char*)path);
	if(curr_file == NULL)
		return -ENOENT;
		
	void *new_buf;

	if (new_size == curr_file->file_size)
		return 0;

	new_buf = realloc(curr_file->data, new_size);
	if (!new_buf && new_size)
		return -ENOMEM;

	if (new_size > curr_file->file_size)
	{
		memset(new_buf + curr_file->file_size, 0, new_size - curr_file->file_size);
		fs_size = fs_size + new_size - curr_file->file_size;
		if(fs_size > max_fs_size)
			return -ENOMEM;
	}

	curr_file->data = new_buf;
	curr_file->file_size = new_size;

	return 0;
}

static int ramdisk_truncate(const char *path, off_t size)
{
	node* curr_file;
			
	curr_file = lookup((char*)path);
	if(curr_file == NULL)
		return -ENOENT;

	return ramdisk_resize(path, size);
}

/*
static int ramdisk_expand(const char *path,size_t new_size)
{
	node* curr_file;
			
	curr_file = lookup((char*)path);
	if(curr_file == NULL)
		return -ENOENT;
		
	if (new_size > curr_file->file_size)
		return ramdisk_resize(path, new_size);
	return 0;
}*/



static int ramdisk_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int write_bytes;
	node* curr_file;
			
	curr_file = lookup((char*)path);
	if(curr_file == NULL)
		return -ENOENT;
	write_bytes = pwrite(fi->fh, buf, size, offset);	
	return write_bytes;	
}



static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
	int read_bytes;
	node* curr_file;
	curr_file = lookup((char*)path);
	

	if(curr_file == NULL)
		return -ENOENT;
		
	read_bytes = pread(fi->fh, buf, size, offset);
	printf("\n\n\n Data Read \n %s",buf);
	
	return read_bytes;
}

int ramdisk_unlink(const char *path)
{
	node* parent,*child,*itr;
	char *parent_name;
	split_path_file(&parent_name,(char*)path);
	
	child = lookup((char*)path);
	
	//No such file or directory
	if(child == NULL)
		return -ENOENT;
		
	printf("\n Trying to delete\n");
		
	parent = lookup(parent_name);
	
	//First node in the subdirectory list
	if(parent->child == child)
	{
		fs_size = fs_size - sizeof(node);
		fs_size = fs_size - child->file_size;
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
				fs_size = fs_size - sizeof(node);
				fs_size = fs_size - child->file_size;
				itr->next = child->next;
				free(child);
				return 0;
			}
			itr = itr->next;
		}
		parent->n_child--;
	}
	
	return 0;
}

void ramdisk_destroy(void *userdata)
{
    printf("\n In destroy\n");
	/*int name_len,type_len,data_len;
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
		data_len = strlen(itr->data);
		fwrite(&data_len,sizeof(int),1,fp);
		fwrite (itr->data, data_len+1, 1, fp);
		
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
	}*/
}

static int ramdisk_rename(const char *from, const char *to)
{
	node* curr_file;
	curr_file = lookup((char*)from);

	if(curr_file == NULL)
		return -ENOENT;
	
	curr_file->name = strdup(to);

	return 0;
}



static struct fuse_operations ramdisk_oper = {
	.getattr	= ramdisk_getattr,
	.mkdir		= ramdisk_mkdir,
	.readdir 	= ramdisk_readdir,
	.rmdir		= ramdisk_rmdir,
	.truncate   = ramdisk_truncate,
	.create 	= ramdisk_create,
	.read  		= ramdisk_read,
	.write		= ramdisk_write,	
	.open 		= ramdisk_open,
	.unlink		= ramdisk_unlink,
	.rename		= ramdisk_rename,
	.destroy 	= ramdisk_destroy,
};

void addqchild(node* parent,node* child)
{
	if(parent->child == NULL)
	{
		parent->child = child;
	//	printf("\n Added node %s\n",child->name);
		return;
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
	}
	//printf("\n Added node %s\n",child->name);
}
int main(int argc, char *argv[])
{
	int size_mb;
	char* fuse_argv[3];
	char buf[PATH_MAX];
	
	//Get the current working directory
	rootpath = getcwd(buf,PATH_MAX);
	strcat(rootpath,"/");
	strcat(rootpath,argv[argc-3]);
	printf("\n Root directory path: %s",rootpath);
	
	if((argc == 4 || argc == 5) && !strcmp(argv[1],"-d"))
	{
		fuse_argv[2] = malloc(strlen(argv[2]));
		strcpy(fuse_argv[2],argv[2]);
		size_mb = atoi(argv[3]);
		if(argc == 5)
		{
			img_avl = 1;
			imagename = argv[4];
		}
	}
	else if(argc < 3 || argc > 4)
	{
		fprintf(stderr, "Usage: ramdisk <mount_point> <size> [<filename>]\n");
		exit(0);
	}
	else
	{		
		if(argc == 4)
		{
			img_avl = 1;
			imagename = argv[3];
		}
		size_mb = atoi(argv[2]);
		
	}
	
	fuse_argv[0] = malloc(strlen(argv[0]));
	fuse_argv[1] = malloc(strlen(argv[1]));
	
	strcpy(fuse_argv[0],argv[0]);
	strcpy(fuse_argv[1],argv[1]);
	
	max_fs_size = size_mb * 1024 * 1024;
	fs_size = 0;
	
	//Initialise the root
	/*root = createnode();
	root->name = "/";
	root->type = "dir";
	root->data = "dir";
	root->next = NULL;
	root->child = NULL;
	*/
	/**********************Extra Credit*************************/
	int node_cnt,i=0;
	int len,tlen,dlen,haschild,hasnext;
	
	//node* t1;
	char name[100],type[20],*data;
	//t1 = malloc(sizeof(node));
	
	FILE* image;
	image = fopen(imagename,"r");
	if(image == NULL)
	{
		//printf("No such file");
		//Initialise the root
		root = createnode();
		root->name = "/";
		root->type = "dir";
		root->data = "dir";
		root->next = NULL;
		root->child = NULL;
	}
	else
	{	
		readqueue = malloc(sizeof(queue));
		initqueue(readqueue);
		fread(&node_cnt,sizeof(int),1,image);
		while(i < node_cnt)
		{
			node* t1;
			t1 = createnode();
			fread(&len,sizeof(int),1,image);
			fread (name, len + 1, 1, image);
			t1->name = strdup(name);
		//	printf("Name length %d",len);	
		//	printf("\n File name : %s\n ",t1->name);
			
			fread (&tlen,sizeof(int),1,image);
			fread (type, tlen + 1, 1, image);	
			t1->type = strdup(type);
		//	printf("\n File type len: %d\n ",tlen);
		//	printf("\n File type : %s\n ",t1->type);
			
			fread (&dlen,sizeof(int),1,image);
		//	printf("Data length %d",dlen);
			data = malloc(dlen);
			fread (data, dlen + 1, 1, image);	
			t1->data = malloc(dlen);
			//printf("\n\n Data read %s \n\n",data);
			memcpy(t1->data, data, dlen);
			t1->file_size = dlen;
			//t1->data = strdup(data);
		//	printf("\n File data len: %d\n ",dlen);
		//	printf("\n File data : %s\n ",(char*)t1->data);
			
			fread(&haschild,sizeof(int),1,image);
			fread(&hasnext,sizeof(int),1,image);
			/*if(haschild)
				printf("Has a child\n");
			else
				printf("No child\n");
			if(hasnext)
				printf("Has a next\n");
			else
				printf("No next\n");
			*/
			fread(&t1->n_child,sizeof(int),1,image);
			//printf("Number of children : %d\n",t1->n_child);
				
			enqueue(readqueue,t1);
			i++;
		}
		//print_queue(readqueue);
		//char ch[1];
		//scanf("%s",ch);
		
		expandqueue = malloc(sizeof(queue));
		initqueue(expandqueue);
		root = readqueue->head->fsnode;
		enqueue(expandqueue,root);
		dequeue(readqueue);
		
		node* expand;
		node* child;
		
		while(!isempty(readqueue))
		{
			expand = expandqueue->head->fsnode;
			int n = 0;
			while(n < expand->n_child)
			{
				child = readqueue->head->fsnode;
				/*printf("\n number of children : %d",expand->n_child);
				printf("\n n = %d",n);
				printf("\n Parent : %s",expand->name);
				printf("\n Child : %s\n",child->name);*/
				addqchild(expand,child);
				enqueue(expandqueue,child);
				dequeue(readqueue);
				n++;
			}
			dequeue(expandqueue);
		}
		
	}
	
	if(argc == 4)
	{	
		return fuse_main(argc-2, fuse_argv, &ramdisk_oper, NULL);
	}
	else if(argc == 5)
	{
		return fuse_main(argc-2, fuse_argv, &ramdisk_oper, NULL);
	}
	else
	{
			return fuse_main(argc-1, fuse_argv, &ramdisk_oper, NULL);
	}
}




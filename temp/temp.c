static int ramdisk_mkdir(const char* path,mode_t mode)
{
	int retstat;
	node* parent;
	char *parent_name;
	char fpath[PATH_MAX];
	get_fullpath(fpath,path);

	split_path_file(&parent_name,(char*)path);
	
	parent = lookup(parent_name);
	
	//printf("\n In mkdir parent : %s \n",parent->name);
	//printf("\n Parent address : %d",(int*)parent);
	addchild(parent,(char*)path,"dir");
	  retstat = mkdir(fpath, mode);
    if (retstat < 0)
		retstat = -ENOMEM;
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
	return retstat;
}

static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int fd;
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

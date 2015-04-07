#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

void root_init()
{
	//Initialise the root
	root = createnode();
	root->name = "/";
	root->type = "dir";
	root->data = "dir";
	root->next = NULL;
	root->child = NULL;
}


void writefile()
{
	printf("\n In writefile");
	int name_len,type_len,data_len;
	int yes = 1, no = 0;
    FILE* fp;
	fp = fopen("unified","w");
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

void update_nodes(node* parent,node* currparent)
{
	int exists = 0, i= 0;
	node* itr;
	node* currnode = currparent->child;
	//Check if the same path exists already
	while(i < currparent->n_child)
	{	
		exists = 0;
		itr = parent->child;
		while(itr->next != NULL)
		{
			if(!strcmp(currnode->name,itr->name))
			{	
				exists = 1;
				update_nodes(itr,currnode);
			}
			itr = itr->next;
		}
		if(exists == 0)
		{
			node* newnode; 
			newnode = createnode();
			newnode->name = strdup(currnode->name);
			newnode->type = strdup(currnode->type);
			newnode->n_child = currnode->n_child;
			newnode->data = strdup(currnode->data);
			newnode->child = currnode->child;
			newnode->next = NULL;
			
			itr->next = newnode;
			parent->n_child++;
		}
		currnode = currnode->next;
		i++;
	}
}

void build_image()
{
	int node_cnt,i=0;
	int len,tlen,dlen,haschild,hasnext;
	
	//node* t1;
	char name[100],type[20],*data;
	//t1 = malloc(sizeof(node));		
	
	char* imagename1 = "img1";
	FILE* image;
	image = fopen(imagename1,"r");
	
	
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
		
		printf("\n Build Image Complete");
	
}

void update_image(char* imagename)
{
	int node_cnt,i=0;
	int len,tlen,dlen,haschild,hasnext;
	
	node* temproot;
	char name[100],type[20],*data;
	//t1 = malloc(sizeof(node));		
	
	//char* imagename1 = "image1";
	FILE* image;
	image = fopen(imagename,"r");
	
	
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
		temproot = readqueue->head->fsnode;
		enqueue(expandqueue,temproot);
		dequeue(readqueue);
		
		node* expand;
		node* child;

		node* itr;
	
	
		
		while(!isempty(readqueue))
		{
			expand = expandqueue->head->fsnode;
			int n = 0;
			i = 0;
			
			while(n < expand->n_child)
			{
				child = readqueue->head->fsnode;
				addqchild(expand,child);
				enqueue(expandqueue,child);
				dequeue(readqueue);
				n++;
			}
			dequeue(expandqueue);
		}
		
		update_nodes(root,temproot);
		printf("\n Update image complete");
}

int main()
{
	build_image();
	update_image("img2");
	update_image("img3");
	writefile();
}

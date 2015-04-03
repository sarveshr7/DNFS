 #include <sys/stat.h>
 #include <sys/types.h>
#include <ctype.h>
#include <dirent.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc,char* argv[])
{
	char* path1,*path2,*sub;
	path1 = malloc(PATH_MAX);
	path2 = malloc(PATH_MAX);
	printf("\n Enter Path1 : ");
	scanf("%s",path1);
	printf("\n Enter Path2 : ");
	scanf("%s",path2);
	sub = strstr(path1,path2);
	if(sub == NULL)
		printf("Not found");
	else if(sub == path1)
	{
		char* newpath;
		int len = strlen(path2);
		printf("Found at the start");
		
		if(!strcmp(path1,path2))
			newpath = "/";
		else
			newpath = path1 + len;
		printf("\n New path = %s\n",newpath);
	}
		
	else
		printf("Not at the beginning");
	
    return 0;
}


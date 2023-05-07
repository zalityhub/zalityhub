#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define MaxStackSize 1024
int		StackPtr = MaxStackSize;
char	*Stack[MaxStackSize+2];


void
PushStack(char *text)
{
	Stack[--StackPtr] = text;
}


char*
PopStack()
{
	if ( StackPtr >= MaxStackSize )
		return NULL;		// empty
	return Stack[StackPtr++];
}


void
InsertRecord(char *pname)
{
	if ( StackPtr >= MaxStackSize )
		return;		// empty

	char *top = strdup(Stack[MaxStackSize-1]);
	char *ptr = strchr(top, ' ');
	*ptr = '\0';
	for(char *tmp; (tmp = strchr(top, ',')) != NULL; )
		strcpy(tmp, tmp+1);
	int size = atoi(top);
	ptr = strstr(ptr+1, " record ");
	*(strchr(ptr+8, ' ')) = '\0';
	int blk = atoi(ptr+8);
	int n = (MaxStackSize - StackPtr);
	for ( char *text; (text = PopStack()) != NULL; )
	{
		printf("INSERT INTO leak VALUES('%s', %d, %d, %d, '%s');\n", pname, blk, --n, size, text);
		free(text);
	}
	free(top);
}


int
main(int argc, char *argv[])
{
	char	bfr[BUFSIZ];

	if ( argc < 2 )
	{
		fprintf(stderr, "Need process name\n");
		exit(1);
	}

	char *pname = argv[1];

	puts("DROP TABLE leak;");
	puts("CREATE TABLE leak (pid TEXT,blk INTEGER, seq INTEGER, size INTEGER, text TEXT);");

	while (fgets(bfr, sizeof(bfr), stdin) != NULL)
	{
		for(char *ptr; (ptr = strchr(bfr, '\r')) != NULL; )
			strcpy(ptr, ptr+1);
		for(char *ptr; (ptr = strchr(bfr, '\n')) != NULL; )
			strcpy(ptr, ptr+1);

		if ( strlen(bfr) <= 0 )
		{
			InsertRecord(pname);
		}
		else
		{
			if ( strstr(bfr, "bytes in ") != NULL && strstr(bfr, " record ") != NULL )
				StackPtr = MaxStackSize;
			PushStack(strdup(bfr));
		}
	}

	if ( StackPtr < MaxStackSize )
		InsertRecord(pname);
	return 0;
}

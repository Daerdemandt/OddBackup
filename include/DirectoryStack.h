#include <stdlib.h>
#include <stdio.h>

struct DirectoryStackStruct
{
struct DirectoryStackStruct * Previous;
struct DirectoryStackStruct * Next;
char * DirectoryName;
};
int StackDebugger = 0;
struct DirectoryStackStruct * DirectoryStackTop = NULL;

void PrintAll()
{
struct DirectoryStackStruct * CurrentStackEntry = DirectoryStackTop;
while (CurrentStackEntry)
	{
	printf("%s\n", CurrentStackEntry->DirectoryName);
	CurrentStackEntry = CurrentStackEntry->Previous;
	}
}

void PushDir(char * DirectoryName)
{
Dprintf("PD:%s is pushed to stack. %i\n",DirectoryName,++StackDebugger);
if (DirectoryStackTop)
	{
	DirectoryStackTop->Next = (struct DirectoryStackStruct *)malloc(sizeof(struct DirectoryStackStruct));
	if (!(DirectoryStackTop->Next))
		{
		printf("Memory allocation error.\n");
		}
	DirectoryStackTop->Next->Previous = DirectoryStackTop;
	DirectoryStackTop = DirectoryStackTop->Next;
	DirectoryStackTop->DirectoryName = (char*)malloc(sizeof(char)*(1 + strlen(DirectoryName)));
	sprintf(DirectoryStackTop->DirectoryName, "%s", DirectoryName);
	DirectoryStackTop->Next = NULL;
	}
else//stack is empty, initialize
	{
	DirectoryStackTop = (struct DirectoryStackStruct *)malloc(sizeof(struct DirectoryStackStruct));
	if (!DirectoryStackTop)
		{
		printf("Memory allocation error.\n");
		}
	DirectoryStackTop->Next = NULL;
	DirectoryStackTop->Previous = NULL;
	DirectoryStackTop->DirectoryName = (char*)malloc(sizeof(char)*strlen(DirectoryName));
	sprintf(DirectoryStackTop->DirectoryName, "%s", DirectoryName);
	}
}

char * PopDir()
{
char * Result;
if(DirectoryStackTop)
	if (DirectoryStackTop->Previous == NULL)//it's the last directory in stack!
		{
		Result = DirectoryStackTop->DirectoryName;
		free(DirectoryStackTop);
		DirectoryStackTop = NULL;
		}
	else
		{
		Result = DirectoryStackTop->DirectoryName;
		DirectoryStackTop = DirectoryStackTop->Previous;
		free(DirectoryStackTop->Next);
		DirectoryStackTop->Next = NULL;
		}
else//stack is empty
	{
	Result = NULL;
	}
Dprintf("PD:%s is popped from stack. %i\n",Result,--StackDebugger);
return Result;
}

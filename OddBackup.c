/*
 * OddBackup, the backuping tool.
 * Version: 1.0
 * Written by Roman A. Kuzmichev (daerdemandt@gmail.com).
 * This software is distributed under terms and conditions of GNU LGPL.
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * ORIGINAL AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define VERSION "1.0"

#define MAX_INPUT_ARGS 4

#define CATCH_ERROR(...)	\
if (errno)			\
	{			\
	perror(__VA_ARGS__);	\
	errno = 0;		\
	}

//#define DEBUG

#ifdef DEBUG
#define Dprintf(...) printf(__VA_ARGS__)
#endif
#ifndef DEBUG
#define Dprintf(...)			//wipe out all Dprintf's
#endif

#define Vprintf(...)		\
if (Verbose)			\
	{			\
	printf(__VA_ARGS__);	\
	}


void PrintHelp();
void PrintVersion();
void GlobaliseInput();
char * AbsoluteSourceName(char *);//simply concatenates ProcessingDirectoryName, "/" and a short name
char * AbsoluteDestinationName(char *);
char * FixFilename(char *);
#include "rights.h"
bool IsToBeBackuped(char *);//checks permissions and timestamps
bool TryDirectory(char *);//TRUE if directory is suitable
bool SourceIncludesDestination();
void BackupAndGzip(char *);
bool DotsCheck(char *);//TRUE for "*/." and "*/.."
void ParseArguments(int, char**);
bool InjectionCheck(char*);

char * PopDir();
void PushDir(char *);
#include "DirectoryStack.h"

char * SourceDirectoryName = NULL;//absolute path, not ended with '/'
char * DestinationDirectoryName = NULL;//absolute path, not ended with '/'
char * ProcessingDirectoryName = NULL;//absolute path, not ended with '/'

char Suspicion = 0;//1 - paranoidal, 0 - normal, 3 - careless, used in InjectionCheck()
bool Verbose = 0;
bool FixFlag = 1; //if it is set, program will try to fix suspicious filenames instead of skipping 'em



int main(int argc, char *argv[])
{
ParseArguments(argc, argv);//incorrect arguments? print help & exit. Also, fill in SourceDirectoryName and DestinationDirectoryName; process flags.
Vprintf("OddBackup, version %s.\nBackup '%s' to '%s'.\n", VERSION, SourceDirectoryName, DestinationDirectoryName);
switch (Suspicion)
	{
	case 1: Vprintf("All attempts of shell-injections will be eliminated!\n");break;
	case 0: Vprintf("You'll be asked about fixing suspicious filenames.\n");break;
	case -1: Vprintf("You won't be disturbed with suspicious filenames. Enjoy your day!\n");break;
	}
struct dirent * DirectoryEntryPointer;
DIR * ProcessingDirPointer;

PushDir(SourceDirectoryName);

while (ProcessingDirectoryName = PopDir())
	{
	if (DotsCheck(ProcessingDirectoryName))//eliminate "." and ".." entries
		{
		continue;
		}
	if (!TryDirectory(ProcessingDirectoryName))
		{
		printf("Can't backup %s.\n", ProcessingDirectoryName);
		continue;
		}
	Dprintf("Processing %s\n", ProcessingDirectoryName);
	Vprintf("Entering %s\n", ProcessingDirectoryName);
	ProcessingDirPointer = opendir(ProcessingDirectoryName);
	CATCH_ERROR("Can't open directory");
	while ((DirectoryEntryPointer = readdir(ProcessingDirPointer)) != NULL)
		{
		switch(DirectoryEntryPointer->d_type)
			{
			case DT_DIR :	PushDir(AbsoluteSourceName(DirectoryEntryPointer->d_name)); break;//it's a directory - will be processed too
			case DT_REG :	if (IsToBeBackuped(DirectoryEntryPointer->d_name))//it's a file
						{
						BackupAndGzip(DirectoryEntryPointer->d_name);
						};
					break;
			default : printf("%s is neither file nor directory - can't backup.\n", DirectoryEntryPointer->d_name);
			}
		}
	Vprintf("Leaving %s\n", ProcessingDirectoryName);
	closedir(ProcessingDirPointer);
	free(ProcessingDirectoryName);
	}

return 0;
}



char * AbsoluteSourceName(char * ShortName)
{
static int BufferSize = 100;
static char * Result = (char*)malloc(sizeof(char)*BufferSize);

while (BufferSize < (strlen(ShortName) + strlen(ProcessingDirectoryName)))//enlarge your buffer if neccessairy
	{
	Dprintf("%i < %i\n", BufferSize, (strlen(ShortName) + strlen(ProcessingDirectoryName)));
	BufferSize *= 2;
	free(Result);
	Result = (char*)malloc(sizeof(char)*BufferSize);
	}
sprintf(Result, "%s/%s", ProcessingDirectoryName, ShortName);
Dprintf("ASN:%s\n", Result);
return Result;
}

char * AbsoluteDestinationName(char * ShortName)
{
static int BufferSize = 100;
static char * Result = (char*)malloc(sizeof(char)*BufferSize);

while (BufferSize < (strlen(ShortName) + strlen(ProcessingDirectoryName)))//enlarge your buffer if neccessairy
	{
	Dprintf("%i < %i\n", BufferSize, (strlen(ShortName) + strlen(ProcessingDirectoryName)));
	BufferSize *= 2;
	free(Result);
	Result = (char*)malloc(sizeof(char)*BufferSize);
	}
int Shift = strlen(SourceDirectoryName);
sprintf(Result, "%s%s/%s", DestinationDirectoryName, ProcessingDirectoryName + Shift, ShortName);
Dprintf("ADN:%s\n", Result);
return Result;
}

char * FixFilename(char * Filename)
{
static int BufferSize = 100;
static char * Result = (char*)malloc(sizeof(char)*BufferSize);
int Length = strlen(Filename);

while (BufferSize < (2 * Length))//enlarge your buffer if neccessairy
	{
	Dprintf("%i < %i\n", BufferSize, (2 * Length));
	BufferSize *= 2;
	free(Result);
	Result = (char*)malloc(sizeof(char)*BufferSize);
	}
int i,j = 0;
for (i = 0; i < Length; i++)
	{
	switch (Filename[i])
		{
		case '`' : Result[j] = '\\'; j++; //fill in the blacklist
		default : Result[j] = Filename[i]; j++; break;
		}
	}
Result[j] = '\0';

Dprintf("FF:%s\n", Result);
return Result;
}



bool DotsCheck(char * Path)
{
bool Result = 0;
int Length = strlen(Path);

if ((Path[Length-1] == '.') && (Path[Length-2] == '/'))
	{
	Result = 1;
	}
if ((Path[Length-1] == '.') && (Path[Length-2] == '.') && (Path[Length-3] == '/'))
	{
	Result = 1;
	}
return Result;
}

bool IsToBeBackuped(char * Filename)//checks permissions and timestamps
{
char * SourceFilename = AbsoluteSourceName(Filename);
char * DestinationFilename = AbsoluteDestinationName(Filename);

//check if we can read from the source file. If not - return 0
FILE * CanBeRead = fopen(SourceFilename,"r");
if (!CanBeRead)
	{
	return 0;
	}
fclose(CanBeRead);
//check if destination file exists. If not - return 1
int DestinationAccessable = open(SourceFilename, 0);
if (DestinationAccessable == -1)
	{
	if (errno == EACCES)
		{
		return 0;
		}
	else
		{
		return 1;
		}
	}
close(DestinationAccessable);
//check timestamps. If ok - return 1, else - 0.
static struct stat SourceStat;
static struct stat DestinationStat;
stat(AbsoluteSourceName(Filename), &SourceStat);
stat(AbsoluteDestinationName(Filename), &DestinationStat);
Dprintf("ITB:%i %i %i\n",SourceStat.st_mode, SourceStat.st_mtime, SourceStat.st_mtime);
if (SourceStat.st_mtime > DestinationStat.st_mtime)
	{
	return 1;
	}
else
	{
	return 0;
	}
}//end of IsToBeBackuped

bool TryDirectory(char * DirectoryName)//TRUE if directory is suitable
{
DIR * SourceCanBeOpened = opendir(DirectoryName);
bool Result = 0;

if (SourceCanBeOpened)
	{
	closedir(SourceCanBeOpened);
	//CATCH_ERROR("TryDirectory");
	char * DestinationDirectory = (char *)malloc(sizeof(char)*(3 + strlen(DestinationDirectoryName)+strlen(DirectoryName)));
	sprintf(DestinationDirectory, "%s%s", DestinationDirectoryName, ProcessingDirectoryName + strlen(SourceDirectoryName));
	Dprintf("TD: %s\n", DestinationDirectory);
	int DestinationExisted = mkdir(DestinationDirectory,UR|UW|UX|GR|GW|GX|OR|OW|OX);
	if(!DestinationExisted || errno==EEXIST)//needed folder exists now
		{
		errno = 0;
		struct stat DestinationStatus;
		stat(DestinationDirectory, &DestinationStatus);
		if (DestinationStatus.st_mode & (OW|UW|GW))
			{
			Result = 1;
			}
		else//noone can write there
			{
			printf("Not enough rights to write in %s.\n", DestinationDirectory);
			Result = 0;
			}
		}
	else//folder doesn't exist and we cannot create it
		{
		printf("Cannot create %s.\n", DestinationDirectory);
		Result = 0;
		}
	free(DestinationDirectory);
	}
else
	{
	Result = 0;
	}
return Result;
}

void BackupAndGzip(char* Filename)
{
Dprintf("BAZ is called\n");
char * Source =  AbsoluteSourceName(Filename);
char * Destination = AbsoluteDestinationName(Filename);
static int BufferSize = 200;
static char * CommandBuffer = (char*)malloc(sizeof(char)*BufferSize);

if(BufferSize < (25 + 2 * (strlen(Source) + strlen(Destination)))) //double size - in case we'll have to fix 'em
	{
	free(CommandBuffer);
	BufferSize*= 2;
	CommandBuffer = (char*)malloc(sizeof(char)*BufferSize);
	}

Vprintf("Copying & compressing '%s'\n", Source);
if (InjectionCheck(Source) && InjectionCheck(Destination))
	{
	sprintf(CommandBuffer, "cp -p -f '%s' '%s'", Source, Destination);//change this string carefully, buffer's not bottomless!
	Dprintf("BAZ:%s\n", CommandBuffer);
	system(CommandBuffer);
	//CATCH_ERROR("Copying file");

	sprintf(CommandBuffer, "gzip -q -f '%s'", Destination);
	Dprintf("BAZ:%s\n", CommandBuffer);
	system(CommandBuffer);
	//CATCH_ERROR("Compressing file");

	}
else
	if (FixFlag)
		{
		Source = FixFilename(Source);
		Destination = FixFilename(Destination);

		sprintf(CommandBuffer, "cp -p -f \"%s\" \"%s\"", Source, Destination);//change this string carefully, buffer's not bottomless!
		Dprintf("BAZ:%s\n", CommandBuffer);
		system(CommandBuffer);
		//CATCH_ERROR("Copying file");

		sprintf(CommandBuffer, "gzip -q -f \"%s\"", Destination);
		Dprintf("BAZ:%s\n", CommandBuffer);
		system(CommandBuffer);
		//CATCH_ERROR("Compressing file");
		}
	else	
		{
		Vprintf("Filename too suspicious. Sorry.\n");
		return;
		}
}

bool SourceIncludesDestination()
{
bool Result = 0;
if (strlen(SourceDirectoryName) <= strlen(DestinationDirectoryName))//they're alerady global
	{
	if (!strncmp(SourceDirectoryName, DestinationDirectoryName, strlen(SourceDirectoryName)))
		{
		Result = 1;
		}
	}
return Result;
}

void GlobaliseInput()
{
char * CallersDirectoryName = get_current_dir_name();
CATCH_ERROR("Can't get calling directory's name");
if (!CallersDirectoryName)
	{
	exit(-1);
	}
Dprintf("GI: Caller's Directory: %s\n",CallersDirectoryName);
if (SourceDirectoryName[0] != '/')//not global path
	{
	char * GlobalSourceDirectoryName = (char*)malloc(sizeof(char)*(strlen(CallersDirectoryName)+strlen(SourceDirectoryName)+3));
	if (SourceDirectoryName[0] == '.')
		{
		sprintf(GlobalSourceDirectoryName, "%s%s", CallersDirectoryName, SourceDirectoryName + 1);
		}
	else
		{
		sprintf(GlobalSourceDirectoryName, "%s/%s", CallersDirectoryName, SourceDirectoryName);
		}
	SourceDirectoryName = GlobalSourceDirectoryName;
	}
Dprintf("GI: Source Directory: %s\n", SourceDirectoryName);
if (DestinationDirectoryName[0] != '/')//not global path
	{
	char * GlobalDestinationDirectoryName = (char*)malloc(sizeof(char)*(strlen(CallersDirectoryName)+strlen(DestinationDirectoryName)+3));
	if (DestinationDirectoryName[0] == '.')
		{
		sprintf(GlobalDestinationDirectoryName, "%s%s", CallersDirectoryName, DestinationDirectoryName + 1);
		}
	else
		{
		sprintf(GlobalDestinationDirectoryName, "%s/%s", CallersDirectoryName, DestinationDirectoryName);
		}
	DestinationDirectoryName = GlobalDestinationDirectoryName;
	}
Dprintf("GI: Destination Directory: %s\n", DestinationDirectoryName);
free(CallersDirectoryName);
}

void PrintHelp()
{
printf("Usage: oddbackup [OPTIONS] source_folder_name destination_folder_name\n");
printf("Options are:\n");
printf("	-p --pedantic - Drop all filenames suspected in shell-injection.\n");
printf("	-n --normal - Ask user about suspicious filenames: drop them or try to fix?\n");
printf("	-c --careless - Try to fix all suspicious filenames.\n");
printf("	-v --version - Print program's version and quit.\n");
printf("	-h --help - Print this message and quit.\n");
printf("	-V --verbose - Verbose mode (program comments it's actions).\n");
}

void PrintVersion()
{
printf("OddBackup version %s\n", VERSION);
printf("Licensed under GNU LGPL.\n");
printf("Written by Roman A. Kuzmichev (daerdemandt@gmail.com).\n");
printf("This is free software: you are free to change and redistribute it with a link to the original author.\nThere is NO WARRANTY, to the extent permitted by law.\n");
}

void ParseArguments(int argc, char** argv)
{
char flags[MAX_INPUT_ARGS + 1] = {0,0,0,0};
if (argc > MAX_INPUT_ARGS + 1)
	{
	printf("%s:Too many arguments.", argv[0]);
	PrintHelp();
	exit(-1);
	}

for (int i = 1; i < argc; i++)
	{
	if (!strcmp(argv[i], "-p")||!strcmp(argv[i], "--paranoic"))
		{
		FixFlag = 0;
		Suspicion = 1;
		flags[i] = 1;
		}

	if (!strcmp(argv[i], "-c")||!strcmp(argv[i], "--careless"))
		{
		FixFlag = 1;
		if (getuid())
			{
			Suspicion = -1;
			}
		else
			{
			printf("root shall not be so careless. --normal is used instead.\n");
			Suspicion = 0;
			}
		flags[i] = 1;
		}

	if (!strcmp(argv[i], "-n")||!strcmp(argv[i], "--normal"))
		{
		FixFlag = 1;
		Suspicion = 0;
		flags[i] = 1;
		}

	if (!strcmp(argv[i], "-v")||!strcmp(argv[i], "--version"))
		{
		PrintVersion();
		exit(0);
		}

	if (!strcmp(argv[i], "-h")||!strcmp(argv[i], "--help"))
		{
		PrintHelp();
		exit(0);
		}

	if (!strcmp(argv[i], "-V")||!strcmp(argv[i], "--verbose"))
		{
		Verbose = 1;
		flags[i] = 1;
		}

	}//end of for-cycle
for (int i = 1; i < argc; i++)
	{
	if (!flags[i])
		{
		flags[0]++;
		if (!SourceDirectoryName)
			{
			SourceDirectoryName = argv[i];
			}
		else
			{
			DestinationDirectoryName = argv[i];
			}
		}
	}//we use flags[0] as a counter
if (flags[0]!=2)//source is 1 and destination is 1. if 1+1!=2 - we're messed
	{
	PrintHelp();
	exit(-1);
	}

GlobaliseInput();
if (SourceIncludesDestination())
	{
	printf("Error: source folder contains destination folder");
	exit(-1);
	}
}//end of ParseArguments

bool InjectionCheck(char * Filename)//1 is OK, 0 is not
{
int Length = strlen(Filename);
for (int i = 0; i < Length; i++)
	{
	if (Filename[i] == '\'')// there's an ' in the filename. It might be an attempt to get out of ''-sandbox
		{
		switch (Suspicion)
			{
			case -1: FixFlag = 1; break;
			case 0: {
				char UserResponse = 'n';
				printf("Filename '%s' looks suspicious, it may be an shell-injection. I can try to fix it, should I?[y/N] ", Filename);
				scanf("%c", &UserResponse);
				FixFlag = (UserResponse == 'y')||(UserResponse == 'Y');
				};				
				break;
			case 1: break;
			}
		return 0;
		}
	}
return 1;
}

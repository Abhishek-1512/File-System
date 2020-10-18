/* NAME: ABHISHEK JAIN
    ID: 1001759977
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define NUM_BLOCKS    4226      // Defining the number of blocks
#define BLOCK_SIZE    8192      // Defining the block size
#define NUM_FILES     128       // Defining the max number of files
#define MAX_FILE_SIZE 10240000  // ( blocks * blocksize)

extern int errno;

FILE * fd;
uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];

char * filename_close;          // To store the filename to close the file

struct Directory_Entry
{
	uint8_t valid;              // flag to check if directory is being used
	char    filename[32];
	uint32_t inode;

};

struct Inode
{
 uint8_t  valid;
 uint8_t  attributes;
 uint32_t size;
 uint32_t blocks[1250]; //pointers to the block- for each file block is around 10MB
};

//directory is the pointer which points to block zero
//declare pointer to the directory entry
struct Directory_Entry * dir;
struct Inode 		   * inodes;
uint8_t                * freeBlockList;
uint8_t                * freeInodeList;

void initializeBlockList()
{
int i;
for(i=0; i< NUM_BLOCKS; i++)
{
	freeBlockList[i] = 1;
}
}

void initializeInodes()
{
int i;
for(i=0; i< NUM_FILES; i++)
{
	inodes[i].valid      = 0;
	inodes[i].size       = 0;
	inodes[i].attributes = 0;

	int j;
	for (j=0; j< 1250; j++)
	{
		inodes[i].blocks[j] = -1;
	}
}
}


void initializeInodeList()
{
int i;
for(i=0; i< NUM_FILES; i++)
{
	freeInodeList[i] = 1;
}
}

void initializeDirectory()
{
int i;
for(i=0; i< NUM_FILES; i++)
{
	dir[i].valid =  0;
	dir[i].inode = -1;

	memset( dir[i].filename, 0,32);
}

}

int df()
{
int i;
int free_space=0;
for(i=0; i< NUM_BLOCKS; i++)
{
	if( freeBlockList[i] == 1)
	{
	  free_space = free_space + BLOCK_SIZE;
	}
}
return free_space;
}

int findfreeBlock()
{

	int i;
	int ret = -1;
	for( i=10; i< NUM_BLOCKS;  i++)
	{

	  if( freeBlockList[i] == 1 )
	  {

		  freeBlockList[i] = 0;
		  return i;
	  }

	}
    return ret;
}

int findFreeInode()
{

	int i;
	int ret = -1;
	for( i=10; i< NUM_FILES;  i++)
	{
	  if( inodes[i].valid == 0 )
	  {
		  inodes[i].valid =1;
		  return i;
	  }

	}
	return ret;
}

int findDirectoryEntry( char * filename)
{
	int i;
	for( i=0; i< NUM_FILES;  i++)
	{
	  if( strcmp(filename, dir[i].filename)== 0 && dir[i].valid == 1 )
	  {
		  return i;
	  }
	}

	for( i=0; i< NUM_FILES;  i++)
	{
	  if(dir[i].valid == 0 )
	  {
		  return i;
	  }
	}
	return -1;
}

int put(char *filename)
{
 struct stat buf;
 int ret;

 ret = stat( filename, &buf );

 if( ret == -1)
 {
	 printf("put error: File does not exist. \n");
	 return -1;
 }

 int size = buf.st_size;

 if( size > MAX_FILE_SIZE)
 {
	 printf("put error: File size is too big. \n");
	 return -1;

 }


  if( size > df())     // checking remaining space on disk
 {
	 printf("put error: Not enough disk space. \n");
	 return -1;

 }

 int directoryIndex = findDirectoryEntry(filename);
 int inodeIndex     = findFreeInode();

 if(inodeIndex == -1)
 {
     printf("Inode error \n");
     return -1;
 }

 dir[directoryIndex].inode = inodeIndex;
 strncpy( dir[directoryIndex].filename, filename, strlen( filename ));
 dir[directoryIndex].valid = 1;


 //*********************************************************************************
  //
  // The following chunk of code demonstrates similar functionality of your put command
  //

  int    status;                    // Hold the status of all return values.
 // struct stat buf;                 // stat struct to hold the returns from the stat call

  // Call stat with out input filename to verify that the file exists.  It will also
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't
  // care about anything but the filesize.
  status =  stat( filename, &buf );

  // If stat did not return -1 then we know the input file exists and we can use it.
  if( status != -1 )
  {

    // Open the input file read-only

	FILE * ip;
	ip = fopen ( filename, "r" );
    printf("Reading %d bytes from %s\n", (int) buf . st_size, filename );

    // Save off the size of the input file since we'll use it in a couple of places and
    // also initialize our index variables to zero.
    int copy_size   = buf . st_size;

    // We want to copy and write in chunks of BLOCK_SIZE. So to do this
    // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
    // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
    int offset      = 0;
    int block       = 0;

    // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big
    // memory pool. Why? We are simulating the way the file system stores file data in
    // blocks of space on the disk. block_index will keep us pointing to the area of
    // the area that we will read from or write to.

    // copy_size is initialized to the size of the input file so each loop iteration we
    // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
    // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
    // we have copied all the data from the input file.
    while( copy_size > 0 )
    {
      int block_index = findfreeBlock();

      inodes[inodeIndex].blocks[block] = block_index;
      inodes[inodeIndex].size  = buf.st_size;
      // Index into the input file by offset number of bytes.  Initially offset is set to
      // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We
      // then increase the offset by BLOCK_SIZE and continue the process.  This will
      // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
      fseek( ip, offset, SEEK_SET );

      // Read BLOCK_SIZE number of bytes from the input file and store them in our
      // data array.
      int bytes  = fread( blocks[block_index], BLOCK_SIZE, 1, ip );

	  //printf(" Error1 : %d", blocks[block_index]);
	  //printf(" Error2 : %d", BLOCK_SIZE);



	 // printf(" Error : %d",bytes);

	  //bool a;
	  //a = (bytes == 0 && !feof( fd ));

	 // printf("boolean value: %B\n", a );

      // If bytes == 0 and we haven't reached the end of the file then something is
      // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
      // It means we've reached the end of our input file.
      if( bytes == 0 && !feof( ip ) )
      {
        printf("An error occured reading from the input file.\n");
        return -1;
      }

      // Clear the EOF file flag.
      clearerr( ip );

      // Reduce copy_size by the BLOCK_SIZE bytes.
      copy_size -= BLOCK_SIZE;

      // Increase the offset into our input file by BLOCK_SIZE.  This will allow
      // the fseek at the top of the loop to position us to the correct spot.
      offset    += BLOCK_SIZE;
      block++;
	  //fd = fopen(filename, "w");
	  //fwrite( blocks[block_index], copy_size, BLOCK_SIZE, fd);
    }

    // We are done copying from the input file so close it out.
    fclose( ip );

    //*********************************************************************************

}
}

int get(char * filename, char * new_filename)
{
    if(!new_filename)
    {
        new_filename = filename;
    }

    int directory_index = findDirectoryEntry(filename);
    int inode_index = dir[directory_index].inode;

    if(directory_index == -1)
    {
        return -1;
    }

    struct Inode * inode = &(inodes[inode_index]);

	FILE *op;
    op = fopen(new_filename, "w");
    if(op == NULL)
    {
        printf("Couldn't open output file: %s \n",new_filename);
        perror("output file returned");
        return -1;
    }

    int block_index = 0;
    int copy_size   = inodes[inode_index].size;
    int offset      = 0;

    printf("Writing %d bytes to %s \n", copy_size, new_filename);

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file.
    while( copy_size > 0 )
    {

      int num_bytes;

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else
      {
        num_bytes = BLOCK_SIZE;
      }

      // Write num_bytes number of bytes from our data array into our output file.

      int new_block_index = inode->blocks[block_index];
      fwrite(&blocks[new_block_index], num_bytes, 1, op);

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index ++;

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek( op, offset, SEEK_SET );
    }

    // Close the output file, we're done.
    fclose( op );
  }


//******************************************************************************************************************


void list()
{
 int validity = 0;
 int i;
 for(i=0; i< NUM_FILES; i++)
 {
	 if(dir[i].valid)
	 {
	     validity = 1;
		 int inode_idx = dir[i].inode;
		 printf("%s %d\n", dir[i].filename, dir[i].inode);
	 }
 }
 if(validity == 0)
    printf("list: No file found. \n");
}

void file_open(char * filename)
{
    fd = fopen(filename, "r");

    if (fd == NULL)
    {
        printf("open: File not found \n");
        return;
    }
    fread(&blocks[0], NUM_BLOCKS,BLOCK_SIZE, fd);
	fclose(fd);
}



void file_close()
{
	fd = fopen(filename_close, "w");
    if (fd == NULL)
    {
        printf("File is not open \n");
        return;
    }

	fwrite( &blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);
	fclose( fd );
	filename_close = NULL;
}

void delete(char * delFilename)
{   int found = 0;
	int i;
	int ret = -1;
	fd = fopen ( "disk.image", "r" );
	if(fd == NULL)
    {
        printf("del error: File not found. \n");
    }
	for( i=0; i< NUM_BLOCKS;  i++)
	{

     if(strcmp(dir[i].filename, delFilename)==0)
	 {
		 strcpy(dir[i].filename, "");
		 dir[i].valid     =  0;
		 dir[i].inode     = -1;
		 freeBlockList[i] =  1;
		 freeInodeList[i] =  1;
		 found = 1;
	 }
	}
	if(found == 0)
        printf("del error: File not found. \n");
	fclose(fd);
}

void createfs( char * filename)
{
fd = fopen( filename, "w" );

memset( &blocks[0], 0, NUM_BLOCKS * BLOCK_SIZE);

initializeDirectory();
initializeBlockList();
initializeInodeList();
initializeInodes();

fwrite( &blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);

fclose( fd );
}

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    int token_index  = 0;
/*    for( token_index = 0; token_index < token_count; token_index ++ )
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );
    } */

    free( working_root );

    dir             = (struct Directory_Entry *)&blocks[0]; //directory points to address of block zero ,
				  // block is 2D array and dir pointing to Directory_Entry - error so we have to cast it!!
    inodes          = (struct Inode*) &blocks[7];                                   //--why ??
    freeInodeList   = (uint8_t *)&blocks[5];
    freeBlockList   = (uint8_t *)&blocks[6];

//    initializeDirectory();
//    initializeBlockList();
//    initializeInodeList();
//    initializeInodes();

    freeInodeList[12] = 0;

    if(strcmp(token[0],"createfs")==0 && token[1]==NULL)
    {
        printf("createfs: File not found \n");
    }

    else

    if(strcmp(token[0],"createfs")==0 && token[1]!=NULL)
    {
        char *filename = token[1];
        createfs(filename);
    }
    else

    if(strcmp(token[0],"open")==0 && token[1]==NULL)
    {
        printf("open: File not found \n");
    }

    else

    if(strcmp(token[0],"open")==0 && token[1]!=NULL)
    {
        char *filename = token[1];
        filename_close = token[1];
        file_open(filename);
    }

    else

    if(strcmp(token[0],"close")==0)
    {
        file_close();
    }

    else

    if(strcmp(token[0],"df")==0)
    {
       printf("Disk space remaining %d\n", df() );
    }

    else

    if(strcmp(token[0],"put")==0 && token[1]==NULL)
    {
        printf("put: File not found \n");
    }

    else

    if(strcmp(token[0],"put")==0 && token[1]!=NULL)
    {
        int len;
        len = strlen(token[1]);
        if(len>32)
        {
            printf("put error: File name too long. \n");
        }
        else
        {
            put(token[1]);
        }
    }

    else

    if(strcmp(token[0],"list")==0 || (strcmp(token[0],"list")==0 && strcmp(token[1],"[-h]")==0))
    {
        list();
    }

    else

    if(strcmp(token[0],"del")==0 && token[1]==NULL)
    {
        printf("del error: Enter filename \n");
    }

    else

    if(strcmp(token[0],"del")==0 && token[1]!=NULL)
    {
        delete(token[1]);
    }

    else

    if(strcmp(token[0],"get")==0 && token[1]!=NULL)
    {
        get(token[1],token[2]);
    }

    else

    if(strcmp(token[0],"exit")==0)
    {
        exit(0);
    }
  }
  return 0;
}

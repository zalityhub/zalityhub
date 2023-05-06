#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>

#include "util.h"



static char *Moi;


static void
Usage(char *msg, ...)
{
  va_list ap;

  if(strlen(msg) > 0) {
      va_start(ap, msg);
      char *text = Sprintfv(NULL, 0, msg, ap);
      fprintf(stderr, "%s\n", text);
  }

  fprintf(stderr, "Usage: %s file\n", Moi);
  exit(0);
}


static int
CreatFile(char *fileName)
{
  close(open(fileName, O_WRONLY|O_CREAT , 0666));
  return 0;
}


int
main(int argc, char *argv[])
{

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  // collect options
  while(argc > 0) {
    char *arg = *argv;

    if(arg[0] != '-')
      break;      // Not an option

    --argc;
    ++argv;
    int inOption = 1;
    while(inOption && arg[inOption]) {
      switch(arg[inOption++]) {
        default:
          fprintf(stderr, "-%c is not a valid option", arg[inOption-1]);
          break;
      }
    }
  }    // while options...

  // Iterate over the file set
  if( argc <= 0 )
    Usage("%s: file set is missing", Moi);

  while(argc > 0) {
    char *filename = *argv;
    --argc;
    ++argv;

#if _NONSTOP
    CreatFile(filename);
#else
#endif
  }

  return 0;
}

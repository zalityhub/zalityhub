#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>

#include "util.h"


typedef struct FinfoDef
{
  short info;
  char  *fileName;
} FinfoDef ;


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

  fprintf(stderr, "Usage: %s item file\n", Moi);
  fprintf(stderr, "Try '%s --help' for more information\n", Moi);
  exit(0);
}

static void
Help()
{
  printf("Usage: %s item file\n", Moi);
  printf(
     "%s - Displays file attribute 'item' from file 'file'\n"
     "   --help: Display this help text and exit\n"
       , Moi);

  exit(0);
}


static short
GetFileInfo(char *name, short ni, short *il, short *ri, short rmx)
{
  short rl = 0;
  short error_item = 0;
  short flen = (short)strlen(name);

  memset(ri, 0, rmx);
  short rc = FILE_GETINFOLISTBYNAME_ (name, flen,
      il,
      ni,
      ri,
      (unsigned short)rmx,
      (unsigned short*)&rl,
      &error_item);

  if(rc != 0) {
    rl = 0 - rc;
    memset(ri, 0, rmx);
  }

  return rl;
}


static int
FinfoFile(char *fileName, short info)
{
  short rmx = BUFSIZ;
  short *ri = (short*)calloc(1, rmx);
  short rl = GetFileInfo(fileName, (short)1, &info, ri, rmx);
  if( rl <= 0 ) {
    printf("Unable to obtain %d from %s; error %d\n", info, fileName, 0-rl);
    free(ri);
    return 0;
  }

  switch(rl) {
    default:
      HexDump(NULL, NULL, ri, rl, 0);
      break;
    case 1:
      printf("%c\n", *((uint8_t*)ri));
      break;
    case 2:
      printf("%d\n", *((uint16_t*)ri));
      break;
    case 4:
      printf("%d\n", *((uint32_t*)ri));
      break;
    case 8:
      printf("%d\n", *((uint64_t*)ri));
      break;
  }

  free(ri);
  return 0;
}


int
main(int argc, char *argv[])
{
  FinfoDef def;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  memset(&def, 0, sizeof(def));

  // collect options
  while(argc > 0) {
    char *arg = *argv;

    if(arg[0] != '-' && arg[0] != '+')
      break;      // Not an option

    if(strcmp(arg, "--help") == 0)
      Help();

    if( ! arg[1] )    // no chr following option choice
      Usage("missing option following '%s'", arg);

    --argc;
    ++argv;
    int inOption = 1;
    while(inOption && arg[inOption]) {
      if( arg[0] == '-' ) {
        switch(arg[inOption++]) {
          default:
            Usage("-%c is not a valid option", arg[inOption - 1]);
            break;
          case '?':
            Usage("");
            break;
        }
      }
      else if( arg[0] == '+' ) {
        switch(arg[inOption++]) {
          default:
            Usage("+%c is not a valid option", arg[inOption - 1]);
            break;
        }
      }
    }
  }    // while options...

  if(argc != 2)
    Usage("missing options");

  def.info = (short)atoi(*argv);   // info item
  def.fileName = *++argv;     // filename
  FinfoFile(def.fileName, def.info);

  return 0;
}

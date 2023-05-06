#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <dirent.h>
#include <glob.h>

#include "util.h"



static void
Usage(char *moi, char *msg, ...)
{
  va_list ap;

  if (strlen (msg) > 0) {
      va_start (ap, msg);
      char *text = Sprintfv (NULL, 0, msg, ap);
      fprintf(stderr, "%s\n", text);
  }

  fprintf (stderr, "Usage: %s [OPTION]... PATTERN[FILE]...\n", moi);
  fprintf (stderr, "Try '%s --help' for more information\n", moi);
  exit (0);
}


/*
 * Parameters
   julian-timestamp
   input
   FIXED:value
   is a 64-bit Julian timestamp to be converted.
   lct
   output
   INT:ref:8
   returns an array containing the date and time of day. A value of -1 is returned in word [0] if
   the supplied Julian timestamp is out of range (see Considerations). The array has this form:
   [0] The Gregorian year (1984, 1985, ...)
   [1] The Gregorian month (1-12)
   [2] The Gregorian day of month (1-31)
   [3] The hour of the day (0-23)
   [4] The minute of the hour (0-59)
   [5] The second of the minute (0-59)
   [6] The millisecond of the second (0-999)
   [7] The microsecond of the millisecond (0-999)
*/
static long
OffsetTimeStamp(short *lct, int offset)
{
  lct[3] += offset;
  int adj = (lct[3] > 23) ? (1) : ((lct[3] < 0) ? -1 : 0);
  if( adj ) {
    lct[3] = 1;
    lct[2] += adj;
    switch (lct[1]) {
    default:
      adj = (lct[2] > 31) ? (1) : ((lct[2] < 1) ? -1 : 0);
      break;
    case 2:
      adj = (lct[2] > 28) ? (1) : ((lct[2] < 1) ? -1 : 0);
      break;
    case 4:
    case 6:
    case 9:
    case 11:
      adj = (lct[2] > 30) ? (1) : ((lct[2] < 1) ? -1 : 0);
      break;
    }
    if(adj) {
      lct[2] = 1;
      if( (lct[1] += adj) > 12) {
        lct[1] = 1;
        lct[0] += adj;
      }
    }
  }
  return 0;
}


static int
SyncFile(char *pathName, void *entry, void *cbarg)
{
  FsFileInfoDef *finfo = (FsFileInfoDef*)entry;
  // long long *cdt = (long long*)&finfo->fileCode[1];
  long long *mdt = (long long*)&fileCode[5];
  short lct[8];
  char *fileName;

  if( *fileCode != 101 )
    return 0;  // skip non edit files

  int long rc = INTERPRETTIMESTAMP(*mdt, lct);
  rc = OffsetTimeStamp(lct, -4);
  char tmp[128];
  sprintf(tmp, "%04d-%02d-%02d|%02d:%02d:%02d", lct[0], lct[1], lct[2], lct[3], lct[4], lct[5]);
  if( (fileName = strrchr(pathName, '.')) == NULL )
    fileName = pathName;
  else
    ++fileName;

  printf("%s|%s\n", fileName, tmp);
  // exit(0);
  return 0;
}

int
main (int argc, char *argv[])
{
  char *moi;
  char *arg;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  // collect options
  while (argc > 0) {
    arg = *argv;

    if (arg[0] != '-')
      break;      // Not an option
    if( ! arg[1] )    // no chr following option choice
      Usage("missing option following '%s'", arg);

    --argc;
    ++argv;
    int inOption = 1;
    while(inOption && arg[inOption]) {
      switch(arg[inOption++]) {
        default:
          Usage (moi, "-%c is not a valid option", arg[inOption - 1]);
          break;
        case '?':
          Usage("");
          break;
        }
    }
  }    // while options...

#if _NONSTOP
  WalkDir("$VC02.VCCNXS.a", FsResolveNodeName, &SyncFile, (void*)NULL);
  WalkDir("$VC02.VCCNXS.zx25tal", FsResolveNodeName, &SyncFile, (void*)NULL);
#endif

  return 0;
}

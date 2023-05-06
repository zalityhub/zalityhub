#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <glob.h>

#include "util.h"



typedef enum
{
  DumpRadixOctal = 8,
  DumpRadixDecimal = 10,
  DumpRadixHex = 16
} DumpRadix_t ;

static DumpRadix_t     DumpRadix = DumpRadixHex;


typedef struct DumpDef
{
#if _NONSTOP
  int editFilesOnly;
#endif
} DumpDef ;


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

  fprintf(stderr, "Usage: %s [-e, -a text] *\n", Moi);
  exit(0);
}


static void
DumpmemFull(void (*put)(int offset, char *dump, char *text, void *arg),
             void *putarg, void *mem, int len, int offset)
{
  const unsigned char *memPtr =(const unsigned char*) mem;
  int   ii, jj;
  char  hexbuf[128];
  char  textbuf[64];
  char  *hb = hexbuf;

  if (len <= 0)
    return;     // done

  hexbuf[0] = textbuf[0] = (char)0;
  for(ii = jj = 0; ii < len; ++ii) {
    if (0 == ii || 0 == (ii % 16)) {   // line break
      if (hexbuf[0] != '\0') // have line data
        (*put) (offset, hexbuf, textbuf, putarg);
      hb = hexbuf;
      *hb = '\0';
      jj = 0;
      if (ii)
        offset += 16;
    }
    hb += sprintf(hb, "%2.2x ", (int) memPtr[ii] & 0xff);

    if (isprint((int) memPtr[ii]))
      textbuf[jj] = memPtr[ii];
    else
      textbuf[jj] = '.';
    textbuf[++jj] = 0;
  }

  if (hexbuf[0]) {
    (*put) (offset, hexbuf, textbuf, putarg);
    offset += 16;
  }
}


typedef struct DumpLine_t
{
 void            (*put)(char *line, void *arg);
 void           *arg;
} DumpLine_t;

static void
_DumpLine(int offset, char *dump, char *text, void *arg)
{
  char  tmp[BUFSIZ];

  if (DumpRadix == DumpRadixHex)
    sprintf(tmp, "%6x: %-48.48s %-16.16s", offset, dump, text);
  else if (DumpRadix == DumpRadixOctal)
    sprintf(tmp, "%6o: %-48.48s %-16.16s", offset, dump, text);
  else      // use decimal
    sprintf(tmp, "%6d: %-48.48s %-16.16s", offset, dump, text);
  DumpLine_t     *dlt = (DumpLine_t *) arg;

  if( dlt->put )
    (*dlt->put) (tmp, dlt->arg);
  else
    puts(tmp);
}


static void
Dumpmem(void (*put)(char *line, void *arg),
        void *putarg, void *mem, int len, int offset)
{
  DumpLine_t      dlt;

  dlt.put = put;
  dlt.arg = putarg;
  DumpmemFull(_DumpLine, (void*)&dlt, mem, len, offset);
}


static int
DumpFile(char *fileName, void *entry, void *cbarg)
{
  DumpDef *def = (DumpDef*)cbarg;

#if _NONSTOP
  FsFileInfoDef *finfo = (FsFileInfoDef*)entry;
  if(def->editFilesOnly && finfo->fileCode != 101)
    return 0;  //  wants edit files only and this is not an edit file...
#endif

  FsFileDef *file = FsOpen(fileName, "r");
  if( file == NULL ) {
    fprintf(stderr, "Unable to open %s\n", fileName);
    perror("FsOpen");
    return 0;
  }

  char bfr[BUFSIZ];
  int  off = 0;
  int  rlen;

  while( (rlen = FsRead(file, bfr, sizeof(bfr))) > 0 ) {
    Dumpmem(NULL, NULL, bfr, rlen, off);
    off += rlen;
  }
  FsClose(file);

  return 0;
}


int
main(int argc, char *argv[])
{
  DumpDef def;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  memset(&def, 0, sizeof(def));

  // collect options
  while(argc > 0) {
    char *arg = *argv;

    if(arg[0] != '-')
      break;      // Not an option
    if( ! arg[1] )    // no chr following option choice
      Usage("missing option following '%s'", arg);

    --argc;
    ++argv;
    int inOption = 1;
    while(inOption && arg[inOption]) {
      switch(arg[inOption++]) {
        default:
          Usage("-%c is not a valid option", arg[inOption - 1]);
          break;
        case '?':
          Usage("");
          break;
#if _NONSTOP
        case 'E':    // do edit files (101) only
          def.editFilesOnly = True;
          break;
#endif
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
    if( WalkDir(filename, FsResolvePathName, &DumpFile, (void*)&def) <= 0 )
      Usage("%s: No files match '%s'\n", Moi, filename);
#else
    glob_t globbuf;
    globbuf.gl_offs = 0;
    int rc;

    switch((rc = glob(filename, 0, NULL, &globbuf))) {
      case 0:
        break;    // good...
      default:
        Usage("%s: glob failed with %d\n", Moi, rc);
        break;
      case GLOB_NOMATCH:
        Usage("%s: No files match '%s'\n", Moi, filename);
        break;
    }

    if( globbuf.gl_pathc <= 0 )
      Usage("%s: No files match '%s'\n", Moi, filename);

    for(int i = 0; i < globbuf.gl_pathc; ++i) {
      char *filename = globbuf.gl_pathv[i];
      DumpFile(filename, NULL, (void*)&def);
    }
#endif
  }

  return 0;
}

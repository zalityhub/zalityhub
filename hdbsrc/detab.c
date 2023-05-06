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



typedef struct TabDef
{
  int     editFilesOnly;
  short   tabsize;
} TabDef ;


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
TabifyFile(char *fileName, void *entry, void *cbarg)
{
  TabDef *def = (TabDef*)cbarg;

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

  for(char *line;(line = FsReadLine(file));) {
    char *tmp = malloc(strlen(line)*def->tabsize+128);
    char *op = tmp;
    char *lp = line;
    char *tp;
    short ll;
    while(tp = strchr(lp, '\t')) {
      ll = (short)(tp-lp);
      memcpy(op, lp, ll);  // copy prefix
      op += ll;
      lp += ++ll;
      for( int i = 0; i < def->tabsize; ++i )
        *op++ = ' ';
    }
    ll = (short)strlen(lp);
    memcpy(op, lp, ll);  // copy prefix
    op += ll;
    *op = '\0';
    printf("%s", tmp);
    free(tmp);
  }

  FsClose(file);

  return 0;
}

int
main(int argc, char *argv[])
{
  TabDef def;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  memset(&def, 0, sizeof(def));
  def.tabsize = 2;

  // collect options
  while(argc > 0) {
    char *arg = *argv;

    if(arg[0] != '-')
      break;      // Not an option

    --argc;
    ++argv;
#if 0
    int inOption = 1;
    while(inOption && arg[inOption]) {
      switch(arg[inOption++]) {
        default:
          fprintf(stderr, "-%c is not a valid option", arg[inOption - 1]);
          break;
        case 'E':    // search edit files (101) only
          def.editFilesOnly = True;
          break;
      }
    }
#else
    if(!isdigit(arg[1]))
      Usage("%s: Invalid tab size '%s'\n", Moi, arg);
    def.tabsize = (short)atoi(&arg[1]);
#endif
  }    // while options...

  // Iterate over the file set
  if( argc <= 0 )
    Usage("%s: file set is missing", Moi);

  while(argc > 0) {
    char *filename = *argv;
    --argc;
    ++argv;

#if _NONSTOP
    if( WalkDir(filename, FsResolveNodeName, &TabifyFile, (void*)&def) <= 0 )
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
      TabifyFile(filename, NULL, (void*)&def);
    }
#endif
  }

  return 0;
}

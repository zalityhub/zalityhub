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



typedef struct CatDef
{
#if _NONSTOP
  int editFilesOnly;
#endif
  char  *displayLinesAfter;
} CatDef ;


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


static int
CatFile(char *fileName, void *entry, void *cbarg)
{
  CatDef *def = (CatDef*)cbarg;

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
    if( def->displayLinesAfter ) {
      if( stristr(line, def->displayLinesAfter) ) {
        def->displayLinesAfter = NULL;    // found it
      }
      continue;
    }
    printf("%s", line);
    if( ! strchr(line, '\n') )
      puts("");
  }

  FsClose(file);

  return 0;
}

int
main(int argc, char *argv[])
{
  CatDef def;

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
        case 'E':    // search edit files (101) only
          def.editFilesOnly = True;
          break;
#endif
        case 'a':    // display after hitting text
          if(argc < 1)
            Usage("-a argument is missing");
          def.displayLinesAfter = strdup(*argv);
          --argc;
          ++argv;
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
    if( WalkDir(filename, FsResolveNodeName, &CatFile, (void*)&def) <= 0 )
      fprintf(stderr, "%s: No files match '%s'\n", Moi, filename);
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
      CatFile(filename, NULL, (void*)&def);
    }
#endif
  }

  return 0;
}

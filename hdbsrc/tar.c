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
#include "cJSON.h"



typedef struct TarDef
{
#if _NONSTOP
  int editFilesOnly;
#endif
} TarDef ;


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


// Create file header
static cJSON*
CreateFileHeader(FsFileDef *file)
{
  char  tmp[BUFSIZ];
  cJSON *root = NULL;

  root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "name", file->fileName);

  sprintf(tmp, "%d,%d", file->_nonstop.finfo.owner.owner[0], file->_nonstop.finfo.owner.owner[1]);
  cJSON_AddStringToObject(root, "owner", tmp);
  cJSON_AddStringToObject(root, "security", FsGetFilePerissions(file->_nonstop.finfo.security));
  cJSON_AddStringToObject(root, "creation_time", FsGetFileDate(file->_nonstop.finfo.creationTime));
  cJSON_AddStringToObject(root, "modify_time", FsGetFileDate(file->_nonstop.finfo.modifyTime));

  cJSON_AddNumberToObject(root, "phyRecLen", file->_nonstop.finfo.phyRecLen);
  cJSON_AddNumberToObject(root, "fileType", file->_nonstop.finfo.fileType);
  cJSON_AddNumberToObject(root, "fileCode", file->_nonstop.finfo.fileCode);
  cJSON_AddNumberToObject(root, "eof", file->_nonstop.finfo.eof);

  return root;
}

static int
TarFile(char *fileName, void *entry, void *cbarg)
{
  TarDef *def = (TarDef*)cbarg;

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

  cJSON *header = CreateFileHeader(file);
  char *out = cJSON_Print(header);
  puts(out);
  free(out);

  char bfr[BUFSIZ];
  int  rlen;

  while( (rlen = FsRead(file, bfr, sizeof(bfr))) > 0 ) {
  }
  FsClose(file);

  return 0;
}


int
main(int argc, char *argv[])
{
  TarDef def;

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
    if( WalkDir(filename, FsResolvePathName, &TarFile, (void*)&def) <= 0 )
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
      TarFile(filename, NULL, (void*)&def);
    }
#endif
  }

  return 0;
}

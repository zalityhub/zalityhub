#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <regex.h>
#include <stdarg.h>
#include <dirent.h>
#include <glob.h>

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

  fprintf(stderr, "Usage: %s [OPTION]... PATTERN[FILE]...\n", Moi);
  fprintf(stderr, "Try 'grep --help' for more information\n", Moi);
  exit(0);
}


static void
Help()
{
  printf("Usage: %s[OPTION]... PATTERN[FILE]...\n", Moi);
  printf(
     "Search for PATTERN in each FILE\n"
     "PATTERN is a basic regular expression\n"
     "Example: grep -i 'hello world' menu.h main.c\n\n"
     "Regexp selection and interpretation:\n"
     "  -i               ignore case distinctions; default on\n"
     "  +i               do not ignore case distinctions\n"
     "  -r               raw text matching - no regular expression\n"
     "  -w               force PATTERN to match only whole words\n");

#if _NONSTOP
  printf(
     "  +e               search all file types \n"
     "  -e               search edit file types (101) only (default)\n");
#endif

  printf(
       "\n"
       "Miscellaneous:\n"
       "  --help           display this help text and exit\n"
       "  -d               debug mode\n"
       "  -q               Suppresses  all  output  except error messages.\n"
       "                   This is useful for easily determining whether\n"
       "                   or not a pattern or string exists in a group\n"
       "                   of files. When searching several files,\n"
       "                   it provides a performance improvement because\n"
       "                   it can quit as soon as it finds the first match,\n"
       "                   and it requires less care by the user in choosing\n"
       "                   the set of files to supply as arguments because\n"
       "                   it exits with a 0 (zero) exit status if it\n"
       "                   detects a match, even if the grep command\n"
       "                   detected an access or read error on\n"
       "                   earlier file arguments.\n"
       "  -v               select non-matching lines\n"
       "  -x command       execute command when match is found\n"
       "\n"
       "Output control:\n"
       "  -n               print line number with output lines\n"
       "  -l               print only names of FILEs containing matches\n"
       "  -s               print short file names\n"
       "  -c               print only a count of matching lines per FILE\n"
       "\n"
       "Context control:\n"
       "  -B               print NUM lines of leading (before) context\n"
       "  -A               print NUM lines of trailing (after) context\n"
       );

  exit(0);
}


static int
Grep(char *fileName, void *entry, void *cbarg)
{
  GrepDef *def = (GrepDef*)cbarg;

#if _NONSTOP
  FsFileInfoDef *finfo = FsGetFileInfo(fileName);
  if(def->editFilesOnly && finfo->fileCode != 101) {
    // printf("skip file %s code=%d\n", fileName, finfo->fileCode);
    return 0;  //  wants edit files only and this is not an edit file...
  }
#endif

  (void)GrepFile(def, fileName);
  return 0;
}


int
main(int argc, char *argv[])
{
  GrepDef def;
  char    *arg;
  int     isMatch = 0;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  memset(&def, 0, sizeof(def));
  def.matchWord = "";
#if _NONSTOP
  def.editFilesOnly = True;
#endif
  def.ignoreCase = REG_ICASE;

  // collect options
  while(argc > 0) {
    arg = *argv;

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
          case 's':
            ++def.shortNames;
            break;
          case 'd':
            ++def.isDebug;
            break;
          case 'q':
            ++def.quiet;
            break;
          case 'x':
            if(argc < 1)
              Usage("-x command argument is missing");
            def.executeCommand = *argv;
            --argc;
            ++argv;
            break;
          case 'i':    // ignore case distinctions
            def.ignoreCase = REG_ICASE;
            break;
          case 'w':
            def.matchWord = "[^a-z,A-Z,0-9,_]";
            break;
          case 'r':    // raw text matching - no regular expression
            def.rawMatch = True;
            break;
        #if _NONSTOP
          case 'e':    // search edit files (101) only
            def.editFilesOnly = True;
            break;
        #endif
          case 'v':  // select non-matching lines
            def.invertMatch = True;
            break;
          case 'n':    // print line number with output lines
            def.displayLineNbrs = True;
            break;
          case 'l':    // print only names of FILEs containing matches
            def.displayNamesOnly = True;
            break;
          case 'c':    // print only a count of matching lines per FILE
            def.displayCountsOnly = True;
            break;
          case 'B':    // print NUM lines of leading context
            if(argc < 1)
              Usage("-B argument is missing");
            def.displayLinesBefore = atoi(*argv);
            --argc;
            ++argv;
            break;
          case 'A':    // print NUM lines of trailing context
            if(argc < 1)
              Usage("-A argument is missing");
            def.displayLinesAfter = atoi(*argv);
            --argc;
            ++argv;
            break;
        }
      }
      else if( arg[0] == '+' ) {
        switch(arg[inOption++]) {
          default:
            Usage("+%c is not a valid option", arg[inOption - 1]);
            break;
          case 'i':
            def.ignoreCase = 0;    // don't ignore case
            break;
        #if _NONSTOP
          case 'e':    // search any files
            def.editFilesOnly = False;
            break;
        #endif
        }
      }
      else
        Fatal("Should not get here\n");

    }
  }    // while options...

  if(argc < 1)
    Usage("");

  arg = *argv;
  --argc;
  ++argv;

  strcpy(def.pattern, arg);

  // Iterate over the file set
  if( argc <= 0 )
    Usage("%s: file set is missing", Moi);

  while(argc > 0) {
    char *fileName = *argv;
    --argc;
    ++argv;

#if _NONSTOP
    def.showFileName = (strchr(fileName, '*') != NULL);
    if( WalkDir(fileName, FsResolveNodeName, &Grep, (void*)&def) <= 0 )
      Usage("%s: No files match '%s'\n", Moi, fileName);
#else
    glob_t globbuf;
    globbuf.gl_offs = 0;
    int rc;

    switch((rc = glob(fileName, 0, NULL, &globbuf))) {
      case 0:
        break;    // good...
      default:
        Usage("%s: glob failed with %d\n", Moi, rc);
        break;
      case GLOB_NOMATCH:
        Usage("%s: No files match '%s'\n", Moi, fileName);
        break;
    }

    if( globbuf.gl_pathc <= 0 )
      Usage("%s: No files match '%s'\n", Moi, fileName);

    for(int i = 0; i < globbuf.gl_pathc; ++i) {
      char *fileName = globbuf.gl_pathv[i];
      Grep(fileName, NULL, (void*)&def);
    }
#endif
    isMatch |= def.totalMatch;
  }

  exit(0);
}

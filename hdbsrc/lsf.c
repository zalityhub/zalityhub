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


typedef struct FileDef
{
  char       *filename;
  FsFileInfoDef finfo;
} FileDef ;

typedef struct LsDef
{
  short optCode;
  short optUser;
  short optGroup;
  short optFname;
  short optBrief;
  short optHuman;
  char  *optExclude[128];
  short optPath;
  short optTime;
  short optSize;
  short optRev;
  long long optNewer;

  int        nSorted;
  FileDef    **sorted;
} LsDef ;


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

  fprintf(stderr, "Usage: %s [ -fpbtsrh, -n date, -c code, -u user, -g group] *\n", Moi);
  fprintf(stderr, "Try 'ls --help' for more information\n", Moi);
  exit(0);
}

static void
Help()
{
  printf("Usage: %s [ -fpbtsrhP, -v text, -n date, -c code, -u user, -g group] *\n", Moi);
  printf(
     "ls - Lists and generates statistics for files\n"
     "   -f      File format: display only file names\n"
     "   -p      Path format: display full file names\n"
     "   -P      Path format: format names with path\n"
     "   -b      Brief format: display {size} {date} {time} {name}\n"
     "                                 114 2022-09-30 15:11 test\n"
     "   -t      Time sorted by modified time\n"
     "   -s      Size sorted\n"
     "   -r      Reverse display order\n"
     "   -h      Human format: show file size by bytes, Kb, Mb, Gb\n"
     "             and display username in place of gid,uid\n"
     "   -v      Exclude files matching text\n"
     "   -n      Modify date newer than 'date'\n"
     "   -c      File Code format: display on those files with filecode = 'code'\n"
     "   -u      User Code format: display on those files with user = 'user'\n"
     "   -g      Group Code format: display on those files with group = 'group'\n"
     "   --help: Display this help text and exit\n"
       );

  exit(0);
}


static int
LsCmpFileName(const void* a, const void* b)
{
  int r = strcmp((*(FileDef**)a)->filename, (*(FileDef**)b)->filename);
  return r;
}


static int
LsCmpFileNameRev(const void* a, const void* b)
{
  int r = strcmp((*(FileDef**)b)->filename, (*(FileDef**)a)->filename);
  return r;
}


static int
LsCmpModifyTime(const void* a, const void* b)
{
  long long *av;
  long long *bv;
  long long r;

  av = (long long*)(*(FileDef**)a)->finfo.modifyTime;
  bv = (long long*)(*(FileDef**)b)->finfo.modifyTime;
  r = (*bv - *av);
  if( r == 0 )
    return 0;
  if( r < 0 )
    return -1;
  return 1;
}


static int
LsCmpModifyTimeRev(const void* a, const void* b)
{
  long long *av;
  long long *bv;
  long long r;

  av = (long long*)(*(FileDef**)a)->finfo.modifyTime;
  bv = (long long*)(*(FileDef**)b)->finfo.modifyTime;
  r = (*av - *bv);
  if( r == 0 )
    return 0;
  if( r < 0 )
    return -1;
  return 1;
}


static int
LsCmpSizeRev(const void* a, const void* b)
{
  uint32_t  av;
  uint32_t  bv;
  int       r;

  av = (uint32_t)(*(FileDef**)a)->finfo.eof;
  bv = (uint32_t)(*(FileDef**)b)->finfo.eof;
  r = (int)(bv - av);
  if( r == 0 )
    return 0;
  if( r < 0 )
    return -1;
  return 1;
}


static int
LsCmpSize(const void* a, const void* b)
{
  uint32_t  av;
  uint32_t  bv;
  int       r;

  av = (uint32_t)(*(FileDef**)a)->finfo.eof;
  bv = (uint32_t)(*(FileDef**)b)->finfo.eof;
  r = (int)(av - bv);
  if( r == 0 )
    return 0;
  if( r < 0 )
    return -1;
  return 1;
}


static int
LsSort(LsDef *def)
{

  if( def->optTime ) {
    if( def->optRev )
      qsort(def->sorted, def->nSorted, sizeof(FileDef*), LsCmpModifyTimeRev);
    else
      qsort(def->sorted, def->nSorted, sizeof(FileDef*), LsCmpModifyTime);
  }
  else if( def->optSize ) {
    if( def->optRev )
      qsort(def->sorted, def->nSorted, sizeof(FileDef*), LsCmpSizeRev);
    else
      qsort(def->sorted, def->nSorted, sizeof(FileDef*), LsCmpSize);
  }
  else {
    if( def->optRev )
      qsort(def->sorted, def->nSorted, sizeof(FileDef*), LsCmpFileNameRev);
    else
      qsort(def->sorted, def->nSorted, sizeof(FileDef*), LsCmpFileName);
  }
  return 0;
}


static int
SvFile(char *filename, void *entry, void *cbarg)
{
  LsDef *def = (LsDef*)cbarg;
  FsFileInfoDef *finfo = (FsFileInfoDef*)entry;

  if( def->optCode >= 0 && def->optCode != finfo->fileCode )
    return 0;    // filter by fileCode

  if( def->optUser && def->optUser != finfo->owner.owner[1] )
    return 0;    // filter by User

  if( def->optExclude ) {
    for(char **tmp = def->optExclude; *tmp; ++tmp)
      if( stristr(filename, *tmp) )
        return 0;    // filter by text
  }

  if( def->optGroup && def->optGroup != finfo->owner.owner[0] )
    return 0;    // filter by Group

  if( def->optNewer && def->optNewer > (*((long long*)finfo->modifyTime)) )
      return 0;    // filter by modify time

// save this file

  FileDef *fdef = (FileDef*)calloc(1, sizeof(FileDef));
  fdef->filename = strdup(filename);
  memcpy(&fdef->finfo, finfo, sizeof(fdef->finfo));

  if( def->sorted == NULL )    // first time
    def->sorted = (FileDef**)calloc(1, sizeof(FileDef));
  def->sorted = (FileDef**)realloc(def->sorted, sizeof(FileDef)*(def->nSorted+1));
  def->sorted[def->nSorted++] = fdef;
  return 0;
}

static char*
FormatEof(LsDef *def, FileDef *file)
{
  static char tmp[32];
  FsFileInfoDef *finfo = &file->finfo;

  if( ! def->optHuman ) {
    sprintf(tmp, "%12d", finfo->eof);
    return tmp;
  }

  uint32_t g, m, k;
  char     *suffix = "";
  double   d = (double)finfo->eof;

  k = 1024;
  m = k * 1024;
  g = m * 1024;

  if( (finfo->eof / g) ) {    // gb range
    strcpy(tmp, ftoa(d / (double)g, NULL));
    suffix = "g";
  }
  else if( (finfo->eof / m) ) {    // gb range
    strcpy(tmp, ftoa(d / (double)m, NULL));
    suffix = "m";
  }
  else if( (finfo->eof / k) ) {    // gb range
    strcpy(tmp, ftoa(d / (double)k, NULL));
    suffix = "k";
  }
  else {
    sprintf(tmp, "%12d", finfo->eof);
  }

  if(strchr(tmp, '.'))
    strchr(tmp, '.')[3] = '\0';   // trim

  strcat(tmp, suffix);
  return tmp;
}


static char*
FormatFname(LsDef *def, FileDef *file)
{
  if( def->optPath ) {
    if( def->optPath == 1 )
      return file->filename;
    return strchr(file->filename, '.')+1;
  }
  return UtilBaseName(file->filename);
}

static char*
FormatUserid(LsDef *def, FileDef *file)
{
  static char tmp[128];
  FsFileInfoDef *finfo = &file->finfo;

  if( ! def->optHuman ) {
    sprintf(tmp, "%3d,%-3d", finfo->owner.owner[0], finfo->owner.owner[1]);
    return tmp;
  }

  sprintf(tmp, "%d,%d",
      finfo->owner.owner[0], finfo->owner.owner[1]);
  UserDef *user = GetUser(tmp);
  if( user )
    strcpy(tmp, user->user_name);
  else // failed, return gid,uid
    sprintf(tmp, "%3d,%-3d", finfo->owner.owner[0], finfo->owner.owner[1]);
  return tmp;
}

static int
LsFile(LsDef *def, FileDef *file)
{
  static char *mons[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    // [0] The Gregorian year (1984, 1985, ...)
    // [1] The Gregorian month (1-12)
    // [2] The Gregorian day of month (1-31)
    // [3] The hour of the day (0-23)
    // [4] The minute of the hour (0-59)
    // [5] The second of the minute (0-59)
    // [6] The millisecond of the second (0-999)
    // [7] The microsecond of the millisecond (0-999)
  int16_t greg[8];

  FsFileInfoDef *finfo = &file->finfo;

  int32_t r = INTERPRETTIMESTAMP(*((long long*)finfo->modifyTime), greg);

  if(def->optBrief) {
     // format: 114 2022-09-30 15:11 test

    printf("%12.12s %4d-%02d-%02d %02d:%02d %s\n",
           FormatEof(def, file),
           greg[0], greg[1], greg[2], greg[3], greg[4],
           FormatFname(def, file));
  } else if( def->optFname || def->optPath == 1) {
    printf("%s\n", FormatFname(def, file));
  } else {
    if( def->optPath == 2 ) {
      printf("%-26.26s  %4d  %12.12s %02d%s%04d %02d:%02d %s %s\n",
        ToUpper(FormatFname(def, file)),
        finfo->fileCode,
        FormatEof(def, file),
        greg[2],  // day
        (greg[1] <= 0 || greg[1] > 12) ? "???" : mons[greg[1]-1],
        greg[0],
        greg[3],
        greg[4],
        FormatUserid(def, file),
        FsGetFilePerissions(finfo->security));
    } else {
      printf("%-8.8s      %4d     %12.12s %02d%s%04d %02d:%02d %s %s\n",
        ToUpper(FormatFname(def, file)),
        finfo->fileCode,
        FormatEof(def, file),
        greg[2],  // day
        (greg[1] <= 0 || greg[1] > 12) ? "???" : mons[greg[1]-1],
        greg[0],
        greg[3],
        greg[4],
        FormatUserid(def, file),
        FsGetFilePerissions(finfo->security));
    }
  }

  return 0;
}


static int
LsFiles(LsDef *def)
{

  for(int i = 0; i < def->nSorted; ++i)
    LsFile(def, def->sorted[i]);

  return 0;
}


int
main(int argc, char *argv[])
{
  short error;
  char ch;
  char  **tmp;
  char *date;
  GregorianTimeDef greg;
  LsDef def;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  memset(&def, 0, sizeof(def));
  def.optCode = -1;    // default

  // collect options
  while(argc > 0) {
    char *arg = *argv;

    if(arg[0] != '-')
      break;      // Not an option

    if(strcmp(arg, "--help") == 0)
      Help();
    if( ! arg[1] )    // no chr following option choice
      Usage("missing option following '%s'", arg);

    --argc;
    ++argv;
    int inOption = 1;
    while(inOption && arg[inOption]) {
      switch(arg[inOption++]) {
        default:
          Usage("-%c is not a valid option", arg[inOption-1]);
          break;
        case '?':
          Usage("");
          break;
        case 'p':
          def.optPath = 1;
          break;
        case 'P':
          def.optPath = 2;
          break;
        case 'h':
          ++def.optHuman;
          break;
        case 'f':
          ++def.optFname;
          break;
        case 'b':
          ++def.optBrief;
          break;
        case 'v':
          if(argc < 1)
            Usage("-v command argument is missing");
          for(tmp = def.optExclude; *tmp; ++tmp)
            continue; // look for empty space
          *tmp = strdup(*argv);
          --argc;
          ++argv;
          break;
        case 't':
          ++def.optTime;
          break;
        case 's':
          ++def.optSize;
          break;
        case 'r':
          ++def.optRev;
          break;
        case 'n':
          if(argc < 1)
            Usage("-n command argument is missing");

          date = strdup(*argv);
          if(strlen(date) != 8)
            Usage("-n: Invalid format '%s': yyyymmdd required\n", date);

          memset(&greg, 0, sizeof(greg));
          ch = date[4];
          date[4] = '\0';
          greg.u.date.year = (int16_t)atoi(date);
          date[4] = ch;
          date += 4;

          ch = date[2];
          date[2] = '\0';
          greg.u.date.month = (int16_t)atoi(date);
          date[2] = ch;
          date += 2;

          ch = date[2];
          date[2] = '\0';
          greg.u.date.day = (int16_t)atoi(date);
          date[2] = ch;
          date += 2;

          def.optNewer = COMPUTETIMESTAMP(greg.u.int16, &error);
          if(error)
            Usage("-n: Invalid format in '%s': error %d\n", date, error);

          --argc;
          ++argv;
          break;
        case 'c':
          if(argc < 1)
            Usage("-c command argument is missing");
          def.optCode = (short)atoi(*argv);
          --argc;
          ++argv;
          break;
        case 'u':
          if(argc < 1)
            Usage("-u command argument is missing");
          def.optUser = (short)atoi(*argv);
          --argc;
          ++argv;
          break;
        case 'g':
          if(argc < 1)
            Usage("-g command argument is missing");
          def.optGroup = (short)atoi(*argv);
          --argc;
          ++argv;
          break;
      }
    }
  }    // while options...

  // Iterate over the file set

  if( argc <= 0 ) {
    argc = 1;
    *argv = "*";
  }

  while(argc > 0) {
    char *filename = *argv;
    --argc;
    ++argv;

#if _NONSTOP
    if( WalkDir(filename, FsResolveNodeName, &SvFile, (void*)&def) <= 0 )
      Fatal("%s: No files match '%s'\n", Moi, filename);
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
        Fatal("%s: No files match '%s'\n", Moi, filename);
        break;
    }

    if( globbuf.gl_pathc <= 0 )
      Fatal("%s: No files match '%s'\n", Moi, filename);

    for(int i = 0; i < globbuf.gl_pathc; ++i) {
      char *filename = globbuf.gl_pathv[i];
      SvFile(filename, NULL, (void*)&def);
    }
#endif
  }

  LsSort(&def);
  LsFiles(&def);

  return 0;
}

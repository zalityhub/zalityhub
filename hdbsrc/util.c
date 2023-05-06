#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <dirent.h>
#include <glob.h>
#include <regex.h>
#include <fcntl.h>
#include <math.h>

#define LEAK_DETECTION 0
#define NEED_PATH
#include "util.h"

#include "bt.h"

#if _NONSTOP
_variable void
Stop(short error, short goodstop)
{
  if (_arg_present(goodstop) && goodstop )
    PROCESS_STOP_ ( ,,0);
  else
    PROCESS_STOP_ ( ,,1,error);
}

char*
strdup(char *in)
{
  size_t len = strlen(in);
  char *out = (char*)malloc(len + 1);
  memcpy(out, in, len+1);
  return out;
}
#endif

#if _NONSTOP || __CYGWIN__ || __GNUC__
char*
stristr(char *s0, char *s1)
{
  char       *str[2];
  char       *cp;
  char       *ptr;

  if((str[0] = (char*)malloc (strlen (s0) + 1)) == NULL)
    return NULL;

  if((str[1] = (char*)malloc (strlen (s1) + 1)) == NULL) {
    free (str[0]);
    return NULL;
  }

  for (ptr = str[0], cp = s0; *cp;) {
    *ptr++ = (char) tolower (*cp);
    ++cp;
  }

  *ptr = '\0';

  for(ptr = str[1], cp = s1; *cp;) {
    *ptr++ = (char) tolower (*cp);
    ++cp;
   }

  *ptr = '\0';

  if ((ptr = strstr (str[0], str[1])) != NULL)
    ptr = (char*)&s0[(ptr - str[0])];

  free (str[0]);
  free (str[1]);
  return ptr;
}


int
strnicmp(char *s1, char *s2, int n)
{

  if( n <= 0 )    // no search
    return 0;

  while( n > 0 && *s1 && *s2 ) {
    if( tolower(*s1) != tolower(*s2) )
      return tolower(*s1) - tolower(*s2);
    if( --n <= 0 )
      break;      // done
    ++s1; ++s2;
  }
  return tolower(*s1) - tolower(*s2);
}


int
stricmp(char *s1, char *s2)
{

  while( *s1 && *s2 ) {
    if( tolower(*s1) != tolower(*s2) )
      return tolower(*s1) - tolower(*s2);
    ++s1; ++s2;
  }
  return tolower(*s1) - tolower(*s2);
}

#endif


int
memicmp(void *s1, void *s2, size_t n)
{
  if (n != 0) {
    unsigned char *p1 = s1, *p2 = s2;

    do {
      if (toupper(*p1) != toupper(*p2))
        return (*p1 - *p2);
      p1++;
      p2++;
    } while (--n != 0);
  }
  return 0;
}


char *memstr(char *haystack, char *needle, int size)
{
  char *p;
  int needlesize = (int)strlen(needle);

  for (p = haystack; p <= (haystack-needlesize+size); p++)
  {
    if (memcmp(p, needle, needlesize) == 0)
      return p; /* found */
  }
  return NULL;
}

char *memistr(char *haystack, char *needle, int size)
{
  char *p;
  int needlesize = (int)strlen(needle);

  for (p = haystack; p <= (haystack-needlesize+size); p++)
  {
    if (memicmp(p, needle, needlesize) == 0)
      return p; /* found */
  }
  return NULL;
}


/*+******************************************************************
    Name:
        ToLower - Convert all characters of a string to lowercase

    Synopsis:
        char *ToLower (char *string)

    Description:
        Converts each character of the given string to its lowercase
        equivalent.  Returns a pointer to the given string.

    Diagnostics:
        None.
-*******************************************************************/
char*
ToLower(char *string)
{
  for (char *s = string; *s; ++s)
    *s = (char)tolower(*s);
  return string;
}


/*+******************************************************************
    Name:
        ToUpper - Convert all characters of a string to uppercase

    Synopsis:
        char *ToUpper(char *string)

    Description:
        Converts each character of the given string to its uppercase
        equivalent.  Returns a pointer to the given string.

    Diagnostics:
        None.
-*******************************************************************/
char*
ToUpper(char *string)
{
    for (char *s = string; *s; ++s)
        *s = (char)toupper(*s);
    return string;
}

char*
Sprintfv (char *out, int size, char *fmt, va_list ap)
{
  va_list aq;

  va_copy (aq, ap);    // save a copy
  if (out == NULL || size <= 0) {
      // no string, return an alloc'ed one
    size = 16;
    out = (char*)calloc (size+2, sizeof (char));
  }

  int len = vsnprintf (out, size, fmt, ap);
  if (len >= size) {   // need room
      int nlen = Max (BUFSIZ, Max (1, len + 1));
      out = (char*)realloc(out, nlen+2);
      len = vsnprintf (out, len + 1, fmt, aq);
  }

  va_end (aq);
  return out;
}


char*
Sprintf (char *out, int size, char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  char *ret = Sprintfv (out, size, fmt, ap);
  va_end (ap);
  return ret;
}


char*
SprintfvCat(char *out, const char *fmt, va_list ap)
{
  va_list aq;

  va_copy(aq, ap);   // save a copy
  if (out == NULL )
    out = (char*)calloc(2, sizeof(char));
  int len = (int)strlen(out);

  char  t[0];
  int olen = vsnprintf(t, 0, fmt, ap);    // get the output size
  int nlen = len + olen;
  out = (char*)realloc(out, nlen+2);
  len = vsnprintf(out+len, olen + 1, fmt, aq);
  va_end(aq);
  return out;
}


char*
SprintfCat(char *out, const char *fmt, ...)
{

  va_list  ap;
  va_start(ap, fmt);
  out = SprintfvCat(out, fmt, ap);
  va_end(ap);
  return out;
}


void
Fatal (char *msg, ...)
{
  va_list ap;

  va_start (ap, msg);
  char *text = Sprintfv (NULL, 0, msg, ap);
  fprintf (stderr, "%s\n", text);
  exit (0);
}

char*
SysLog(SysLogLevel lvl, char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  char *bfr = Sprintfv (NULL, 0, fmt, ap);
  va_end (ap);
  if( lvl > LogWarn )
    fputs(bfr, stderr);
  else
    fputs(bfr, stdout);
  free(bfr);
  return "";
}

// You must free the result if result is non-NULL.
char*
str_replace(char *orig, char *rep, char *with)
{
  char *result; // the return string
  char *ins;    // the next insert point
  char *tmp;    // varies
  int len_rep;  // length of rep (the string to remove)
  int len_with; // length of with (the string to replace rep with)
  int len_front; // distance between rep and end of last rep
  int count;    // number of replacements

  // sanity checks and initialization
  if (!orig || !rep)
      return NULL;
  len_rep = (int)strlen(rep);
  if (len_rep == 0)
      return NULL; // empty rep causes infinite loop during count
  if (!with)
      with = "";
  len_with = (int)strlen(with);

  // count the number of replacements needed
  ins = orig;
  for (count = 0; tmp = strstr(ins, rep); ++count) {
      ins = tmp + len_rep;
  }

  tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

  if (!result)
      return NULL;

  // first time through the loop, all the variable are set correctly
  // from here on,
  //    tmp points to the end of the result string
  //    ins points to the next occurrence of rep in orig
  //    orig points to the remainder of orig after "end of rep"
  while (count--) {
      ins = strstr(orig, rep);
      len_front = ins - orig;
      tmp = strncpy(tmp, orig, len_front) + len_front;
      tmp = strcpy(tmp, with) + len_with;
      orig += len_front + len_rep; // move to next "end of rep"
  }
  strcpy(tmp, orig);
  return result;
}

int
StartsWith(char *stg, char *text)
{
  if( stg == NULL || text == NULL )
    return 0;
  return strncmp(stg, text, (int)strlen(text)) == 0;
}


char*
FTrim(char *text, const char *trimchr)
{
  if (text == NULL)
    return text;
  return RTrim((LTrim(text, trimchr)), trimchr);
}


char*
LTrim(char *text, const char *trimchr)
{
  if (text == NULL)
    return text;
  for (int ii = (int)strspn(text, trimchr); ii > 0; ii = (int)strspn(text, trimchr)) {
    int len = (int)strlen(&text[ii]);
    memmove(text, &text[ii], len);
    text[len] = '\0';
  }
  return text;
}


char*
RTrim(char *text, const char *trimchr)
{
  if (text == NULL)
    return text;
  for (char *eol = &text[strlen(text) - 1]; eol >= text;) {
    if (strchr(trimchr, *eol) != NULL)
      *eol-- = '\0';
    else
      break;              // done
  }
  return text;
}


char*
UtilBaseName(char *name)
{
  char *basename;

  if((basename = strrchr(name, PathSeparator[0])))
    ++basename;
  else
    basename = name;
  return basename;
}


char*
ExpandPath (char *dirPath)
{
  regex_t regex;
  int flen = 0;
  char *exp;
  char *path;

  char *files = (char*)malloc (2);
  memcpy (files, "\0\0", 2);

  if ((exp = strrchr (path = strdup (dirPath), PathSeparator[0]))) {
    *exp++ = '\0';    /*we have a path component */
  } else {
    free (path);
    path = strdup (".");
    exp = dirPath;    // treat as a file expression...
  }

  if (regcomp (&regex, exp, REG_EXTENDED | REG_NOSUB | REG_ICASE) < 0) {
    fprintf (stderr, "Unable to compile '%s'\n", exp);
    perror ("regcomp failed");
    free (path);
    return files;
  }

  DIR *dir = opendir (path);
  if (NULL != dir) {
    for (struct dirent * entry; NULL != (entry = readdir (dir));) {
      // Skip dots
      if (('.' == entry->d_name[0] && ('\0' == entry->d_name[1])) ||
        ('.' == entry->d_name[1] && '\0' == entry->d_name[2]))
        continue;    // do nothing

      if (exp) {
        if(regexec (&regex, entry->d_name, 0, 0, 0) == 0) {
          files = (char*)realloc(files, strlen (entry->d_name) + flen + 2);
          strcpy (&files[++flen], entry->d_name);
          flen += strlen (entry->d_name);
          files[flen + 1] = '\0';
        }
      }
    }

    closedir (dir);
  } else {
    fprintf (stderr, "opendir of %s failed; error %d", path, errno);
    files = (char*)realloc(files, 2);
    memcpy (files, "\0\0", 2);
  }

  regfree (&regex);
  free (path);
  return files;
}



#if _NONSTOP
// Returns number of files matched.
int
WalkDir (char *path, FsResolveLevel rl, WalkDirCallBack callback, void *cbarg)
{
  int       ret = 0;
  int       error;
  short     sid;

  if (callback == NULL)
    return ret;      // nothing to do

  short pathl = (short)strlen(path);
  if( (error = FILENAME_FINDSTART_(&sid, path, pathl, (short)rl)) == 0 )
    do {
      char name[BUFSIZ];
      short len;
      if( (error = FILENAME_FINDNEXT_(sid, name, sizeof(name), &len)) == 0 ) {
        name[len] = '\0';
        FsFileInfoDef *finfo = FsGetFileInfo(name);
        ToLower(name);
        int rc = (short)(*callback) (name, (void*)finfo, cbarg);
        if( rc == 0 )
          ++ret;    // increase file count
        else
          ret = rc;
      }
  } while(error == 0 && ret > 0);
  error = FILENAME_FINDFINISH_(sid);

  return ret;
}
#else
// Returns number of files matched.
int
WalkDir (char *path, FsResolveLevel rl, WalkDirCallBack callback, void *cbarg)
{
  int ret = 0;

  if (callback == NULL)
    return ret;      // nothing to do

  DIR *dir = opendir (path);
  if (NULL != dir) {
    struct dirent *entry;

    for (; NULL != (entry = readdir (dir));) {
      // Skip dots
      if(('.' == entry->d_name[0]) &&
          ('\0' == entry->d_name[1] ||
           ('.' == entry->d_name[1] && '\0' == entry->d_name[2]))) {
          continue;    // do nothing
      }

      char *target = (char*)malloc (strlen (path) + strlen (entry->d_name) +
10);
      sprintf (target, "%s/%s", path, entry->d_name);

      struct stat sbuf;
      stat(target, &sbuf);
      if ((*callback) (target, &sbuf, cbarg) < 0)
        break;    // returned an error; skip out...
      ++ret;    // increase file count
      free (target);
    }

    closedir (dir);
    dir = NULL;    // done
  } else {
    fprintf (stderr, "opendir of %s failed; error %d", path, errno);
    ret = -1;
  }

  return ret;
}
#endif


long
CnvStringLongValue(char *stg, char **endptr, int base)
{
  return strtol (stg, endptr, base);
}


int
CnvStringBooleanValue(char *val)
{
  char *eptr;
  return (val != NULL) && ((stricmp(val, "yes") == 0) ||
   (stricmp(val, "true") == 0) || (CnvStringLongValue(val, &eptr, 0) != 0));
}


char*
CnvIntegerToString(int value)
{
  static char tmp[12];
  return Sprintf(tmp, 12, "%d", value);
}


// formats an 8 byte IP address into dot notation
char*
CnvIpAddrToString(unsigned char *addr)
{
  static char tmp[32];
  return Sprintf(tmp, sizeof(tmp), "%d.%d.%d.%d",
  (unsigned int) (addr[0]),
  (unsigned int) (addr[1]), (unsigned int) (addr[2]),
  (unsigned int) (addr[3]));
}

// Returns a textual representation of error (errno)
char           *
ErrnoToString(int error)
{
  static char tmp[128];
  Sprintf(tmp, sizeof(tmp), "%s(%d)", strerror(error), error);
  return tmp;
}


int
FsParsePath(char *pname, char **components)
{
  char sep;

  if(*pname == '/' )
    sep = '/';
  else
    sep = '.';

  int idx = 0;
  char *dup = strdup(pname);
  components[idx++] = dup;    // caller must free this...

  char *cp;
  while(cp = strrchr(dup, sep)) {
    components[idx++] = (cp+1);
    *cp = '\0';
  }
  if(strlen(dup))
    components[idx++] = dup;

#if 1
    for(int i = idx; (--i > 0); ) {
      printf("%d: '%s'", i, components[i]);
      switch(i) {
        default:
          puts(": Don't know what this is");
          break;
        case 1:
          puts(": file");
          break;
        case 2:
          puts(": svol");
          break;
        case 3:
          puts(": vol");
          break;
        case 4:
          puts(": system");
          break;
      }
    }
#endif

  return idx;
}


#if _NONSTOP
char*
FsGetFileDate(char date[8])
{
  static char tmp[32];

    // [0] The Gregorian year (1984, 1985, ...)
    // [1] The Gregorian month (1-12)
    // [2] The Gregorian day of month (1-31)
    // [3] The hour of the day (0-23)
    // [4] The minute of the hour (0-59)
    // [5] The second of the minute (0-59)
    // [6] The millisecond of the second (0-999)
    // [7] The microsecond of the millisecond (0-999)
  int16_t greg[8];

  int32_t r = INTERPRETTIMESTAMP(*((long long*)date), greg);

  sprintf(tmp, "%4d-%02d-%02d %02d:%02d",
           greg[0], greg[1], greg[2], greg[3], greg[4]);

  return tmp;
}


char*
FsGetFilePerissions(char security[4])
{
  static char permissions[] = {'0', 'G', 'O', '3', 'N', 'C', 'U', '7'};
  static char tmp[8];
  #define gPerm(x) (((int)x) < 0 || ((int)x) > 7) ? '?' : permissions[((int)x)]

  sprintf(tmp, "%c%c%c%c",
      gPerm(security[0]),
      gPerm(security[1]),
      gPerm(security[2]),
      gPerm(security[3]));
  return tmp;
}

FsFileInfoDef*
FsGetFileInfo(char *name)
{
  short item_list[] = {
               34     // uint16_t  phyRecLen;
            ,  41     // uint16_t  fileType;
            ,  42     // uint16_t  fileCode;
            ,  54     // char      creationTime[8];
            ,  58     // uint16_t  owner;
            ,  62     // char      security[4];
            ,  75     // uint16_t  isopen;
            ,  142    // uint32_t  eof;
            ,  145    // char      modifyTime[8];
         };
  short number_of_items = sizeof(item_list)/sizeof(item_list[0]);
  unsigned short result_length = 0;
  short error_item = 0;
  short flen = (short)strlen(name);

  static FsFileInfoDef finfo;
  short *result_items = (short*)&finfo;
  unsigned short result_max = sizeof(finfo);

  errno = 0;

  memset(&finfo, 0, sizeof(finfo));
  short rc = FILE_GETINFOLISTBYNAME_ (name, flen,
      item_list,
      number_of_items,
      result_items,
      result_max,
      &result_length,
      &error_item);
  if(rc != 0)
    memset(&finfo, 0, sizeof(finfo));
  return &finfo;
}
#endif


BtNode_t       *FsOpenFiles = NULL;

FsFileDef*
FsOpen(char *fname, char *mode, ...)
{
  va_list ap;
  va_start (ap, mode);

  errno = 0;

  FsFileDef *file = (FsFileDef*)calloc(1, sizeof(*file));

  file->fileName = strdup(fname);
  file->mode = strdup(mode);

#if _NONSTOP
  file->_nonstop.edit.bfr = file->_nonstop.edit.bptr = NULL;
  file->_nonstop.edit.bfrLen = 0;

  memcpy(&file->_nonstop.finfo, FsGetFileInfo(file->fileName), sizeof(file->_nonstop.finfo));
  file->type = ((file->_nonstop.finfo.fileCode == 101) ?
                            (FileTypeEdit) :
                            ((stricmp(file->fileName, "$receive")) ?
                              (FileTypeUnstructured) :
                              (FileTypeReceive)
                            )
               );

  short flen = (short)strlen(file->fileName);
  short rc;

  file->fn = -1;
  short access = (
             (stricmp(file->mode, "r")==0)? 1 : // read
             (stricmp(file->mode, "w")==0)? 2 : // write
             0  // read/write
           );

  switch(file->type) {
    case FileTypeEdit:
      rc = OPENEDIT_(file->fileName, flen, &file->fn, access);
      break;
    case FileTypeReceive:
      file->_nonstop.receiveDepth = va_arg(ap, short);
      file->_nonstop.noWait = 1;
      rc = FILE_OPEN_(file->fileName, flen, &file->fn, access,, file->_nonstop.noWait, file->_nonstop.receiveDepth);
      break;
    default:
      rc = FILE_OPEN_(file->fileName, flen, &file->fn, access);
      break;
  }

  if( rc ) {
    fprintf(stderr, " %s rc=%d\n", file->fileName, rc);
    file->fn = -1;
    FsClose(file);    // release memory
    errno = rc;
    va_end (ap);
    return NULL;
  }
#else
  FILE *f = NULL;
  if((f = fopen(file->fileName, mode)) == NULL) {
    perror("fopen");
    fprintf(stderr, " %s\n", file->fileName);
    file->fn = -1;
    FsClose(file);    // release memory
    va_end (ap);
    return NULL;
  }
  file->fn = (short)fileno(f);
#endif

  va_end (ap);

  FsOpenFiles = BtInsert(FsOpenFiles,
                  (unsigned long)file->fn,
                  (unsigned long)file);
  return file;
}


int
FsOn(FsFileDef *file, char *on, int(*fnc)(FsFileDef *file, char *on, ...))
{
  FsEventDef evt;

  errno = 0;

  memset(&evt, 0, sizeof(evt));
  evt.on = ToLower(strdup(on));
  evt.fnc = fnc;

  if(!file->events) {
    file->nevents = 0;
    file->events = (FsEventDef*)calloc(file->nevents+1, sizeof(evt));
  }

  ++file->nevents;
  file->events = (FsEventDef*)realloc(file->events, (file->nevents+1)*sizeof(evt));
  memcpy(&file->events[file->nevents-1], &evt, sizeof(evt));
  memset(&file->events[file->nevents], 0, sizeof(evt));

  return 0;
}

#if 0
  if(strcmp(don, "open") == 0) {
  }
  if(strcmp(don, "close") == 0) {
  }
  if(strcmp(don, "wait") == 0) {
  }
  if(strcmp(don, "readRcv") == 0) {
  }
  if(strcmp(don, "read") == 0) {
  }
  if(strcmp(don, "readLine") == 0) {
  }
  if(strcmp(don, "reply") == 0) {
  }
#endif



int
FsClose(FsFileDef *file)
{

  errno = 0;

  if( file->fileName )
    free(file->fileName);
  if(file->mode )
    free(file->mode);

  if(file->fn >= 0) {
#if _NONSTOP
    if(file->_nonstop.edit.bfr)
      free(file->_nonstop.edit.bfr);
    if(file->_nonstop.edit.line)
      free(file->_nonstop.edit.line);

    switch(file->type) {
      case FileTypeEdit:
        CLOSEEDIT_(file->fn);
        break;
      default:
        FILE_CLOSE_(file->fn);
        break;
    }
#else
    fclose(file->f);
    file->f = NULL;              // done with this file
#endif
  }

  FsOpenFiles = BtDelete(FsOpenFiles,
                  (unsigned long)file->fn);

  file->fn = 0;

  memset(file, 0, sizeof(*file));
  free(file);
  return 0;
}


FsFileDef*
FsWait(int *fn, void *dataPtr, int *dataLen, long *tag, long waitTime)
{

  errno = 0;

#if _NONSTOP
  {
    short *_fn = (short*) fn;
    __int32_t *_dataPtr = (__int32_t*) dataPtr;
    unsigned short *_dataLen = (unsigned short*) dataLen;
    __int32_t *_tag = (__int32_t*) tag;
    __int32_t _waitTime = (__int32_t) waitTime;


  AWAITIOX (
            _fn          // IN/OUT
                         // FILE NUMBER. ( -1 IMPLIES ANY FILE.
                         // THE FILE NUMBER OF THE COMPLETING
                         // REQUEST IS RETURNED )
          , _dataPtr     // OUT OPTIONAL
                         // USER DATA BUFFER POINTER
          , _dataLen     // OUT OPTIONAL
                         // ACTUAL NUMBER OF BYTES TRANSFERRED BY
                         // OPERATION (ZERO FOR CONTROL
                         // OPERATIONS)
          , _tag         // OUT OPTIONAL
                         // REQUEST TAG PASSED WITH THE ORIGINAL
                         // REQUEST
          , _waitTime    // IN OPTIONAL
                         // AMOUNT OF TIME TO WAIT FOR A
                         // COMPLETION (0D MEANS CHECK BUT DON'T
                         // WAIT IF THERE IS NO SATISFYING
                         // COMPLETION)
          );

    *fn = (int) *_fn;
    dataPtr = (void*)_dataPtr;
    *dataLen = (int) *_dataLen;
    *tag = (long) *_tag;
  }

  if(*fn >= 0) {   // possible good file number
    short ferrno = 0;
    FILE_GETINFO_(*fn, &ferrno);
    errno = (short)ferrno;

    BtRecord_t *node = BtFind(FsOpenFiles, (unsigned long)*fn, False);
    if(node != NULL)
      return (FsFileDef*)node->value;
  }
  return NULL;      // unable to find the file (*fn has the num)

#else
  fprintf(stderr, "To be implemented\n");
  return NULL;
#endif
}


#if _NONSTOP

int
_FsReadRcv(FsFileDef *file, void *bfr, int rlen)
{
  short slen;
  short rc = 0;

  errno = 0;

  if( file->type != FileTypeReceive) {
    fprintf (stderr, "FsReadRcv from %s not supported\n", file->fileName);
    return -1;
  }

  file->_nonstop.condcode = (short)READUPDATEX(file->fn, bfr, rlen, (unsigned short*)&slen);
  if(! _status_eq(file->_nonstop.condcode)) {
    if(_status_lt(file->_nonstop.condcode))
      fprintf(stderr, "lt\n");
    else if(_status_gt(file->_nonstop.condcode))
      fprintf(stderr, "gt\n");
    rc = FILE_GETINFO_(file->fn, &file->_nonstop.ferrno);
    rc = errno = (short)file->_nonstop.ferrno;
    slen = -1;
  }
  rlen = (int)slen;
  if( (rc != 0 && rc != 6) || slen < 0 ) {
    if( rc != 1 )
      fprintf (stderr, "readupdatex of %s failed; rc=%d\n", file->fileName, rc);
    file->_nonstop.ferrno = errno = rc;
    return (rc == 1)? 0:-1;
  }

  if( rlen == 0 ) {
    int fn = file->fn;
    void *dataPtr;
    int tag;
    int len;

    FsFileDef *f = FsWait(&fn, &dataPtr, &len, &tag, -1);
    rc = errno;
    if( (rc != 0 && rc != 6) || len < 0 ) {
      if( rc != 1 )
        fprintf (stderr, "awaitiox of %s failed; rc=%d\n", file->fileName, rc);
      file->_nonstop.ferrno = errno = rc;
      return (rc == 1)? 0:-1;
    }
    rlen = (int)len;
  }

  if(rlen > 0) {
    rc = FILE_GETRECEIVEINFO_(file->_nonstop.receiveInfo);
    if( rc != 0 ) {
      if( rc != 1 )
        fprintf (stderr, "FILE_GETRECEIVEINFO_ of %s failed; rc=%d\n", file->fileName, rc);
      file->_nonstop.ferrno = errno = rc;
      return (rc == 1)? 0:-1;
    }
  }

  return rlen;
}


int
FsReadRcv(FsFileDef *file, void *bfr, int rlen, int mtype)
{
  int done = 0;
  int len;

  while( ! done && (len = _FsReadRcv(file, bfr, rlen)) >= 0 ) {
    zsys_ddl_smsg_def *msg = (zsys_ddl_smsg_def*)bfr;
    ((char*)bfr)[len] = '\0';
    int type = msg->u_z_msg.z_msgnumber[0];

    if( ! (done = (mtype == 0) || (mtype == type)))
      (void)FsReply(file, bfr, 0, 0, 0);
  }

  return len;
}
#endif


int
FsRead(FsFileDef *file, void *bfr, int rlen)
{
#if _NONSTOP
  short slen;
  short rc;

  errno = 0;

  for(int e = 0; e < file->nevents; ++e) {
    FsEventDef *evt = &file->events[e];
    if(strcmp(evt->on, "read") == 0)
      (*evt->fnc)(file, "read");
  }

  switch(file->type) {
    case FileTypeEdit:
      rc = READEDIT(file->fn,
        ,                     // rec-no
        bfr,                  // contains text read
        rlen-1,               // length of input buffer
        &slen);               // number of bytes read
      rlen = (int)slen;
      if( rc != 0 || rlen < 0 ) {
        rlen = 0;
        if( rc != 1 )
          fprintf (stderr, "read of %s failed; rc=%d\n", file->fileName, rc);
        file->_nonstop.ferrno = errno = rc;
        return (rc == 1)? 0:-1;
      }
      ((char*)bfr)[rlen++] = '\n';     // nl
      break;

    case FileTypeReceive:
      rlen = _FsReadRcv(file, bfr, rlen);
      break;

    default:
      file->_nonstop.condcode = (short)READX(file->fn, bfr, rlen, (unsigned short*)&slen);
      if(! _status_eq(file->_nonstop.condcode)) {
        rc = FILE_GETINFO_(file->fn, &file->_nonstop.ferrno);
        rc = errno = (short)file->_nonstop.ferrno;
        slen = -1;
      }
      rlen = (int)slen;
      if( (rc != 0 && rc != 6) || rlen < 0 ) {
        if( rc != 1 )
          fprintf (stderr, "read of %s failed; rc=%d\n", file->fileName, rc);
        file->_nonstop.ferrno = errno = rc;
        return (rc == 1)? 0:-1;
      }
      break;
  }
#else
  rlen = read(file->fn, bfr, rlen);
  if( rlen < 0 ) {
    fprintf (stderr, "read of %s failed; rc=%d\n", file->fileName, rlen);
    return -1;
  }
#endif
  return rlen;
}


int
FsWrite(FsFileDef *file, void *bfr, int wlen)
{
  int   rc;

  errno = 0;

#if _NONSTOP
  switch(file->type) {
    case FileTypeUnstructured:
      file->_nonstop.condcode = (short)WRITEX(file->fn, bfr, wlen);
      if(! _status_eq(file->_nonstop.condcode)) {
        rc = FILE_GETINFO_(file->fn, &file->_nonstop.ferrno);
        rc = errno = (short)file->_nonstop.ferrno;
      }
      if( rc != 0 ) {
        if( rc != 1 && rc != 70)
          fprintf (stderr, "write to %s failed; rc=%d\n", file->fileName, rc);
        file->_nonstop.ferrno = errno = (short)rc;
        return (rc == 1)? 0:-1;
      }
      break;

    default:
      fprintf (stderr, "write to %s not supported\n", file->fileName);
      wlen = -1;
      break;
  }
#else
  wlen = write(file->fn, bfr, wlen);
  if( wlen < 0 ) {
    fprintf (stderr, "write of %s failed; rc=%d\n", file->fileName, errno);
    return -1;
  }
#endif

  return wlen;
}


#if _NONSTOP
int
FsWriteRead(FsFileDef *file, void *bfr, int wlen, int rsize)
{
  int   rc;
  short slen;
  int   rlen;

  errno = 0;

  switch(file->type) {
    case FileTypeUnstructured:
      rc = WRITEREADX(file->fn, bfr, wlen, rsize, (unsigned short*)&slen);
      rlen = (int)slen;
      rc = FILE_GETINFO_(file->fn, &file->_nonstop.ferrno);
      rc = errno = (short)file->_nonstop.ferrno;
      if( rc != 0 ) {
        if( rc != 1 && rc != 70)
          fprintf (stderr, "writeread to %s failed; rc=%d\n", file->fileName, rc);
        file->_nonstop.ferrno = errno = (short)rc;
        return (rc == 1)? 0:-1;
      }
      break;

    default:
      fprintf (stderr, "writeread to %s not supported\n", file->fileName);
      rlen = -1;
      break;
  }

  return rlen;
}


int
FsReply(FsFileDef *file, void *bfr, int wlen, int tag, int ret)
{
  int   rc;
  short slen;

  errno = 0;

  switch(file->type) {
    case FileTypeReceive:
      if( tag == 0 )
        tag = file->_nonstop.receiveInfo[2];
      rc = REPLYX(bfr, wlen, (unsigned short*)&slen, tag, ret);
      wlen = (int)slen;
      if( rc != 0 || wlen < 0 ) {
        if( rc != 1 )
          fprintf (stderr, "reply to %s failed; rc=%d\n", file->fileName, rc);
        file->_nonstop.ferrno = errno = (short)rc;
        return (rc == 1)? 0:-1;
      }
      break;

    default:
      fprintf (stderr, "reply to %s not supported\n", file->fileName);
      wlen = -1;
      break;
  }

  return wlen;
}
#endif

char*
FsReadLine(FsFileDef *file)
{

  errno = 0;

#if _NONSTOP
  if( file->_nonstop.edit.bfr == NULL ) {   // need to init
    file->_nonstop.edit.bfr = calloc(1, BUFSIZ);
    file->_nonstop.edit.line = calloc(1, BUFSIZ);
    file->_nonstop.edit.bptr = NULL;
    file->_nonstop.edit.bfrLen = 0;
  }

  // fill the line until a \r\n or \0
  strcpy(file->_nonstop.edit.line, "");   // empty
  for(;;) {
    char *lptr = file->_nonstop.edit.line;
    char *eol;
    int  ll;

    if(file->_nonstop.edit.bfrLen <= 0) {        // buffer is empty
      char  tmp[BUFSIZ];
      short rlen = 0;

      file->_nonstop.edit.bptr = file->_nonstop.edit.bfr;

      rlen = FsRead(file, tmp, sizeof(tmp));
      if(rlen < 0 ) {
        fprintf (stderr, "read failed\n");
        return NULL;
      }
      tmp[rlen] = '\0';
      // change \r\n to \n
      for( char *eptr = tmp; (eptr = strstr(eptr, "\r\n")); ) {
        strcpy(eptr, eptr+1);   // ditch the \r
        --rlen;
      }
      if( (file->_nonstop.edit.bfrLen = rlen) <= 0 )
        return strlen(file->_nonstop.edit.line) <= 0 ? NULL: file->_nonstop.edit.line;
      memcpy(file->_nonstop.edit.bfr, tmp, file->_nonstop.edit.bfrLen);
      file->_nonstop.edit.bfr[file->_nonstop.edit.bfrLen] = '\0';
    }

    // process the saved buffer
    if( (eol = strchr(file->_nonstop.edit.bptr, '\n')) == NULL )
      eol = &file->_nonstop.edit.bptr[file->_nonstop.edit.bfrLen-1];
    ll = (eol - file->_nonstop.edit.bptr) + 1;
    memcpy(lptr, file->_nonstop.edit.bptr, ll);
    lptr += ll;
    *lptr = '\0';
    file->_nonstop.edit.bptr += ll;
    file->_nonstop.edit.bfrLen -= ll;
    if( eol[0] == '\n' || eol[1] == '\0' )
      break;
  }

  return file->_nonstop.edit.line;

#else
  fprintf(stderr, "To be implemented\n");
  return NULL;
#endif
}


// tandem version is broken...
// this one uses 'y' as an integer
// good enough for this 'use case'
double Pow(double x, double y)
{
  double p = x;
  for(int i = (int)(y+0.5); i > 1; --i)
    p = p * x;
  return p;
}

// Function to swap two numbers
void swap(char *x, char *y) {
  char t = *x; *x = *y; *y = t;
}

// Function to reverse `buffer[i.j]`
char*
reversestr(char *buffer, int i, int j)
{
  while (i < j) {
    swap(&buffer[i++], &buffer[j--]);
  }

  return buffer;
}

// Iterative function to implement `itoa()` function in C
char*
itoa(int value, char *buffer, int base)
{
  static char str[32];

  if( buffer == NULL )
    buffer = str;  // use internal

 // invalid input
  if (base < 2 || base > 32) {
    return buffer;
  }

 // consider the absolute value of the number
  int n = abs(value);

  int i = 0;
  while (n) {
   int r = n % base;

   if (r >= 10) {
     buffer[i++] = (char)('A' + (r - 10));
   }
   else {
     buffer[i++] = (char)('0' + r);
   }

   n = n / base;
  }

  // if the number is 0
  if (i == 0) {
    buffer[i++] = '0';
  }

  // If the base is 10 and the value is negative, the resulting string
  // is preceded with a minus sign (-)
  // With any other base, value is always considered unsigned
  if (value < 0 && base == 10) {
    buffer[i++] = '-';
  }

  buffer[i] = '\0'; // null terminate string

  // reverse the string and return it
  return reversestr(buffer, 0, i - 1);
}


// tandem printf for floats is broken
char*
ftoa(double x, char *out)
{
  static char str[32];
  int n,i=0,k=0;

  if( out == NULL )
    out = str;    // use internal space

  n = (int)x;
  while(n > 0) {
    x /= 10;
    n=(int)x;
    ++i;
  }

  *(out+i) = '.';
  x *= 10;
  n = (int)x;
  x = x - (double)n;
  while((n > 0)||(i > k)) {
    if(k == i)
      ++k;
    *(out+k) = (char)('0' + n);
    x *= 10;
    n = (int)x;
    x = x - (double)n;
    ++k;
  }

  *(out+k) = '\0';
  return out;
}

uint32_t
lz77_compress(uint8_t *u_text,
              uint32_t u_size,
              uint8_t *lz_text,
              uint8_t pointer_length_width)
{
  uint16_t  pointer_pos,
            temp_pointer_pos,
            output_pointer,
            pointer_length,
            temp_pointer_length;
  uint32_t  lz_pointer,
            output_size,
            coding_pos,
            output_lookahead_ref,
            look_behind,
            look_ahead;

  uint16_t pointer_pos_max,
           pointer_length_max;

  pointer_pos_max = (uint16_t)Pow(2.0, (double)(16 - pointer_length_width));
  pointer_length_max = (uint16_t)Pow(2.0, (double)pointer_length_width);

  *((uint32_t *) lz_text) = u_size;
  *(lz_text + 4) = pointer_length_width;
  lz_pointer = output_size = 5;

  for(coding_pos = 0; coding_pos < u_size; ++coding_pos) {
    pointer_pos = 0;
    pointer_length = 0;
    for(temp_pointer_pos = 1; (temp_pointer_pos < pointer_pos_max) && (temp_pointer_pos <= coding_pos); ++temp_pointer_pos) {
      look_behind = coding_pos - temp_pointer_pos;
      look_ahead = coding_pos;
      for(temp_pointer_length = 0; u_text[look_ahead++] == u_text[look_behind++]; ++temp_pointer_length) {
        if(temp_pointer_length == pointer_length_max)
          break;
      }
      if(temp_pointer_length > pointer_length) {
        pointer_pos = temp_pointer_pos;
        pointer_length = temp_pointer_length;
        if(pointer_length == pointer_length_max)
          break;
      }
    }
    coding_pos += pointer_length;
    if((coding_pos == u_size) && pointer_length) {
      output_pointer = (uint16_t)((pointer_length == 1) ? 0 : ((pointer_pos << pointer_length_width) | (pointer_length - 2)));
      output_lookahead_ref = coding_pos - 1;
    }
    else
    {
      output_pointer = (uint16_t)((pointer_pos << pointer_length_width) | (pointer_length ? (pointer_length - 1) : 0));
      output_lookahead_ref = coding_pos;
    }
    *((uint16_t *) (lz_text + lz_pointer)) = output_pointer;
    lz_pointer += 2;
    *(lz_text + lz_pointer++) = *(u_text + output_lookahead_ref);
    output_size += 3;
  }

  return output_size;
}

uint32_t
lz77_decompress(uint8_t *lz_text,
                uint8_t *u_text)
{
  uint8_t pointer_length_width;
  uint16_t input_pointer,
           pointer_length,
           pointer_pos,
           pointer_length_mask;
  uint32_t lz_pointer,
           coding_pos,
           pointer_offset,
           u_size;

  u_size = *((uint32_t *) lz_text);
  pointer_length_width = *(lz_text + 4);
  lz_pointer = 5;

  pointer_length_mask = (uint16_t)Pow(2.0, (double)(pointer_length_width)) - 1;

  for(coding_pos = 0; coding_pos < u_size; ++coding_pos) {
    input_pointer = *((uint16_t *) (lz_text + lz_pointer));
    lz_pointer += 2;
    pointer_pos = (uint16_t)(input_pointer >> pointer_length_width);
    pointer_length = (uint16_t)(pointer_pos ? ((input_pointer & pointer_length_mask) + 1) : 0);
    if(pointer_pos)
        for(pointer_offset = coding_pos - pointer_pos; pointer_length > 0; --pointer_length) {
          u_text[coding_pos++] = u_text[pointer_offset++];
        }
    *(u_text + coding_pos) = *(lz_text + lz_pointer++);
  }

  return coding_pos;
}


uint32_t
fsize(int in)
{
  uint32_t pos = lseek(in, 0L, SEEK_CUR);    // current pos
  uint32_t length = lseek(in, 0L, SEEK_END);  // end of file
  pos = lseek(in, (long)pos, SEEK_SET);      // back to starting point
  return length;
}

int32_t
file_lz77_compress(char *filename_in,
                   char *filename_out,
                   uint8_t pointer_length_width)
{
  int in, out;
  uint8_t *raw_text, *lz_text;
  uint32_t raw_size, lz_size;
  uint32_t rlen;

  in = open(filename_in, O_RDONLY);
  if(in < 0) {
    fprintf(stderr, "Unable to open %s(%d)\n", filename_in, errno);
    perror("open");
    return -1;
  }
  raw_size = fsize(in);
  raw_text = (uint8_t*)malloc(raw_size*2);
  rlen = read(in, raw_text, raw_size);
  if(rlen != raw_size) {
    fprintf(stderr, "Unable to read %s(%d)\n", filename_in, errno);
    perror("read");
    free(raw_text);
    return -1;
  }
  close(in);

  lz_text = (uint8_t*)malloc(raw_size*4);

  lz_size = lz77_compress(raw_text, raw_size, lz_text, pointer_length_width);

  out = open(filename_out, O_WRONLY|O_CREAT, 0666);
  if(out < 0) {
    fprintf(stderr, "Unable to open %s(%d)\n", filename_out, errno);
    perror("open");
    free(lz_text);
    free(raw_text);
    return -1;
  }
  if((lz_size != write(out, lz_text, lz_size))) {
    fprintf(stderr, "Unable to write %s(%d)\n", filename_out, errno);
    perror("write");
    free(lz_text);
    free(raw_text);
    return -1;
  }
  close(out);

  free(lz_text);
  free(raw_text);
  return (int32_t)lz_size;
}

int32_t
file_lz77_decompress(char *filename_in, char *filename_out)
{
  int in, out;
  uint32_t lz_size, raw_size;
  uint8_t *lz_text, *raw_text;
  uint32_t rlen;

  in = open(filename_in, O_RDONLY);
  if(in < 0) {
    fprintf(stderr, "Unable to open %s(%d)\n", filename_in, errno);
    perror("open");
    return -1;
  }
  lz_size = fsize(in);
  lz_text = (uint8_t*)malloc(lz_size);
  rlen = read(in, lz_text, lz_size);
  if(rlen != lz_size) {
    fprintf(stderr, "Unable to read %s(%d)\n", filename_in, errno);
    perror("read");
    free(lz_text);
    return -1;
  }
  close(in);

  raw_size = *((uint32_t *) lz_text);
  raw_text = (uint8_t*)malloc(raw_size);

  if(lz77_decompress(lz_text, raw_text) != raw_size)
    return -1;

  out = open(filename_out, O_WRONLY|O_CREAT, 0666);
  if(out < 0) {
    fprintf(stderr, "Unable to open %s(%d)\n", filename_out, errno);
    perror("open");
    free(lz_text);
    free(raw_text);
    return -1;
  }
  if(write(out, raw_text, raw_size) != raw_size) {
    fprintf(stderr, "Unable to write %s(%d)\n", filename_out, errno);
    perror("write");
    free(lz_text);
    free(raw_text);
    return -1;
  }
  close(out);

  free(lz_text);
  free(raw_text);
  return (int32_t)raw_size;
}


uint64_t
ComputeSum(uint8_t *s, uint32_t len, uint64_t sum_value)
{
  int p = 31;
  int m = 1e9 + 9;
  uint64_t p_pow = 1;

  for(uint8_t *c = s; len > 0; --len) {
    sum_value = (sum_value + (*c - 'a' + 1) * p_pow) % m;
    p_pow = (p_pow * p) % m;
  }

  return sum_value;
}


#define BYTES_PER_LINE    16

void
_FormatLine(long offset, char *mem, char *obfr, int len)
{
 static char    *asciiDigit = { "0123456789ABCDEF" };
 char           *op;
 char           *ip;
 int             nb;
 int             value;

 /*
  * op = &obfr[sprintf (obfr, "%6lX: ", offset)];
  *//*
  * offset
  */

 sprintf(obfr, "%6lX: ", offset);
 op = &obfr[6];

 /*
  * format hex part of line
  */

 ip = &mem[0];

 for (nb = 0; nb < len && nb < BYTES_PER_LINE; ++nb) {
  if ((nb % 8) == 0 && nb != 0) {
   *op++ = ' ';
   *op++ = ':';  /* put a : every 8 bytes */
  }
  *op++ = ' ';   /* separator */
  value = *ip++ & 0xFF;
  *op++ = asciiDigit[(value >> 4) & 0x0F];
  *op++ = asciiDigit[value & 0x0F];
 }

 /*
  * format ascii part of line
  */

 *op = '\0';
 for (nb = (int)strlen(obfr); nb < 60; ++nb)
  *op++ = ' ';   /* move to 60th column */

 ip = &mem[0];

 for (nb = 0; nb < BYTES_PER_LINE && nb < len; ++nb) {
  int ch = (*ip++) & 0x7F; /* lower 7 bits */
  if (!isprint(ch))
   ch = '.';   /* not printable */
  *op++ = (char)ch;
 }

 *op = '\0';     /* terminate the line */
}


int
HexDump(
  int (*oFnc)(char*, void*), // function to handle formatted lines
  void *oFncArg,   // the argument to pass to the user function
  void *mem,       // data to dump
  int len,         // number of bytes
  long offset      // starting offset used in output
 )
{
  int   status;
  int   nl;
  int   dupLine;
  char  fLine[128];
  char  *ptr = (char*)mem;


  if (len <= 0)
    return 0;    // nothing to dump

  if( oFnc == NULL )
    oFnc = (int (*)(char*, void*))puts;

  status = nl = dupLine = 0;

  do {
    if (nl) {
      if (memcmp(&ptr[-BYTES_PER_LINE], &ptr[0], BYTES_PER_LINE) == 0) {
        ++dupLine;
        goto nextLine;
      }
    }

    /*
     * Current line is different from previous line
    */

    if (dupLine) {   // previously detected dup lines
      if (--dupLine) { // And more than one
        sprintf(fLine, "          %u duplicate lines.", dupLine);
        if ((status = (*oFnc) (fLine, oFncArg)) < 0)
          break;  // user function reported error
      }

      // force printing of the duplicate line
      offset -= BYTES_PER_LINE;
      ptr -= BYTES_PER_LINE;
      len += BYTES_PER_LINE;
      --nl;
      dupLine = 0;
    }

    _FormatLine(offset, ptr, fLine, len);
    if ((status = (*oFnc) (fLine, oFncArg)) < 0)
      break;    // user function reported error

nextLine:
    offset += BYTES_PER_LINE;
    ptr += BYTES_PER_LINE;
    len -= BYTES_PER_LINE;
    ++nl;     // count the line
  } while (len > 0);


  /*
    Flush any saved duplicate lines
  */

  if(status == 0 && dupLine) {
    if (--dupLine) {
      sprintf(fLine, "          %u duplicate lines.", dupLine);
      if ((status = (*oFnc) (fLine, oFncArg)) < 0)
        return status;
    }

    /*
     * force printing of the duplicate line
    */
    offset -= BYTES_PER_LINE;
    ptr -= BYTES_PER_LINE;
    len += BYTES_PER_LINE;
    --nl;
    dupLine = 0;

    _FormatLine(offset, ptr, fLine, len);
    if ((status = (*oFnc) (fLine, oFncArg)) < 0)
      return status; // user function reported error
  }

  return status;
}

#if _NONSTOP
UserDef*
GetUser(char *id)
{
  static UserDef user;
  short   rc;
  short   user_name_len;
  short   dvol_len;
  short   initdir_len;
  short   initprog_len;
  short   desc_txt_len;
  short   desc_bin_len;

  if( ! id )
    return NULL;    // nothing...

  memset(&user, 0, sizeof(user));
  user_name_len = 0;

  if( isdigit(*id) ) {    // search by gid,uid
    char    tmp[49];
    strcpy(tmp, id);
    char *ptr = strchr(tmp, ',');
    if( ! ptr )
      return NULL;    // bad format, not going to work.
    *ptr++ = '\0';    // cut at the comma
    user.uidgid.giduid.v.gid = (uint8_t)atoi(tmp);
    user.uidgid.giduid.v.uid = (uint8_t)atoi(ptr);
  }
  else { // alpha input.. get by name
    strcpy(user.user_name, id);
    user_name_len = (short)strlen(user.user_name);
  }

  rc = USER_GETINFO_ (
        user.user_name             // char *user_name
      , sizeof(user.user_name)     // short user_maxlen
      , &user_name_len              // short *user_curlen
      , &user.uidgid.user_id      // __int32_t *user_id
      , &user.isAlias             // short *is_alias
      ,                           // short *group_count
      ,                           // __int32_t *group_list
      ,                           // __int32_t *primary_group
      , user.default_vol          // char *volsubvol
      , sizeof(user.default_vol)  // short volsubvol_maxlen
      , &dvol_len                 // short *volsubvol_len
      , user.initdir              // char *initdir
      , sizeof(user.initdir)      // short initdir_maxlen
      , &initdir_len              // short *initdir_len
      , user.initprog             // char *initprog
      , sizeof(user.initprog)     // short initprog_maxlen
      , &initprog_len             // short *initprog_len
      , &user.default_security    // short *default_security
      , user.desc_txt             // char *desc_txt
      , sizeof(user.desc_txt)     // short desc_txt_maxlen
      , &desc_txt_len             // short *desc_txt_len
      , user.desc_bin             // char *desc_bin
      , sizeof(user.desc_bin)     // short desc_bin_maxlen
      , &desc_bin_len             // short *desc_bin_len
                     );
  if( rc != 0 )
    return NULL;
  user.user_name[user_name_len] = '\0';
  user.default_vol[dvol_len] = '\0';
  user.initdir[initdir_len] = '\0';
  user.initprog[initprog_len] = '\0';
  user.desc_txt[desc_txt_len] = '\0';
  user.desc_bin[desc_bin_len] = '\0';
  return &user;
}


UserDef**
GetUsers(char *id)   // returns null terminated list of UserDef*
{
  UserDef **users;
  int     uc;
  uint8_t vgid = 0;
  uint8_t vuid = 0;
  short   rc = 0;
  char    user_name[128];
  short   user_name_len = 0;
  short   isAlias = 0;

  if( ! id )
    return NULL;    // nothing...

  if( isdigit(*id) ) {    // search by gid,uid
    char    tmp[49];
    strcpy(tmp, id);
    char *ptr = strchr(tmp, ',');
    if( ptr ) {
      *ptr++ = '\0';    // cut at the comma
      vuid = (uint8_t)atoi(ptr);
    }
    vgid = (uint8_t)atoi(tmp);
  }

// initial empty ptr
  uc = 0;
  users = (UserDef**)calloc(uc+1, sizeof(*users));

  strcpy(user_name, "");   // from the first
  do {
    user_name_len = (short)strlen(user_name);
    rc = USER_GETNEXT_(user_name, sizeof(user_name), &user_name_len, &isAlias);
    if( rc == 0 ) {
      user_name[user_name_len] = '\0';
      UserDef *user = GetUser(user_name);
      if( user == NULL )
        continue;         // not available, skip
      if( isdigit(*id) ) {      // by gid,uid
          if( user->uidgid.giduid.v.gid != vgid ||
            (vuid != 0 && user->uidgid.giduid.v.uid != vuid) )
              user = NULL;    // no match
      } else {
        if(stristr(user->user_name, id) == NULL)
          user = NULL;        // no match
      }
      if( user == NULL )
        continue;         // no match, skip

      // add to ptr list
      users[uc] = (UserDef*)calloc(1, sizeof(*user));
      memcpy(users[uc], user, sizeof(*user));
      ++uc;               // one more
      users = (UserDef**)realloc(users, (uc+1)*sizeof(*users));
    }
  } while(rc == 0);

  return users;
}
#endif


int
GrepLine(GrepDef *def, char *bfr, int rawMatch)
{

  def->isMatch = False;

  if( def->isDebug )
    printf("%s\n", bfr);

  if( rawMatch ) {          // use the passed flag
    char *cp;
    if(def->ignoreCase)
      cp = stristr(bfr, def->exp);
    else
      cp = strstr(bfr, def->exp);

    if( cp ) {  // found expression
      if( strlen(def->matchWord) <= 0 ) // ?word mode...
        def->isMatch = cp != NULL;  // not word mode: is matched
      else {    // word mode...
        char fc = (char)tolower(cp[-1]);
        char lc = (char)tolower(cp[strlen(def->exp)]);

        if( ((cp == bfr)                    // start of bfr
          || ! isalnum(fc))
          && ! isalnum(lc) )
        {
           def->isMatch = True;
        }
      }
    }
  } else {
    def->isMatch = (regexec(&def->regexp, bfr, 0, 0, 0) == 0);
  }

  def->isMatch = (def->invertMatch && !def->isMatch) || (def->isMatch && !def->invertMatch);

  if(def->displayLinesForced) {
    if(def->isMatch)
      ++def->matchCnt;
    if(def->displayLineNbrs)
      printf("%8d: ", def->lineNbr);
    puts(bfr);
    if(--def->displayLinesForced < 0)
      def->displayLinesForced = 0;
    def->isMatch = False;  // omit further matching logic
  }

  if(def->isMatch) {
    ++def->matchCnt;

    if( def->displayNamesOnly) {
      printf("%s\n", def->fileName);
      return 1;       // Done... and get next file
    }

    if(def->displayCountsOnly)
      return 0;       // Done... and get next bfr

    if( def->showFileName )
      printf("%s:", def->fileName);

    // if 'before' option, display saved lines
    if(def->displayLinesBefore) {
      int ln = def->lineNbr - def->displayLinesBefore;
      int i = def->savedLines;
      do {
        if( ! def->quiet ) {
          if(def->displayLineNbrs)
            printf("%8d: ", ln);
          puts(def->bfrs[i]);
        }
    ++ln;
        if(++i >= def->displayLinesBefore)
          i = 0;  // wrap around
      } while(i != def->savedLines);
    }

  // Display the matching line
    if( ! def->quiet ) {
      if(def->displayLineNbrs)
        printf("%8d: ", def->lineNbr);

      puts(bfr);
  }
    if(def->displayLinesAfter)
      def->displayLinesForced = def->displayLinesAfter;
  }

  if(def->displayLinesBefore) {
    strcpy(def->bfrs[def->savedLines], bfr);  // save the bfr
    if(++def->savedLines >= def->displayLinesBefore)
      def->savedLines = 0;  // wrap around
  }

  return 0;
}


int
GrepFile(GrepDef *def, char *fileName)
{
  int rawMatch = def->rawMatch;
  char tmp[BUFSIZ];
  int  rlen;

  if( def->shortNames )
    def->fileName = UtilBaseName(fileName);
  else
    def->fileName = fileName;

  if( def->isDebug )
    printf("%s\n", fileName);

  FsFileDef *file = FsOpen(fileName, "r");
  if( file == NULL ) {
    fprintf(stderr, "Unable to open %s\n", fileName);
    perror("FsOpen");
    return -1;
  }

  def->lineNbr = 0;
  def->isMatch = 0;
  def->matchCnt = 0;
  def->displayLinesForced = 0;

  // Determine how many lines to save and allocate buffers

  if(def->displayLinesBefore) {
    if((def->bfrs = (char**)calloc(def->displayLinesBefore,
                             sizeof(def->bfrs[0]))) == NULL)
      Fatal("%s: Unable to allocate enough memory for buffers", "grep");
    for(int i = 0; i < def->displayLinesBefore; ++i) {
      if((def->bfrs[i] = (char*)calloc(1, BUFSIZ + 1)) == NULL)
        Fatal("%s: Unable to allocate enough memory for buffers", "grep");
    }
  }

  if( file->type != FileTypeEdit )
    rawMatch = True;   // if not an edit file, set raw

  // compile the expression
  if(! rawMatch) {
    sprintf(def->exp, "%s%s%s", def->matchWord, def->pattern, def->matchWord);
    int rc = regcomp(&def->regexp, def->exp, REG_EXTENDED | REG_NOSUB | def->ignoreCase);
    if(rc < 0) {
      fprintf(stderr, "Unable to compile '%s'\n", def->exp);
      perror("regcomp failed");
      return -1;
    }
  }
  else {
    strcpy(def->exp, def->pattern);
  }

  for(char *bfr; file != NULL;) {
    switch(file->type) {
      case FileTypeEdit:
        bfr = FsReadLine(file);
        break;

      default:
        if( (rlen = FsRead(file, tmp, sizeof(tmp))) <= 0 )
          bfr = NULL;   // end of file
        else {
          for(int i = 0; i < rlen; ++i) // Replace non printable chs
            if( ! isprint(tmp[i]) )
              tmp[i] = ' ';
          tmp[rlen] = '\0';
          bfr = tmp;
        }
        break;
    }

// grep the input

    if( ! bfr )
      break;          // hit end

    ++def->lineNbr;
    bfr = FTrim(bfr, "\n\r");
    int r = GrepLine(def, bfr, rawMatch);
    if( r || (def->quiet && def->isMatch) ) {
  // finished with this guy, close it
      FsClose(file);
      file = NULL;
    }
  }    // until EOF

  if(def->displayCountsOnly)
    printf("%s:%d\n", def->fileName, def->matchCnt);

  if( file )
    FsClose(file);
  file = NULL;

  if( def->isMatch )
    ++def->totalMatch;      // count it

  // release save buffers

  if(def->displayLinesBefore) {
    for(int i = 0; i < def->displayLinesBefore; ++i)
      free(def->bfrs[i]);
    free(def->bfrs);
  }

  if(! rawMatch)
    regfree(&def->regexp);

  return def->isMatch;
}

// If string not found; returns NULL
EnumToStringMap_t *
EnumMapStringToVal(EnumToStringMap_t * map, char *text, char *prefix)
{

  if (map == NULL || text == NULL)
    return NULL;   // can't map

  EnumToStringMap_t *mp = NULL;

  for (mp = map; mp->stg != NULL; ++mp) {
    if (stricmp(text, mp->stg) == 0)
      break;
  }

  if (mp->stg == NULL && prefix != NULL && strlen(prefix) > 0) {
   // Ok, that didn't work; Add optional prefix and try again
    char *p = (char*)malloc(strlen(text) + strlen(prefix) + 10);

    sprintf(p, "%s%s", prefix, text);
    for (mp = map; mp->stg != NULL; ++mp) {
      if (stricmp(p, mp->stg) == 0)
        break;
    }
    free(p);
  }

  if (mp->stg == NULL)
    return NULL;

  return mp;
}


// If value not found; returns NULL
EnumToStringMap_t *
EnumMapValToString(EnumToStringMap_t * map, int val)
{

  EnumToStringMap_t *mp = NULL;

  for (mp = map; mp->stg != NULL; ++mp) {
    if (mp->en == val)
      break;
  }

  return mp;
}


static EnumToStringMap_t SignalStringMap[] = {
 {SIGHUP, "SIGHUP: Hangup"},
 {SIGINT, "SIGINT: Interrupt"},
 {SIGQUIT, "SIGQUIT: Quit"},
 {SIGILL, "SIGILL: Illegal instruction"},
#if ! _NONSTOP
 {SIGTRAP, "SIGTRAP: Trace trap"},
 {SIGIOT, "SIGIOT: IOT trap"},
 {SIGBUS, "SIGBUS: BUS error"},
 {SIGXCPU, "SIGXCPU: CPU limit exceeded"},
 {SIGXFSZ, "SIGXFSZ: File size limit exceeded"},
 {SIGVTALRM, "SIGVTALRM: Virtual alarm clock"},
 {SIGPROF, "SIGPROF: Profiling alarm clock"},
 {SIGPWR, "SIGPWR: Power failure restart"},
 {SIGSYS, "SIGSYS: Bad system call"},
#endif
 {SIGABRT, "SIGABRT: Abort"},
 {SIGFPE, "SIGFPE: Floating-point exception"},
 {SIGKILL, "SIGKILL: Kill, unblockable"},
 {SIGUSR1, "SIGUSR1: User-defined signal 1"},
 {SIGSEGV, "SIGSEGV: Segmentation violation"},
 {SIGUSR2, "SIGUSR2: User-defined signal 2"},
 {SIGPIPE, "SIGPIPE: Broken pipe"},
 {SIGALRM, "SIGALRM: Alarm clock"},
 {SIGTERM, "SIGTERM: Termination"},
 {SIGCHLD, "SIGCHLD: Child status has changed"},
 {SIGCONT, "SIGCONT: Continue"},
 {SIGSTOP, "SIGSTOP: Stop, unblockable"},
 {SIGTSTP, "SIGTSTP: Keyboard stop"},
 {SIGTTIN, "SIGTTIN: Background read from tty"},
 {SIGTTOU, "SIGTTOU: Background write to tty"},
 {SIGURG, "SIGURG: Urgent condition on socket"},
 {SIGWINCH, "SIGWINCH: Window size change"},
 {SIGIO, "SIGIO: I/O now possible"},
 {-1, NULL}
};


char*
NxSignalNbrToString(int signo)
{
  EnumToStringMap_t *map;

  if ((map = EnumMapValToString(SignalStringMap, signo)) == NULL) {
    static char     tmp[25];
    sprintf(tmp, "InvalidSigNo(%d)", signo);
    return tmp;
  }
  return map->stg;
}


/*
 * + Name: IntFromString(2) - Convert an ascii number to binary
 *
 * Synopsis: unsigned long long IntFromString (char *num, int *err);
 *
 * Description: Convert ascii string into a binary int.  Assumes the value is in decimal unless prefixed with a 0x which then
 * indicates a hex value.  If the
     value has a leading zero, octal will not be assumed.  The ascii value may have a leading sign
 * character to express a positive or negative value.  Any leading spaces are ignored.
 *
 * 10 - is a decimal ten -10 - is
     a negative decimal ten 0x10 - is a hex 10 or decimal 16 -0x10 - is a negative hex 10 010 - is
 * a decimal 10 -010 - is an negative octal 10
 *
 * Returns the converted binary value or 0xdeadbeef -
 */

unsigned long long
IntFromString(char *num, int *err)
{
 int  errBoolean;

 if (err == NULL)
  err = &errBoolean;    // not used...
 *err = 0;

 num = &num[strspn(num, " \t")]; /* skip leading whitespace */

 /*
  * check for leading sign character
  */

 char            sign;

 if (*num == '-' || *num == '+')
  sign = *num++;
 else
  sign = '+';

 num = &num[strspn(num, " \t")]; /* skip leading whitespace */

 if (!isdigit(*num)) {
  *err = 1;
  return (0xdeadbeef); /* not a number */
 }

 /*
  * check for leading radix indicator
  */

 int             radix;

 if (*num == '0' && tolower(num[1]) == 'x') {
  radix = 16;
  num += 2;
 } else {
  radix = 10;
 }

 unsigned long long result = 0;

 for (int digit; isxdigit(*num);) {
  digit = toupper(*num++);
  digit -= '0';
  if (digit > 9) {  /* hex range */
   if (radix != 16) /* not hex radix */
    break;
   digit -= (('A' - '9') - 1);
  } else if (digit > 7 && radix == 8) {
   break;    /* non octal digit */
  }

  result = (result * radix) + digit;
 }

 if (sign == '-')   /* negative value */
  result = 0 - result;

 return (result);
}


unsigned long long
IntFromHexString(char *num, int *err)
{
 char  *hex = (char*)calloc(1, strlen(num) + 3);

 sprintf(hex, "0x%s", num);
 unsigned long long i = IntFromString(hex, err);

 free(hex);
 return i;
}



int
striprefix(char *stg, char *pattern)
{
  int   slen;
  int   plen;

  plen = (int)strlen(pattern);
  slen = (int)strlen(stg);

  if (slen < plen)
    return 1;    // no match

  return strnicmp(stg, pattern, plen);
}


int
strprefix(char *stg, char *prefix)
{
  int  slen;
  int  plen;

  plen = (int)strlen(prefix);
  slen = (int)strlen(stg);

  if (slen < plen)
    return 1;    // mismatch

  return strncmp(stg, prefix, plen);
}


#if ! _NONSTOP
int
MkDirPath(char *path)
{
  char  dir[512];
  char  *cp;
  char  *ep;

  strcpy(dir, path);

  cp = dir;
  while ((ep = strchr(cp, '/')) != NULL) {
    *ep = '\0';
    if (strlen(dir) > 0) {
      (void) mkdir(dir, 0777);
      (void) chmod(dir, 0777);
    }
    *ep = '/';
    cp = ep + 1;
  }

  if (mkdir(dir, 0777) < 0 && (errno != EEXIST && errno != EACCES)) {
    fprintf(stderr, "mkdir failed; errno=%s\n", ErrnoToString(errno));
    return -1;
  }
  if (chmod(dir, 0777) < 0)
    fprintf(stderr, "chmod() error=%s: {%s}\n", ErrnoToString(errno), dir);

  return 0;
}


int
IterateDirPath(char *dirPath, IterateDirCallBack_t callback)
{

  int  ret = 0;

  if (callback == NULL)
    return ret;    // nothing to do

  DIR   *dir = opendir(dirPath);

  if (NULL != dir) {
    struct dirent  *entry;

    for (; NULL != (entry = readdir(dir));) { // Skip dots
      if ('.' == entry->d_name[0] && ('\0' == entry->d_name[1] || ('.' == entry->d_name[1] && '\0' == entry->d_name[2])))
        continue;  // do nothing

      char *target = malloc(strlen(dirPath) + strlen(entry->d_name) + 10);
      sprintf(target, "%s/%s", dirPath, entry->d_name);
      struct stat     st;

      if (0 == lstat(target, &st)) {
        if ((*callback) (target, &st) != 0)
          break;  // returned an error; skip out...
        } else {
          fprintf(stderr, "lstat of %s failed; error %s\n",
            target, ErrnoToString(errno));
          ret = -1;
        }
      free(target);
      }

    closedir(dir);
    dir = NULL;    // done
  }

  return ret;
}


int
RmDirPath(char *path)
{
  if (IterateDirPath(path, (void*)RmDirNode) != 0)
    fprintf(stderr, "Unable to IterateDirPath %s; error %s\n", path, ErrnoToString(errno));

  if (rmdir(path) != 0) {
    fprintf(stderr, "Unable to delete %s; error %s\n", path, ErrnoToString(errno));
    return -1;
  }

  return 0;
}


int
RmDirNode(char *path, void *arg)
{
  struct stat *st = (struct stat*)arg;

  if (st->st_mode & S_IFREG) { // is it a regular file?
    if (unlink(path) != 0)
      perror("unlink");
  } else if (st->st_mode & S_IFDIR) { // directory?
    IterateDirPath(path, (void*)RmDirNode);
    if (rmdir(path) != 0)
      perror("rmdir");
  } else if (st->st_mode & S_IFCHR) { // character device?
    if (unlink(path) != 0)
      perror("unlink");
  } else if (st->st_mode & S_IFBLK) { // block device?
    if (unlink(path) != 0)
      perror("unlink");
  } else if (st->st_mode & S_IFIFO) { // FIFO (named pipe)?
    if (unlink(path) != 0)
      perror("unlink");
  } else if (st->st_mode & S_IFLNK) { // symbolic link? (Not in POSIX.1-1996.)
    if (unlink(path) != 0)
      perror("unlink");
  } else if (st->st_mode & S_IFSOCK) { // socket? (Not in POSIX.1-1996.)
    if (unlink(path) != 0)
      perror("unlink");
  }

  return 0;
}


#endif


char*
GetMnemonicCh(unsigned char ch, char *mnem)
{
  static char    *specialChs[32] = {
    "Nul", "Soh", "Stx", "Etx", "Eot", "Enq", "Ack", "Bel", "Bs",
    "Ht", "Lf", "Vt", "Ff", "Cr", "So", "Si", "Dle", "Dc1",
    "Dc2", "Dc3", "Dc4", "Nak", "Syn", "Etb", "Can", "Em", "Sub",
    "Esc", "Fs", "Gs", "Rs", "Us"
  };
  static char tmp[4];

  if (mnem == (char *) 0)
   mnem = tmp;    // use internal bfr

  ch &= 0x7F;     // strip top bit

  if (ch < 32)    // a non printing ch
    strcpy(mnem, specialChs[ch]);
  else if (ch == 127)
    strcpy(mnem, "del"); // mnemonic for delete ch
  else {
   mnem[0] = (char) ch;
   mnem[1] = '\0';
  }

  return mnem;
}


char*
Stringify(char *s, int l)
{
  // count the number of escapes needed
  char *rep = "\"";
  char *with = "\\\"";
  int  rep_len = (int)strlen(rep);
  int  with_len = (int)strlen(with);
  int  count;
  char *tmp = s;

  if( ! s ) {
    s = "";
    l = 1;
  }
  for(count = 0; tmp = strstr(tmp, rep); ++count)
    tmp += rep_len;

  tmp = calloc(1, l + (with_len - rep_len) * count + 1);
  for(int i = 0; i < l; ++i) {
    if(!s[i] || !isprint(s[i])) {
      tmp[i] = ' ';  // replace non printables
      continue;
    }
    if(memcmp(&s[i], rep, rep_len) == 0) {
      memcpy(&tmp[i], with, with_len);
      i += rep_len;
      continue;
    }
    tmp[i] = s[i];
  }

  return tmp;
}


#if _NONSTOP
int
PathToOld(char *path, short *old)
{
  char localSubvolume[256];
  short svLen;

  int rc = PATHNAME_TO_FILENAME_(path,
    localSubvolume, (short)sizeof(localSubvolume), &svLen);
  localSubvolume[svLen] = '\0';
  rc = FILENAME_TO_OLDFILENAME_(localSubvolume, svLen, old);
  return rc;
}


int
LaunchProgram(char *program, startup_msg_type *sup)
{
  short   rc;
  jmp_buf exitFunc;
  FsFileDef *rcv_file = NULL;
  char      *pname = NULL;

  {
    int val = setjmp(exitFunc);
    if( val != 0 ) {
      if(rcv_file)
        FsClose(rcv_file);
      if(pname)
        free(pname);
      return val;
    }
  }


// open the receive file
  char    *rcv_name = "$receive";
  rcv_file = FsOpen(rcv_name, "rw", 10);
  if( rcv_file == NULL ) {
    fprintf(stderr, "Unable to open %s\n", rcv_name);
    longjmp(exitFunc, -1);
  }

  struct __zsys_ddl_plaunch_parms    parms;
  memset(&parms, 0, sizeof(parms));

  parms.z_version = ZSYS_VAL_PLAUNCH_PARMS_VER;
  parms.z_length = ZSYS_VAL_PLAUNCH_PARMS_LEN;

  parms.z_program_name = (zsys_ddl_char_extaddr_def)program;
  parms.z_program_name_len = (long)strlen(program);

  // parms.z_hometerm_name = (zsys_ddl_char_extaddr_def)myName;
  // parms.z_hometerm_name_len = (long)strlen(myName);
  parms.z_name_options = (short)ZSYS_VAL_PCREATOPT_NAMEDBYSYS5;
  //parms.z_debug_options = (ZSYS_VAL_PCREATOPT_INSPECT|ZSYS_VAL_PCREATOPT_RUND);
  parms.z_priority = (short)-1;


// request a start

  short   error_detail;
  struct __zsys_ddl_smsg_proccreate out_list;
  short   out_list_len = 0;
  rc = PROCESS_LAUNCH_ ( &parms /* i 1*/
                        , &error_detail  /* o 2*/
                        , &out_list  /* o:i 3*/
                        , (short)sizeof(out_list) /* o:i 3*/
                        , &out_list_len); /* o 4*/

  if(rc) {
    fprintf(stderr, "PROCESS_LAUNCH_: error_detail=%d\n", error_detail);
    longjmp(exitFunc, -1);
  }

// get the launch result (process name)
  zsys_ddl_smsg_proccreate_def msg;
  int len = FsReadRcv(rcv_file, &msg, sizeof(msg), ZSYS_VAL_SMSG_PROCCREATE);
  if( len <= 0 ) {
    fprintf(stderr, "Unable to read %s\n", rcv_file->fileName);
    longjmp(exitFunc, -1);
  }
  (void)FsReply(rcv_file, &msg, 0, 0, 0);

  if( msg.z_error ) {
    fprintf(stderr, "Unable to launch '%s'; error=%d\n",
                      program, msg.z_error_detail);
    longjmp(exitFunc, -1);
  }

  pname = strdup(msg.u_z_data.z_procname);
  if(strchr(pname, ':'))
    *strchr(pname, ':') = '\0';   // truncate at the colon

// build, then send startup
  short defaultName[24];
  startup_msg_type newmsg;
  {
    char cwd[256];

    memset(&newmsg, ' ', sizeof(newmsg));
    memcpy(&newmsg, sup, sizeof(newmsg));

    getcwd(cwd, sizeof(cwd));
    PathToOld(cwd, defaultName);
    memcpy(newmsg.defaults.whole, defaultName, sizeof(newmsg.defaults.whole));
    newmsg.msg_code = -1;
  }

  memset(newmsg.infile.whole, ' ', sizeof(newmsg.infile.whole));
  memcpy(newmsg.infile.parts.volume,    "$RECEIVE", 8);

  memset(newmsg.outfile.whole, ' ', sizeof(newmsg.outfile.whole));
  memcpy(newmsg.outfile.whole, defaultName, sizeof(newmsg.outfile.whole));

// set the file name
  char *components[16];
  int ci = FsParsePath(program, components);

  memset(newmsg.outfile.parts.file, ' ', sizeof(newmsg.outfile.parts.file));
  if(ci > 0)
    memcpy(newmsg.outfile.parts.file, components[1], strlen(components[1]));
  else
    memcpy(newmsg.outfile.parts.file, "output", 6);
  free(components[0]);

  memset((void*)newmsg.param, 0, sizeof(newmsg.param));

// open the process and send the startup message

  FsFileDef *pfile = FsOpen(pname, "rw");
  if( pfile == NULL ) {
    fprintf(stderr, "Unable to open %s\n", pname);
    perror("FsOpen");
    longjmp(exitFunc, -1);
  }

  rc = FsWriteRead(pfile, &newmsg, sizeof(newmsg), sizeof(newmsg));
  if( rc < 0 && pfile->_nonstop.ferrno != 70) {
    fprintf(stderr, "Unable to write to %s, error=%d\n", pname, rc);
    longjmp(exitFunc, -1);
  }

  if(pfile->_nonstop.ferrno == 70) {
    FsClose(pfile);
    pfile = FsOpen(pname, "rw");
    if( pfile == NULL ) {
      fprintf(stderr, "Unable to open %s\n", pname);
      perror("FsOpen");
      longjmp(exitFunc, -1);
    }
  }

  FsClose(rcv_file);    // close
  free(pname);

  return 0;
}

#else

int
LaunchProgram(Launch_t *launch)
{

  pid_t parent = getpid();
  // pid_t pid = fork();

  // if(pid == 0 || (int)pid < 0 ) // parent
    // return (int)pid;       // the parent

  // prepare arg list
    int argc = 0;
    for(int i = 0; launch->argv[i]; ++i)
      ++argc;    // count the args

    char **argv = (char**)calloc(argc + 2, sizeof(*argv));

  // copy the args
    argv[0] = launch->program;    // first, is the name of the program
    for(int i = 0; i < argc; ++i )
      argv[i+1] = strdup(launch->argv[i]);
    argv[argc+1] = NULL;

  // optionally change current directory
    if(launch->cwd) {
      if( chdir(launch->cwd) ) {
        fprintf(stderr, "%d: Unable to chdir to '%s', error %d\n", getpid(), launch->cwd, errno);
        _exit(errno);   // cannot switch directories; too late to tell parent
      }
    }
    // optionally open a new stdin
    if(launch->infile) {
      FILE *file;
      if((file = fopen(launch->infile, "r")) == NULL) {
        fprintf(stderr, "%d: Unable to fopen('%s', \"r\"), error %d\n", getpid(), launch->infile, errno);
        _exit(errno);   // cannot fopen; too late to tell parent
      }
      close(0);
      dup2(fileno(file), 0);
    }
    // optionally open a new stdout
    if(launch->outfile) {
      FILE *file;
      if((file = fopen(launch->outfile, "w")) == NULL) {
        fprintf(stderr, "%d: Unable to fopen('%s', \"w\"), error %d\n", getpid(), launch->outfile, errno);
        _exit(errno);   // cannot fopen; too late to tell parent
      }
      close(1);
      dup2(fileno(file), 1);
    }
    execvp(launch->program, argv);
    _exit(EXIT_FAILURE);   // exec never returns
}
#endif


#ifndef isblank
#define isblank(_c) ( (_c)==' ' || (_c) == '\t' )
#endif

Parser_t*
ParserNew(int size)
{
  Parser_t *this = calloc(1, sizeof(*this));

	this->bfr = (char*)calloc(1, size+1);
  this->bfrLen = size;
	this->stack = StackNew((StackTuple_t)-1, NULL);
	ParserClear(this);
	return this;
}


void
ParserDestroy(Parser_t *this)
{

	if (this->token != NULL)
		free(this->token);		// release previous token

	while ( this->stack->depth > 0 )
	{
		StackPop(this->stack);		// pop len
		StackPop(this->stack);		// pop next pointer
		free((void*)StackPop(this->stack));		// the saved token
	}
	StackDestroy(this->stack);
	free(this->bfr);
}


int
ParserSetInputData(Parser_t *this, char *bfr, int plen)
{

  free(this->bfr);
  this->bfr = calloc(1, plen+1);
  memcpy(this->bfr, bfr, plen);
	this->setLen = this->len = this->bfrLen = plen;
	return plen;
}


#if 0
int
ParserSetInputFile(Parser_t *this, int f)
{
	int rlen = this->maxsize - this->len;

	if (rlen <= 0)
		return rlen;			// unable to read

	char *bfr = alloca(rlen + 10);

	this->eof = 0;

	int ii = read(f, bfr, rlen);

	if (ii >= 0)
	{
		bfr[ii] = '\0';			// terminate input string
		ii = ParserSetInputData(this, bfr, ii);
	}

	if (ii <= 0 && errno == 0)
		this->eof = 1;

	return ii;
}
#endif


void
ParserRewind(Parser_t *this)
{
	this->next = this->bfr;
	this->len = this->setLen;
}


void
ParserClear(Parser_t *this)
{
	memset(this->bfr, 0, this->bfrLen);
	this->next = this->bfr;
	this->len = 0;
}


void
ParserNormalizeInput(Parser_t *this)
{
	char *inp = this->next;

	if (this->len > 0)		// some
	{
		// replace control chars with a space
		char *ptr;
		for (ptr = inp; *ptr;)
		{
			if (*ptr < ' ' || *ptr > '~')
			{
				strcpy(ptr, ptr + 1);
				--this->len;
			}
			else
			{
				++ptr;			// keep char
			}
		}
		// strip multi-space (consecutive spaces) down to single space
		while (this->len > 0 && ((ptr = strchr(inp, ' ')) != NULL && isspace(ptr[1])))
		{
			strcpy(ptr, ptr + 1);
			--this->len;
		}
		// strip leading spaces
		while (this->len > 0 && *inp == ' ')
		{
			strcpy(inp, inp + 1);
			--this->len;
		}
		// strip trailing spaces
		for (char *ptr = &inp[this->len - 1]; ptr >= inp && *ptr == ' ';)
		{
			*ptr-- = '\0';
			--this->len;
		}
	}

	this->next = inp;
	this->len = strlen(inp);
	this->setLen = this->len;
}


char*
ParserGetNextToken(Parser_t *this, char *delimeters)
{
	StackPush(this->stack, (StackTuple_t)this->token);
	StackPush(this->stack, (StackTuple_t)this->next);
	StackPush(this->stack, (StackTuple_t)this->len);

	this->token = NULL;

	// eat blanks
	while (isblank(*this->next))
	{
		++this->next;
		--this->len;
	}

	if (this->len <= 0)
		return "";				// none

	int ch = *this->next;
	char quoted = '\0';
	int len = 0;

	if (ch == '\\')				// escape
	{
		if (--this->len <= 0)
			return "";			// none
		ch = *(++this->next);
	}
	else if (ch == '\'' || ch == '"')
	{
		if (--this->len <= 0)	// no more input
		{
			this->token = calloc(1, 10);
			this->token[0] = *this->next++;	// just the open quote, no close
			return this->token;
		}
		quoted = *this->next++;
		char *ptr = strchr(this->next, quoted);

		if (ptr == NULL)		// no terminating end-quote; go to end of bfr
			ptr = &this->next[this->len];
		len = ptr - this->next;
	}
	else						// not a quoted token
	{
		if (ch != '\0' && strchr(delimeters, ch) != NULL)	// the next char is a delimeter
			len = 1;
		else
			len = strcspn(this->next, delimeters);
	}

	if (len <= 0)
		return "";				// none

	this->token = calloc(1, len + 10);
	memcpy(this->token, this->next, len);
	this->next += len;		// skip the token
	if ( (this->len -= len) > 0 && quoted == *this->next )	// need to close the quoted string
	{
		++this->next;
		--this->len;
	}

	if ( this->len > 0 && strchr(delimeters, *this->next ) != NULL )		// hit a delimeter
	{
		++this->next;
		--this->len;
	}

	return this->token;
}


char*
ParserUnGetToken(Parser_t *this, char *delimeters)
{
	if ( this->stack->depth <= 0 )
		return NULL;		// nothing saved

	this->len = (int)StackPop(this->stack);		// pop len
	this->next = (char*)StackPop(this->stack);			// pop next pointer
	this->token = (char*)StackPop(this->stack);		// the saved token
	return ParserGetNextToken(this, delimeters);
}


char*
ParserGetFullString(Parser_t *this)
{
	return this->bfr;
}


char*
ParserGetString(Parser_t *this)
{
	return this->next;
}


char *
ParserDownshift(Parser_t *this, char *string)
{

	for (char *s = string; *s; ++s)
		*s = tolower(*s);

	return string;
}


static const int StackGrowSize = 16;

Stack_t*
StackNew(StackTuple_t invalidTuple, char *name, ...)
{
  Stack_t *this = calloc(1, sizeof(*this));
	{
		va_list ap;
		va_start(ap, name);
		this->name = strdup(Sprintfv(NULL, 0, name, ap));
	}

	this->invalidTuple = invalidTuple;
	this->size = StackGrowSize;
	if ( (this->tuples = calloc(StackGrowSize, sizeof(*this->tuples))) == NULL )		// initial allocation
		SysLog(LogFatal, "No memory for %s", this->name);
	return this;
}


void
StackDestroy(Stack_t *this)
{
	free(this->name);
	free(this->tuples);
  free(this);
}


void
StackClear(Stack_t *this)
{
	this->depth = 0;
}


void
StackGrow(Stack_t *this, int size)
{
	this->size += size;		// increase
	if ( (this->tuples = (StackTuple_t*)realloc(this->tuples, (this->size*sizeof(*this->tuples)))) == NULL )
		SysLog(LogFatal, "No memory for %s %d", this->name, this->size);
}


void
StackPush(Stack_t *this, StackTuple_t tuple)
{
	if ((this->depth + 1) > this->size)	// need more bfr space
		StackGrow(this, StackGrowSize);

	this->tuples[this->depth++] = tuple;
}


StackTuple_t
StackPop(Stack_t *this)
{
	if ( --this->depth < 0 ) {
		SysLog(LogError, "Stack underflow in %s", this->name);
		this->depth = 0;
		return this->invalidTuple;
	}

	return this->tuples[this->depth];
}


StackTuple_t
StackTop(Stack_t *this)
{
	if ( this->depth <= 0 ) {
		SysLog(LogError, "Stack underflow in %s", this->name);
		return this->invalidTuple;
	}

	return this->tuples[this->depth-1];
}


StackTuple_t
StackBottom(Stack_t *this)
{
	if ( this->depth <= 0 ) {
		SysLog(LogError, "Stack underflow in %s", this->name);
		return this->invalidTuple;
	}

	return this->tuples[0];
}

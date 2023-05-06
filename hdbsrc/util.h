#ifndef _UTILH_
#define _UTILH_
#include <stdint.h>
#include <regex.h>
#include <signal.h>


#define _NONSTOP (_TANDEM_ARCH_ != 0)

#if _NONSTOP
#pragma fieldalign shared2
#include <cextdecs>
#include "zsysc"
#include <talh>
#endif

#include "leak.h"

#ifdef NEED_PATH
#if defined (WIN32) || defined (_WIN32)
static const char* PathSeparator = "\\";
#elif _NONSTOP
extern int PathToOld(char *path, short *old);
static const char* PathSeparator = ".";
#else
static const char* PathSeparator = "/";
#endif
#endif


#define Max(a, b)(((a) > (b)) ? (a) : (b))
#define Min(a, b)(((a) < (b)) ? (a) : (b))

typedef enum { False = 0, off = False, True, on = True } boolean ;

typedef enum
{
  LogAny          = 0xFFFF,       // always (-1)
  LogLevelMin     = 0,            // just a reference point
  LogFatal        = 0,            // catastrophic failure... does not return
  LogError        = 1,            // something bad; but, probably recoverable
                                  // or at least, not critical
  LogWarn         = 2,            // something of interest; could be an error
  LogDebug        = 3,            // miscellaneous useful detail for debug
  LogLevelMax                     // a reference point
} SysLogLevel;


typedef int (*WalkDirCallBack) (char*, void*, void*);


#if _NONSTOP
extern _variable void Stop(short error, short goodstop);
extern char *strdup(char *in);
#endif

#if _NONSTOP || __GNUC__
extern char *stristr(char *s0, char *s1);
extern int stricmp(char *s1, char *s2);
extern int strnicmp(char *s1, char *s2, int n);
#endif
#if __CYGWIN__
extern int stricmp(char *s1, char *s2);
#endif

extern int memicmp(void *s1, void *s2, size_t n);
extern char *memstr(char *haystack, char *needle, int size);
extern char *memistr(char *haystack, char *needle, int size);

extern char* GetMnemonicCh(unsigned char ch, char *mnem);
extern char *ToLower(char *string);
extern char *ToUpper(char *string);
extern char *Sprintfv(char *out, int size, char *fmt, va_list ap);
extern char *Sprintf(char *out, int size, char *fmt, ...);
extern char *SprintfCat(char *out, const char *fmt, ...);
extern char *SprintfvCat(char *out, const char *fmt, va_list ap);
extern void Fatal(char *msg, ...);
extern char *SysLog(SysLogLevel lvl, char *fmt, ...);
extern int   StartsWith(char *stg, char *text);
extern char* FTrim(char *text, const char *trimchr);
extern char* LTrim(char *text, const char *trimchr);
extern char* RTrim(char *text, const char *trimchr);
extern char *ExpandPath(char *dirPath);
extern char* UtilBaseName(char *name);

// You must free the result if result is non-NULL.
extern char* str_replace(char *orig, char *rep, char *with);
extern char* Stringify(char *s, int l);


typedef enum
{
  FileTypeEdit            = 1,
  FileTypePipe            = 2,
  FileTypeReceive         = 3,
  FileTypeUnstructured    = 4
} FsFileType;

typedef enum
{
  FsResolveNodeName =   -1,
  FsResolvePathName =    0,
  FsResolveSvolName =    1,
  FsResolveFileName =    2
} FsResolveLevel ;

#if _NONSTOP

typedef struct GregorianTimeDef
{
  union
  {
    int16_t int16[8];
    struct
    {
      int16_t year;   // [0] The Gregorian year (1984, 1985, ...)
      int16_t month;   // [1] The Gregorian month (1-12)
      int16_t day;   // [2] The Gregorian day of month (1-31)
      int16_t hour;   // [3] The hour of the day (0-23)
      int16_t minute;   // [4] The minute of the hour (0-59)
      int16_t second;   // [5] The second of the minute (0-59)
      int16_t mill;   // [6] The millisecond of the second (0-999)
      int16_t micro;   // [7] The microsecond of the millisecond (0-999)
    } date;
  } u;
} GregorianTimeDef ;


typedef struct FsFileInfoDef
{
  uint16_t  phyRecLen;        // 34
  uint16_t  fileType;         // 41
  uint16_t  fileCode;         // 42
  char      creationTime[8];  // 54
  union
  {
    uint8_t   owner[2];
    uint16_t  groupUser;
  } owner;                    // 58
  char      security[4];      // 62
  uint16_t  isopen;           // 75
  uint32_t  eof;              // 142
  char      modifyTime[8];    // 145
} FsFileInfoDef ;

extern FsFileInfoDef* FsGetFileInfo(char *name);
extern char* FsGetFilePerissions(char secuity[4]);
extern char* FsGetFileDate(char date[8]);
#endif

extern uint32_t fsize(int in);

struct FsFileDef;

typedef struct FsEventDef
{
  char *on;
  int  (*fnc)(struct FsFileDef *file, char *on, ...);
} FsEventDef;

typedef struct FsFileDef
{
  char        *fileName;
  char        *mode;
  short       fn;

  FsFileType  type;

  int         nevents;
  FsEventDef  *events;

#if _NONSTOP
  struct
  {
    short       condcode;
    short       ferrno;
    short       noWait;
    short       receiveDepth;
    short       receiveInfo[17];

    FsFileInfoDef finfo;

    struct
    {
      char      *bfr;
      char      *line;
      char      *bptr;
      int       bfrLen;
    } edit ;    // edit read control
  } _nonstop ;

#else
  FILE          *f;
#endif

} FsFileDef ;

typedef struct Launch_t
{
  char  *program;
  char  *cwd;
  char  *infile;
  char  *outfile;
  char  **argv;
} Launch_t ;


extern int WalkDir (char *path, FsResolveLevel rl, WalkDirCallBack callback, void *cbarg);

extern long CnvStringLongValue(char *stg, char **endptr, int base);
extern int CnvStringBooleanValue(char *val);
extern char *CnvIntegerToString(int value);
extern char *CnvIpAddrToString(unsigned char *addr);
extern char *ErrnoToString(int error);

extern int LaunchProgram(Launch_t *launch);
extern int FsParsePath(char *pname, char **components);
extern FsFileDef* FsOpen(char *fname, char *mode, ...);
extern int FsOn(FsFileDef *file, char *on, int(*fnc)(FsFileDef *file, char *on, ...));
extern int FsClose(FsFileDef *file);
extern FsFileDef* FsWait(int *fn, void *dataPtr, int *dataLen, long *tag, long waitTime);
extern int FsRead(FsFileDef *file, void *bfr, int rlen);
extern char* FsReadLine(FsFileDef *file);
extern int FsWrite(FsFileDef *file, void *bfr, int wlen);

#if _NONSTOP
extern int FsReadRcv(FsFileDef *file, void *bfr, int rlen, int mtype);
extern int FsWriteRead(FsFileDef *file, void *bfr, int wlen, int rsize);
extern int FsReply(FsFileDef *file, void *bfr, int wlen, int tag, int ret);
#endif


extern double Pow(double x, double y);
extern void swap(char *x, char *y);
extern char* reversestr(char *buffer, int i, int j);
extern char* itoa(int value, char* buffer, int base);
extern char* ftoa(double x, char *out);
extern uint32_t lz77_compress(uint8_t *u_text,
              uint32_t u_size,
              uint8_t *lz_text,
              uint8_t pointer_length_width);
extern uint32_t lz77_decompress(uint8_t *lz_text,
                uint8_t *u_text);
extern int32_t file_lz77_compress(char *filename_in,
                   char *filename_out,
                   uint8_t pointer_length_width);
extern int32_t file_lz77_decompress(char *filename_in, char *filename_out);

extern uint64_t ComputeSum(uint8_t *s, uint32_t len, uint64_t sum_value);

extern int HexDump( int (*oFnc)(char*, void*), // user function
  void *oFncArg,   // the argument to pass to the user function
  void *mem,       // data to dump
  int len,         // number of bytes
  long offset      // starting offset used in output
  );

#if _NONSTOP
typedef struct UserDef
{
  char    user_name[128];
  short   isAlias;
  short   default_security;
  char    default_vol[128];
  char    initdir[128];
  char    initprog[128];
  char    desc_txt[128];
  char    desc_bin[128];
  union
  {
    int32_t     user_id;
    struct
    {
      uint16_t    giduid;
      struct
      {
        uint8_t   gid;
        uint8_t   uid;
      } v ;
    } giduid;
  } uidgid ;
} UserDef ;

extern UserDef *GetUser(char *id);
extern UserDef**GetUsers(char *id);
#endif

typedef struct GrepDef
{
#if _NONSTOP
  int           editFilesOnly;
#endif
  int           totalMatch;
  char          pattern[BUFSIZ];
  char          exp[BUFSIZ];
  regex_t       regexp;

  int           ignoreCase;
  int           isDebug;
  char          *matchWord;
  int           rawMatch;
  int           invertMatch;
  int           quiet;
  int           displayLineNbrs;
  int           displayNamesOnly;
  int           displayCountsOnly;
  int           displayLinesBefore;
  int           displayLinesAfter;
  char          *executeCommand;
  int           savedLines;
  int           showFileName;
  int           shortNames;
  char          **bfrs;

  // Used during file matching
  char          *fileName;
  int           lineNbr;
  int           isMatch;
  int           matchCnt;
  int           displayLinesForced;
} GrepDef ;

extern int GrepLine(GrepDef *def, char *line, int rawMatch);
extern int GrepFile(GrepDef *def, char *fileName);

typedef struct EnumToStringMap_t
{
  int     en;
  char    *stg;
} EnumToStringMap_t;

#ifndef EnumToString
#define EnumToString(e) \
case (e): \
 text = #e; \
  break
  #endif

#define NullToValue(n, v) ((n)==NULL)?(v):(n)
#define BooleanToString(b) (b)?"yes":"no"

extern EnumToStringMap_t * EnumMapStringToVal(EnumToStringMap_t * map, char *text, char *prefix);
extern EnumToStringMap_t * EnumMapValToString(EnumToStringMap_t * map, int val);

extern unsigned long long IntFromString(char *num, int *err);
extern unsigned long long IntFromHexString(char *num, int *err);

#endif

extern int MkDirPath(char *path);
typedef int (*IterateDirCallBack_t) (char*, void*);
extern int IterateDirPath(char *dirPath, IterateDirCallBack_t callback);
extern int RmDirPath(char *path);
extern int RmDirNode(char *path, void *arg);


typedef uintptr_t StackTuple_t;

typedef struct Stack_t
{
	char		*name;
	int			size;			// current size
	int			depth;			// current tuple
	StackTuple_t		invalidTuple;
	StackTuple_t		*tuples;
} Stack_t;


// External Functions
extern Stack_t* StackNew(StackTuple_t invalidTuple, char *name, ...);
extern void StackDestroy(Stack_t *this);

static inline int StackLength(Stack_t *this) { return this->depth; }

extern void StackClear(Stack_t *this);
extern void StackPush(Stack_t *this, StackTuple_t tuple);
extern StackTuple_t StackPop(Stack_t *this);
extern StackTuple_t StackTop(Stack_t *this);
extern StackTuple_t StackBottom(Stack_t *this);
extern void StackGrow(Stack_t *this, int size);


typedef struct Parser_t
{
	int			eof;

	Stack_t		*stack;

	char  	*bfr;
  int     bfrLen;

	int			setLen;				// original amount 'set'
	int			len;					// how much is being used.
	char		*next;				// current output pointer

	char		*token;
} Parser_t;


extern Parser_t* ParserNew(int size);
extern void ParserDestroy(Parser_t *this);
extern int ParserSetInputData(Parser_t *this, char *bfr, int plen);
// TODO: extern int ParserSetInputFile(Parser_t *this, int f);
extern void ParserClear(Parser_t *this);
extern void ParserRewind(Parser_t *this);
extern void ParserNormalizeInput(Parser_t *this);
extern char *ParserGetNextToken(Parser_t *this, char *delimeters);
extern char *ParserUnGetToken(Parser_t *this, char *delimeters);
extern char *ParserGetFullString(Parser_t *this);
extern char *ParserGetString(Parser_t *this);
extern char *ParserDownshift(Parser_t *this, char *string);

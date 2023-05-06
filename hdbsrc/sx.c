#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>

#include "util.h"


typedef struct DoDef
{
  int   doFiller;
  char  *inp;
  char  *out;
} DoDef ;

extern int DoChar(char *rec, char *field, DoDef *def);
extern int DoInt(char *rec, char *field, DoDef *def);
extern int DoLongLong(char *rec, char *field, DoDef *def);
extern int DoLong(char *rec, char *field, DoDef *def);
extern int DoUnsigned(char *rec, char *field, DoDef *def);
extern int DoSubStruct(char *rec, char *field, DoDef *def);
extern int DoSubUnion(char *rec, char *field, DoDef *def);
extern int DoStruct(char *rec, char *strName, DoDef *def);

extern char*  GetStruct(char **strName, char **inp);
extern char*  GetString(char *inp, int off, int len);
extern char*  GetBraced(char **inp);
extern char*  DefWord(char *w, int len);
extern char*  GetWord(char *inp, int *len);
extern char*  ReadInput(FILE *f);
extern regmatch_t* RegiEx(const char *text, const char *exp, int n);

#define Trim(s) FTrim(s, " \t\r\n\f")
#define Max(a, b)(((a) > (b)) ? (a) : (b))
#define Min(a, b)(((a) < (b)) ? (a) : (b))


int
main(int argc, char *argv[])
{
  char  *out = NULL;
  char  *file = NULL;

  if( argc > 1 ) {
    file = ToLower(Trim(argv[1]));
    if( freopen(file, "r", stdin) == NULL ) {
      perror("freopen");
      fprintf(stderr, "Unable to open %s\n", file);
      exit(1);
    }
  }

  char *inp = ReadInput(stdin);
  for(char *ptr = inp; (ptr) && *ptr; ) {
    regmatch_t  *mp;
    Trim(ptr);

    if( (mp = RegiEx((const char*)ptr, "typedef *struct", 1)) == NULL )
      break;

    int   len = mp->rm_eo - mp->rm_so;
    char  *strName;

    ptr += mp->rm_so + len;   // position after matched string
    char *braced = GetStruct(&strName, &ptr);
    if( braced ) {
      DoDef def;
      def.doFiller = 0;   // no filler
      def.inp = braced;
      def.out = out;
      if( DoStruct(strName, (char*)"", &def) != 0 ) {
        out = NULL;
        break;        // error
      }
      out = def.out;
    }
    if( braced )
      free(braced);
    if( strName )
      free(strName);
    free(mp);
  }
  if(inp)
    free(inp);
  if(out) {
    static char *proto1 = {"char*\n"
            "%s(char *type, %s_def *rec, int rlen)\n"
            "{\n"
            "  char  *out = (char*)calloc(1, 65535);\n"
            "  char  *op = out;\n"
            "  char  *indent = \"  \";\n"
            "\n"
            "  op += sprintf(op, \"%s:\\n\", type);\n"
            "\n"
          };
    static char *proto2 = {
            "\n"
            "  return out;\n"
            "}\n"
          };

    char *strName = strdup(ToLower(UtilBaseName(file)));
    if( strName[strlen(strName)-1] == 'x' )
      strName[strlen(strName)-1] = '\0';
    printf("// file %s\n", file);
    printf(proto1, strName, strName, strName);
    printf("%s\n", out);
    printf(proto2);
    free(strName);
    free(out);
  }

  return 0;
}


char*
ReadInput(FILE *f)
{
  char  bfr[BUFSIZ];
  int   ilen = 32;
  char  *inp = (char*)malloc(ilen+1);    // start with 32 bytes
  char  *ptr = inp;
  int   len = ilen;
  int   incomment = 0;

  while(fgets(bfr, sizeof(bfr), f) ) {
    char *bptr = bfr;
    int   wasSpace = 1;    // fake, will cause trim of left margin

    if(strlen(Trim(bptr)) <= 0)
      continue;       // skip blank lines
    if(strchr(bptr, '#'))
      continue;       // skip compiler directives

    if( strstr(bptr, "/*") != 0 )
      ++incomment;

    while(*bptr) {
      char  ch;

      do {
        ch = *bptr++;
        if( incomment && ch == '*' && *bptr == '/' ) {
          --incomment;
          ch = *bptr++;
          ch = *bptr++;
        }
      } while ( incomment && *bptr );

      if( !ch || !isprint(ch) )
        continue;
      if( isspace(ch) && wasSpace )
        continue;

      wasSpace = isspace(ch);

      *ptr++ = (char)tolower(ch);
      *ptr = '\0';
      if( --len <= 0 ) {    // need more memory
        inp = (char*)realloc(inp, ilen+33);    // start with 32 bytes
        len = 32;
        ptr = inp + ilen;
        ilen += 32;
      }
    }
  }

  return Trim(inp);
}


char*
GetStruct(char **strName, char **inp)
{
  char *iptr = *inp;

  *strName = NULL;

  Trim(iptr);
  if( *iptr == '\0' ) {
    *inp = iptr;
    return NULL;    // nothing here
  }

  if( *iptr != '{' ) {   // has a name
    char *ptr = Trim(++iptr);
    while( *iptr && *iptr != '{' )
      iptr = Trim(++iptr);            // walk until hit the open brace
    *strName = GetString(ptr, 0, (iptr-ptr));
  }

  char *braced = GetBraced(&iptr);

  Trim(iptr);
  if( *iptr != ';' ) {      // get the trailing name
    if( *strName )
      free(*strName);     // free old one
    char *ptr = Trim(iptr);
    while(*iptr && *iptr != ';')
      ++iptr;
    *strName = GetString(ptr, 0, (iptr-ptr));
  }
  if( *iptr != ';' ) {    // no closing semi
    fprintf(stderr, "Missing closing ';'\n");
    if( *strName )
      free(*strName);     // free old one
    strName = NULL;
    if( braced )
      free(braced);
    braced = NULL;
  }
  ++iptr;       // skip trailing ';'

  if( braced && *strName == NULL )
    *strName = strdup("");     // no name...

  *inp = iptr;
  return braced;
}


char*
GetBraced(char **inp)
{
  int   blen = 32;
  char  *braced = (char*)malloc(blen+1);    // start with 32 bytes
  int   len = blen;
  int   depth = 0;

  char  *out = braced;
  char  *ptr = Trim(*inp);

  while( *ptr ) {
    char ch = *ptr++;

    if( depth == 0 && isspace(ch) )
      continue;   // toss out of context spaces.

    if( ch == '{' ) { // open
      ++depth;
      if( depth == 1 )
        continue;     // don't save the first one
    }
    if( ch == '}' ) {// close
      if( --depth <= 0 )
        break;    // done
    }

    if( !isspace(ch) && depth == 0 ) {     // bad format
      fprintf(stderr, "Missing open '{'\n");
      puts(braced);
      free(braced);
      return NULL;
    }

    *out++ = (char)ch;
    *out = '\0';
    if( --len <= 0 ) {    // need more memory
      braced = (char*)realloc(braced, blen+33);    // start with 32 bytes
      len = 32;
      out = braced + blen;
      blen += 32;
    }
  }

  *inp = ptr;
  return braced;
}


char*
GetString(char *ptr, int off, int len)
{
  if( len <= 0 )
    return strdup("");    // no string
  char *stg = (char*)calloc(len+1, sizeof(*ptr));
  memcpy(stg, &ptr[off], len);
  stg[len] = '\0';
  return stg;
}


char*
DefWord(char *w, int len)
{
  char *word = (char*)malloc(len+1);
  memcpy(word, w, len);
  word[len] = '\0';
  return word;
}


#define IsWordCh(ch) ((ch) && (isalnum(ch) || (ch) == '_'))

char*
GetWord(char *inp, int *len)
{

  Trim(inp);

  char *word = inp;
  if( ! *word )
    return strdup("");    // not one

  char *wp = word+1;
  while( IsWordCh(*word) && IsWordCh(*wp) )
    ++wp;
  *len = wp - word;

  word = DefWord(word, *len);
  return word;
}


typedef struct
{
  const char    *word;
  int           (*doFnc)(char *rec, char *field, DoDef *def);
} WordsDef ;

int
EmitOutput(DoDef *def, char *type, char *rec, char *field, char *name)
{
  if( ! StartsWith(name, "filler") || def->doFiller ) {
    def->out = SprintfCat(def->out, "%-10.10s(%-32.32s,   %s%s%s);\n", type, rec, field, (field[0]?".":""), name);
  }

  return 0;
}


int
EmitOutputArray(DoDef *def, char *type, char *rec, char *field, char *name, int i)
{
  if( ! StartsWith(name, "filler") || def->doFiller ) {
    def->out = SprintfCat(def->out, "%-10.10s (%-32.32s,   %s%s%s[%d]);\n", type, rec, field, (field[0]?".":""), name, i);
  }

  return 0;
}


int
DoChar(char *rec, char *field, DoDef *def)
{
  int   wlen;

  char *name = GetWord(def->inp, &wlen);
  def->inp += wlen;
  char *value = GetWord(def->inp, &wlen);
  def->inp += wlen;

  if( strcmp(value, "[") == 0 ) {  // array
    EmitOutput(def, "pString", "rec", field, name);
  } else {
    EmitOutput(def, "pChar", "rec", field, name);
  }

  while( *value != ';' && *def->inp && *def->inp != ';' )
    ++def->inp;          // parse until trailing ';';
  if( *def->inp == ';' )
    ++def->inp;
  if(name)
    free(name);
  if(value)
    free(value);

  return 0;
}

int
DoInt(char *rec, char *field, DoDef *def)
{
  int   wlen;

  char *name = GetWord(def->inp, &wlen);
  def->inp += wlen;
  char *value = GetWord(def->inp, &wlen);
  def->inp += wlen;

  if( strcmp(value, "[") == 0 ) {  // array
    free(value);
    char *value = GetWord(def->inp, &wlen);   // the size
    def->inp += wlen;
    int err;
    int rep = (int)IntFromString(value, &err)-1;
    if( ! err ) {
      for(int i = 0; i <= rep; ++i)
        EmitOutputArray(def, "pInt", "rec", field, name, i);
    }
  } else {
    EmitOutput(def, "pInt", "rec", field, name);
  }

  while( *value != ';' && *def->inp && *def->inp != ';' )
    ++def->inp;          // parse until trailing ';';
  if( *def->inp == ';' )
    ++def->inp;
  if(name)
    free(name);
  if(value)
    free(value);

  return 0;
}


int
DoLongLong(char *rec, char *field, DoDef *def)
{
  int   wlen;

  char *name = GetWord(def->inp, &wlen);
  def->inp += wlen;
  char *value = GetWord(def->inp, &wlen);
  def->inp += wlen;

  if( strcmp(value, "[") == 0 ) {  // array
    free(value);
    char *value = GetWord(def->inp, &wlen);   // the size
    def->inp += wlen;
    int err;
    int rep = (int)IntFromString(value, &err)-1;
    if( ! err ) {
      for(int i = 0; i <= rep; ++i)
        EmitOutputArray(def, "pLongLong", "rec", field, name, i);
    }
  } else {
    EmitOutput(def, "pLongLong", "rec", field, name);
  }

  while( *value != ';' && *def->inp && *def->inp != ';' )
    ++def->inp;          // parse until trailing ';';
  if( *def->inp == ';' )
    ++def->inp;
  if(name)
    free(name);
  if(value)
    free(value);

  return 0;
}


int
DoLong(char *rec, char *field, DoDef *def)
{
  int   wlen;

  char *name = GetWord(def->inp, &wlen);
  def->inp += wlen;

  if( strcmp(name, "long") == 0 ) {    // this is a long long
    DoLongLong(rec, field, def);
    if(name)
      free(name);
    return 0;
  }

  char *value = GetWord(def->inp, &wlen);
  def->inp += wlen;

  if( strcmp(value, "[") == 0 ) {  // array
    free(value);
    char *value = GetWord(def->inp, &wlen);   // the size
    def->inp += wlen;
    int err;
    int rep = (int)IntFromString(value, &err)-1;
    if( ! err ) {
      for(int i = 0; i <= rep; ++i)
        EmitOutputArray(def, "pLong", "rec", field, name, i);
    }
  } else {
    EmitOutput(def, "pLong", "rec", field, name);
  }

  while( *value != ';' && *def->inp && *def->inp != ';' )
    ++def->inp;          // parse until trailing ';';
  if( *def->inp == ';' )
    ++def->inp;
  if(name)
    free(name);
  if(value)
    free(value);

  return 0;
}


int
DoUnsigned(char *rec, char *field, DoDef *def)
{
  int   wlen;

  char *name = GetWord(def->inp, &wlen);
  def->inp += wlen;

  if( strcmp(name, "char") == 0 ) {    // this is a char
    DoChar(rec, field, def);
    if(name)
      free(name);
    return 0;
  }
  if( strcmp(name, "short") == 0 ) {    // this is a short
    DoInt(rec, field, def);
    if(name)
      free(name);
    return 0;
  }
  if( strcmp(name, "int") == 0 ) {    // this is a int
    DoInt(rec, field, def);
    if(name)
      free(name);
    return 0;
  }
  if( strcmp(name, "long") == 0 ) {    // this is a long
    DoLong(rec, field, def);
    if(name)
      free(name);
    return 0;
  }

  char *value = GetWord(def->inp, &wlen);
  def->inp += wlen;

  if( strcmp(value, "[") == 0 ) {  // array
    free(value);
    char *value = GetWord(def->inp, &wlen);   // the size
    def->inp += wlen;
    int err;
    int rep = (int)IntFromString(value, &err)-1;
    if( ! err ) {
      for(int i = 0; i <= rep; ++i)
        EmitOutputArray(def, "pUnsigned", "rec", field, name, i);
    }
  } else {
    EmitOutput(def, "pUnsigned", "rec", field, name);
  }

  while( *value != ';' && *def->inp && *def->inp != ';' )
    ++def->inp;          // parse until trailing ';';
  if( *def->inp == ';' )
    ++def->inp;
  if(name)
    free(name);
  if(value)
    free(value);

  return 0;
}

int
DoSubStruct(char *rec, char *field, DoDef *def)
{
  char  *strName;
  char  *braced = GetStruct(&strName, &def->inp);
  int   ret = -1;

  if( braced && strName ) {
    char  *sinp = def->inp;
    def->inp = braced;
    char *tmp = strdup(strName);
    if( strlen(field) > 0 )
      tmp = SprintfCat(NULL, "%s.%s", field, strName);
    ret = DoStruct(rec, tmp, def);
    def->inp = sinp;
    free(tmp);
  }

  if( braced )
    free(braced);
  if( strName )
    free(strName);

  return ret;
}


int
DoSubUnion(char *rec, char *field, DoDef *def)
{
  char  *strName;
  char  *braced = GetStruct(&strName, &def->inp);
  int   ret = -1;

  if( braced && strName ) {
    char  *sinp = def->inp;
    def->inp = braced;
    char *tmp = strdup(strName);
    if( strlen(field) > 0 )
      tmp = SprintfCat(NULL, "%s.%s", field, strName);
    ret = DoStruct(rec, tmp, def);
    def->inp = sinp;
    free(tmp);
  }

  if( braced )
    free(braced);
  if( strName )
    free(strName);

  return ret;
}


int
DoStruct(char *rec, char *strName, DoDef *def)
{
  WordsDef words[] =
  {
    "union",      DoSubUnion,
    "struct",     DoSubStruct,
    "char",       DoChar,
    "short",      DoInt,
    "int",        DoInt,
    "signed",     DoInt,
    "long",       DoLong,
    "__int32_t",  DoLong,
    "__uint32_t", DoUnsigned,
    "int32_t",    DoLong,
    "unsigned",   DoUnsigned,
    NULL, NULL
  };
  char  *ptr;
  int   ret = 0;

  ptr = Trim(def->inp);

  int  wlen;
  for(char *word; ret == 0 && strlen((word = GetWord(ptr, &wlen))) > 0; ) {
    ptr += wlen;
    def->inp = ptr;

    WordsDef *wrd = words;
    while(wrd->word && wrd->doFnc) {
      if(strcmp(wrd->word, word) == 0 )
        break;
      ++wrd;
    }

    if( ! wrd->doFnc ) {
      fprintf(stderr, "no handler for '%s.%s'\n", rec, word);
      ret = -1;
    }
    else {
      ret = (*wrd->doFnc)(rec, strName, def);
    }

    ptr = def->inp;
    if(word)
      free(word);
  }

  def->inp = ptr;

  return ret;
}


regmatch_t*
RegiEx(const char *text, const char *exp, int n)
{
  regex_t     regex;
  regmatch_t  *mp;

  mp = (regmatch_t*)calloc(n, sizeof(regmatch_t));
  if (regcomp (&regex, exp, REG_EXTENDED | REG_ICASE) < 0) {
    fprintf(stderr, "Unable to compile '%s'\n", exp);
    perror ("regcomp failed");
    free(mp);
    return NULL;
  }

  int r = regexec(&regex, text, 1, mp, 0);
  if( r != 0 ) {
    regfree (&regex);
    free(mp);
    return NULL;
  }

  regfree (&regex);
  return mp;
}

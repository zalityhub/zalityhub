#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>


#include "util.h"


typedef enum { PlainFmt=0, VerboseFmt=1, BarFmt=2} Format ;

typedef struct FingerDef
{
  Format  optFmt;
  int     optPipe;
  int     optAll;
} FingerDef ;


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

  fprintf(stderr, "Usage: %s [-vba] user\n", Moi);
  fprintf(stderr, "Try '%s --help' for more information\n", Moi);
  exit(0);
}

static void
Help()
{
  printf("Usage: %s [-vba] user\n", Moi);
  printf(
     "%s - List information about: user\n"
     "   -v      Display in verbose format\n"
     "   -b      Display in bar format\n"
     "   -a      Display all users\n"
     "   --      Display all users from stdin\n"
     "   --help: Display this help text and exit\n"
       , Moi);

  exit(0);
}

static char*
UserBar(UserDef *user)
{
  static char tmp[1024];
  char        *ptr = tmp;
  ptr += sprintf(ptr, "%s|", user->user_name);
  ptr += sprintf(ptr, "%d,%d|", user->uidgid.giduid.v.gid,
                     user->uidgid.giduid.v.uid);
  // ptr += sprintf(ptr, "%s|", (user->isAlias?"yes":"no"));
  ptr += sprintf(ptr, "%s|", user->default_vol);
  ptr += sprintf(ptr, "%s", user->desc_txt);
  return tmp;
}


static char*
UserVerbose(UserDef *user)
{
  static char tmp[1024];
  char        *ptr = tmp;
  ptr += sprintf(ptr, "user_name   = %s\n", user->user_name);
  ptr += sprintf(ptr, "user_id     = %d,%d\n", user->uidgid.giduid.v.gid,
                     user->uidgid.giduid.v.uid);
  ptr += sprintf(ptr, "is_alias    = %s\n", (user->isAlias?"yes":"no"));
  ptr += sprintf(ptr, "default_vol = %s\n", user->default_vol);
  ptr += sprintf(ptr, "desc_txt    = %s", user->desc_txt);
  return tmp;
}


static char*
UserPlain(UserDef *user, char *id)
{
  static char tmp[1024];
  char        *ptr = tmp;
  if( isdigit(*id) ) // by gid,uid
    ptr += sprintf(ptr, "%s=%s", id, user->user_name);
  else
    ptr += sprintf(ptr, "%s=%d,%d", id,
                       user->uidgid.giduid.v.gid,
                       user->uidgid.giduid.v.uid);
  return tmp;
}


static char*
UserToString(UserDef *user, char *id, Format fmt)
{
  if( fmt == VerboseFmt )
    return(UserVerbose(user));
  else if( fmt == BarFmt )
    return(UserBar(user));
  else return(UserPlain(user, id));
}


static void
DisplayUser(UserDef *user, char *id, Format fmt)
{
  puts(UserToString(user, id, fmt));
}


static int
DisplayUsers(char *text, Format fmt, Format sfmt)
{
  UserDef **users;
  int     uc = 0;

  users = GetUsers("");     // all users
  if( users ) {
    UserDef **us = users;
    while( *us ) {
      UserDef *user = *us;
      char *out = UserToString(user, user->user_name, sfmt);
      if( stristr(out, text) ) {    // is match
        DisplayUser(user, user->user_name, fmt);
        ++uc;
      }
      ++us;
    }
    free(users);
  }

  return uc;
}


static void
Finger(FingerDef *def, char *id)
{
  UserDef *user = GetUser(id);
  if( user == NULL ) {
    int uc = DisplayUsers(id, def->optFmt, BarFmt); // miss, do a bar search
    if( uc <= 0 )
      fprintf(stderr, "User '%s' not found\n", id);
    return;
  }
  DisplayUser(user, id, def->optFmt);
}


int
main(int argc, char *argv[])
{
  FingerDef def;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  memset(&def, 0, sizeof(def));
  def.optFmt = PlainFmt;

  // collect options
  while(argc > 0) {
    char *arg = *argv;

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
            Usage("-%c is not a valid option", arg[inOption-1]);
            break;
          case '?':
            Usage("");
            break;
          case '-':
            ++def.optPipe;
            break;
          case 'a':
            ++def.optAll;
            break;
          case 'b':
            def.optFmt = BarFmt;
            break;
          case 'v':
            def.optFmt = VerboseFmt;
            break;
        }
      }
      else if( arg[0] == '+' ) {
        switch(arg[inOption++]) {
          default:
            Usage("+%c is not a valid option", arg[inOption - 1]);
            break;
        }
      }
    }
  }    // while options...

  // Iterate over the remainder of command set

  if(def.optAll)
    (void)DisplayUsers("", def.optFmt, def.optFmt);

  if( argc > 0 ) {
    while(argc > 0) {
      char *arg = *argv;
      --argc;
      ++argv;
      Finger(&def, arg);
    }
  } else if(def.optPipe && ! def.optAll ) {
    char bfr[BUFSIZ];
    char *id = bfr;
    while( gets(bfr) ) {
      UserDef *user = GetUser(id);
      if( user == NULL ) {
        int uc = DisplayUsers(id, def.optFmt, def.optFmt);
        if( uc <= 0 )
          fprintf(stderr, "User '%s' not found\n", id);
        continue;
      }
      DisplayUser(user, id, def.optFmt);
    }
  }
  else if(! def.optAll ) {
    Usage("");
  }

  return 0;
}

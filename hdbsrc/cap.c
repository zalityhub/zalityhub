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



typedef struct TermDef
{
#if _NONSTOP
  int editFilesOnly;
#endif
} TermDef ;


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

  fprintf(stderr, "Usage: %s/name $xxx,out outfile/\n", Moi);
  fprintf(stderr, "Try '%s --help' for more information\n", Moi);
  exit(0);
}

static void
Help()
{
  printf("Usage: %s/name $xxx,out outfile/\n", Moi);
  printf(
     "%s - Caputures output directed to this process\n"
     "     and re-writes to the specified output file\n"
     "   --help: Display this help text and exit\n"
       , Moi);

  exit(0);
}


int
main(int argc, char *argv[])
{
  TermDef def;

  Moi = UtilBaseName(argv[0]);

  --argc;
  ++argv;

  memset(&def, 0, sizeof(def));

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
      else if( arg[0] == '+' ) {
        switch(arg[inOption++]) {
          default:
            Usage("+%c is not a valid option", arg[inOption - 1]);
            break;
        }
      }
    }
  }    // while options...


  char *fileName = "$receive";

  FsFileDef *file = FsOpen(fileName, "rw");
  if( file == NULL ) {
    fprintf(stderr, "Unable to open %s\n", fileName);
    perror("FsOpen");
    return 0;
  }

  int  done = 0;
  int  opens = 0;
  char bfr[BUFSIZ];
  int  rlen;

  while( ! done && (rlen = FsRead(file, bfr, sizeof(bfr))) >= 0 ) {
    zsys_ddl_smsg_def *msg = (zsys_ddl_smsg_def*)bfr;
    bfr[rlen] = '\0';
    switch(msg->u_z_msg.z_msgnumber[0]) {
      default:
        printf("msgcode: %d\n", msg->u_z_msg.z_msgnumber[0]);
        break;

      case ZSYS_VAL_SMSG_PROCCREATE:
        printf("Create\n");
        break;

      case ZSYS_VAL_SMSG_OPEN:
        printf("Open\n");
        ++opens;
        break;

      case ZSYS_VAL_SMSG_CLOSE:
        printf("Close\n");
        if( --opens <= 0 )
          ++done;
        break;

      case ZSYS_VAL_SMSG_CONTROL:
        printf("Control: %d\n", msg->u_z_msg.z_control.z_operation);
        break;
    }
    (void)FsReply(file, bfr, 0, 0, 0);
  }
  FsClose(file);

  return 0;
}

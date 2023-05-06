#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include "util.h"


int
main(int argc, char *argv[])
{
  char cwd[256];
  Launch_t launch;

  memset(&launch, 0, sizeof(launch));
  getcwd(cwd, sizeof(cwd));
  launch.cwd = cwd;

  if(argc <= 1) {
    fprintf(stderr, "Usage: lp program\n");
    exit(0);
  }

// collect the arguments...

  while(--argc ) {
    char *arg = FTrim(*++argv, " \t");
    if( arg[0] == '<' ) {   // redirect stdin
      arg = FTrim(arg+1, " \t");    // trim post the symbol
      if(! strlen(arg)) {     // nothing there; use next arg...
        if(--argc <= 0 ) {
          fprintf(stderr, "Missing file name after '<'\n");
          exit(0);
        }
        arg = FTrim(*++argv, " \t");
      }
      launch.infile = FTrim(arg, " \t");
    } else if( arg[0] == '>' ) {   // redirect stdout
      arg = FTrim(arg+1, " \t");    // trim post the symbol
      if(! strlen(arg)) {     // nothing there; use next arg...
        if(--argc <= 0 ) {
          fprintf(stderr, "Missing file name after '>'\n");
          exit(0);
        }
        arg = FTrim(*++argv, " \t");
      }
      launch.outfile = FTrim(arg, " \t");
    } else {
      if(launch.program) { 
        fprintf(stderr, "You already told me to run '%s'; can't do another...\n", launch.program);
        exit(0);
      }
      launch.program = arg;
    }
  }
  ++argv;     // pop the last arg used...

  if(! launch.program) {
    fprintf(stderr, "Usage: lp program\n");
    exit(0);
  }

#if 1
  launch.argv = argv;     // what's left
  if( LaunchProgram(&launch) < 0 ) {
    fprintf(stderr, "Launch of %s failed\n", launch.program);
    exit(0);
  }
#endif

  return 0;
}
#if _NONSTOP
?  , fnamecollapse
?  , fnameexpand
?  , mypid
?  , newprocess
?  , numin
?  , numout
?  , oldfilename_to_filename_
#endif

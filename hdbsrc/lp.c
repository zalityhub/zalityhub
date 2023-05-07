#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include "util.h"


int
main(int argc, char *argv[])
{

  if(argc <= 1) {
    fprintf(stderr, "Usage: lp program\n");
    exit(0);
  }

// collect the arguments...

  char *args = NULL;

  while(--argc ) 
    args = SprintfCat(args, " %s", *++argv);



  Launch_t *launch = LaunchParseArgs(args);

  char cwd[256];
  getcwd(cwd, sizeof(cwd));
  launch->cwd = cwd;
  if(! launch->program) {
    fprintf(stderr, "No program name given\nUsage: lp program\n");
    exit(0);
  }

#if 1
  if( LaunchProgram(launch) < 0 ) {
    fprintf(stderr, "Launch of %s failed\n", launch->program);
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

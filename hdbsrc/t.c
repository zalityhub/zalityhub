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


int
main(int argc, char *argv[])
{
  char cmd[BUFSIZ];
  char *out = cmd;

  memset(cmd, 0, sizeof(cmd));
  while(--argc)
    out += sprintf(out, "%s ", *++argv);

  if(strlen(cmd)) {
    puts(cmd);
    execp(cmd);
  }
  return 0;
}

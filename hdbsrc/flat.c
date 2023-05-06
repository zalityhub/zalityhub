#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"


int main(int argc, char *argv[])
{
  char bfr[BUFSIZ] = {'\0'};
  char *tmp = (char*)calloc(1, sizeof(char));

  while( fgets(bfr, sizeof(bfr), stdin) ) {
    FTrim(bfr, "\n\r\t");
    if( tmp[0] != '\0' )
      strcat(tmp, " ");
    tmp = (char*)realloc(tmp, strlen(tmp)+strlen(bfr)+4);
    strcat(tmp, bfr);
  }

  puts(tmp);
  return 0;
}

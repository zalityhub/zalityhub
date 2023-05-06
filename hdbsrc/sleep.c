#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{

 while( --argc >= 0 ) {
  sleep(atoi(*++argv));
 }

  return 0;
}

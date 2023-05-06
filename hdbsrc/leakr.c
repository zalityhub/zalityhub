
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <map>

using namespace std;

int
main(int argc, char *argv[])
{
  map<string, string> alloc;
  char   bfr[32767];

  if( argc > 1 ) {
    if( freopen(argv[1], "r", stdin) == NULL ) {
      perror("freopen");
      fprintf(stderr, "Unable to open %s\n", argv[1]);
      exit(1);
    }
  }

  while( fgets(bfr, sizeof(bfr), stdin) ) {
    for(char *ptr = NULL; (ptr = strchr(bfr, '\r')) || (ptr = strchr(bfr, '\n') ) != NULL; )
      *ptr = '\0';

// look for: 090924 15:41:01 eventlib.c.115 malloc(232)=0x8dc56e8

    char    *key;
    if( (key = strstr(bfr, "malloc(")) != NULL || (key = strstr(bfr, "calloc(")) != NULL || (key = strstr(bfr, "realloc_")) != NULL ) {
      if( (key = strchr(key, '=')) != NULL ) {
        char *addr = strdup(++key);
        char *ptr = strchr(addr, ',');      // address stack?
        if( ptr != NULL )
            *ptr = '\0';                    // remove it
        alloc[addr] = bfr;
        free(addr);
      }
    }
    else if( (key = strstr(bfr, "free(")) != NULL ) {
      if( (key = strchr(key, '=')) != NULL ) {
        char *addr = ++key;
        alloc[addr] = "";       // released
      }
    }
    else {
      printf("I don't know this: %s\n", bfr);
    }
  }

#if 0
// sort by value
  multimap<int, string> sorted;
  for( map<string,int>:: iterator it = alloc.begin(); it != alloc.end(); ++it )
  {
      sorted.insert(pair<int,string>(it->second, it->first));
  }

  for( multimap<int, string>:: iterator it = sorted.begin(); it != sorted.end(); ++it ) {
    printf("%d: %s\n", it->first, it->second.c_str());
  }
#else
// sort by value

// display any remaining malloc'ed items...
  for( map<string,string>:: iterator it = alloc.begin(); it != alloc.end(); ++it ) {
    if( strlen(it->second.c_str()) > 0 )
      printf("%s: %s\n", it->first.c_str(), it->second.c_str());
  }
#endif

  return 0;
}

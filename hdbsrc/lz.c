#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>


#include "util.h"


int
main (int argc, char const *argv[])
{
  char *filename_in = "tf";

  for(uint8_t i = 5; i < 6; ++i) {
    int32_t cs = file_lz77_compress(filename_in, "l7", i);
    if( cs < 0) {
      fprintf(stderr, "Unable to compress %s\n", filename_in);
      return 1;
    }
    int32_t zs = file_lz77_decompress("l7", "ln7");
    if( zs < 0) {
      fprintf(stderr, "Unable to decompress %s\n", "l7");
      return 1;
    }
    double pct = (1.0 - ((double)cs / (double)zs)) * 100.0;
    printf("Compressed (%d): %ld, decompressed: (%ld): %s%% savings\n",
   i, cs, zs, ftoa(pct, NULL));
  }
  return 0;
}

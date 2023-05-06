#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    while(--argc > 0) {
        char tmp[1024];
        long v;

        ++argv;
        if( strncmp(*argv, "0x", 2) == 0 ) {
            v = strtol(*argv+2, NULL, 16);
            sprintf(tmp, "%d", v);
        } else {
            v = strtol(*argv, NULL, 10);
            sprintf(tmp, "0x%X", v);
        }

       puts(tmp);
    }
    return 0;
}

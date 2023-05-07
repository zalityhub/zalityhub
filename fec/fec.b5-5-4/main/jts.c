#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "include/cJSON.h"



int
main(int argc, char *argv[])
{
	static char bfr[1024*1024];

	while ( fgets(bfr, sizeof(bfr), stdin) != NULL )
	{
		char *ptr = bfr;

		while ( *ptr && *ptr != '{' )
			++ptr;

		if ( *ptr == '{' )
		{
			cJSON *root = cJSON_Parse(ptr);
			if ( root == NULL )
			{
				fprintf(stderr, "'%s' is invalid jSON input\n", ptr);
			}
			else
			{
				char *tmp = cJSON_Print(root);
				if ( tmp )
				{
					puts(tmp);
					free(tmp);
				}
				cJSON_Delete(root);
			}
		}
	}

	return 0;
}

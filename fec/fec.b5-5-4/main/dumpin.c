/*****************************************************************************

Filename:   main/dumpin.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int main(int argc, char *argv[])
{
	char	bfr[BUFSIZ];


	while (fgets(bfr, sizeof(bfr), stdin) != NULL )
	{
		char *ptr = strstr(bfr, ": ");
		if ( ptr == NULL )
			continue;

		ptr += 2;		// start of first nibble

		for(int i = 0; i < 16; ++i )
		{
			int ch1 = *ptr++;
			int ch2 = *ptr++;

			if ( isspace(ch1) && isspace(ch2) )
				continue;		// skip blank

			if ( ! isxdigit(ch1) )
			{
				fprintf(stderr, "Nibble %c is not a hex digit\n", ch1);
				exit(1);
			}
			if ( ! isxdigit(ch2) )
			{
				fprintf(stderr, "Nibble %c is not a hex digit\n", ch2);
				exit(1);
			}

			{		// convert to hex
				int		dig1 = toupper(ch1) - '0';
				if ( dig1 > 9 ) dig1 -= 7;
				int		dig2 = toupper(ch2) - '0';
				if ( dig2 > 9 ) dig2 -= 7;
				int hex = (dig1<<4) | dig2;
				putchar((char)hex);
				fflush(stdout);
			}
			++ptr;		// skip space
		}
		// putchar('\n');
		fflush(stdout);
	}

	return 0;		// done
}

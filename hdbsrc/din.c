#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>


/*
 * + Name: IntFromString(2) - Convert an ascii number to binary
 *
 * Synopsis: unsigned long long IntFromString (char *num);
 *
 * Description: Convert ascii string into a binary int.  Assumes the value is in decimal unless prefixed with a 0x which then
 * indicates a hex value.  If the
     value has a leading zero, octal will not be assumed.  The ascii value may have a leading sign
 * character to express a positive or negative value.  Any leading spaces are ignored.
 *
 * 10 - is a decimal ten -10 - is
     a negative decimal ten 0x10 - is a hex 10 or decimal 16 -0x10 - is a negative hex 10 010 - is
 * a decimal 10 -010 - is an negative octal 10
 *
 * Returns the converted binary value or 0xdeadbeef -
 */

unsigned long long
IntFromString(const char *num, int *err)
{
 int         errBoolean;

 if (err == NULL)
  err = &errBoolean;
 *err = 0;

 num = &num[strspn(num, " \t")]; /* skip leading whitespace */

 /*
  * check for leading sign character
  */

 char            sign;

 if (*num == '-' || *num == '+')
  sign = *num++;
 else
  sign = '+';

 num = &num[strspn(num, " \t")]; /* skip leading whitespace */

 if (!isdigit(*num)) {
  *err = 1;
  return (0xdeadbeef); /* not a number */
 }

 /*
  * check for leading radix indicator
  */

 int             radix;

 if (*num == '0' && tolower(num[1]) == 'x') {
  radix = 16;
  num += 2;
 } else {
  radix = 10;
 }

 unsigned long long result = 0;

 for (int digit; isxdigit(*num);) {
  digit = toupper(*num++);
  digit -= '0';
  if (digit > 9) {  /* hex range */
   if (radix != 16) /* not hex radix */
    break;
   digit -= (('A' - '9') - 1);
  } else if (digit > 7 && radix == 8) {
   break;    /* non octal digit */
  }

  result = (result * radix) + digit;
 }

 if (sign == '-')   /* negative value */
  result = 0 - result;

 return (result);
}


unsigned long long
IntFromHexString(const char *num, int *err)
{
 char  *hex = (char*)calloc(1, strlen(num) + 3);

 sprintf(hex, "0x%s", num);
 unsigned long long i = IntFromString(hex, err);

 free(hex);
 return i;
}


int
main(int argc, char *argv[])
{
  char bfr[BUFSIZ];

  // freopen("od", "r", stdin);
  int dino = open("dino", O_WRONLY|O_CREAT , 0666);

  while( gets(bfr) != NULL ) {
    char *cp = bfr;
    if( (cp = strchr(cp, ':')) ) {
      ++cp;
 // 93664: 30 30 30 37 33 34 43 45 44 49 4e 53 54 20 31 31  000734CEDINST 11
      for(int i = 0 ; i < 16; ++i) {
        char *num = ++cp;
        while(*cp && *cp != ' ')
          ++cp;
        *cp = '\0';
        int err;
        unsigned long long v = IntFromHexString(num, &err);
        unsigned char byt[1];
        byt[0] = (unsigned char)v;
        write(dino, byt, 1);
      }
      puts("");
    }
  }

  close(dino);
  return 0;
}

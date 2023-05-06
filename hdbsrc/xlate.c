#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



/* xlate  <from>  <to>  <files>... */

#define TRUE      1
#define FALSE     0


void show_usage()
{

  fprintf(stderr, "xlate: from_pattern  to_pattern  file_spec\n");
  exit(0);
}


int xlate_file(char *from, char *to, char *iname, FILE *ifile, FILE *ofile)
{
  char    bfr[BUFSIZ];
  char    *bp;
  char    *cp;
  char    ch;
  int     len;
  int     changed = 0;


  len = (int)strlen(from);

  while( fgets(bfr, sizeof(bfr), ifile) ) {
    bp = bfr;
    while( (cp = strchr(bp, *from)) != NULL ) {
      if( strncmp(cp, from, len) == 0 ) {     // match
        *cp = '\0';     // terminate substring
        fputs(bp, ofile);
        fputs(to, ofile);
        bp = cp + len;
        ++changed;
      }
      else
      {
        ch = *(++cp);
        *cp = '\0';     // terminate substring
        fputs(bp, ofile);
        *cp = ch;
        bp = cp;
      }
    }
    if( *bp != '\0' )
      fputs(bp, ofile);
  }

  if( changed )
    fprintf(stderr, "Changed %d strings in %s.\n", changed, iname);

  return changed ? 1:0;
}


int xlate(char *from, char *to, char *iname)
{
  FILE    *ifile;
  FILE    *ofile;
  char    *oname;
  int     changed = 0;

  if( (ifile = fopen(iname, "r")) == NULL ) {
    fprintf(stderr, "Can't open %s\n", iname);
    perror("fopen");
    return 0;
  }

// get directory base

  oname = "hdbtmp.xlateo";

  if( (ofile = fopen(oname, "w")) == NULL ) {
    fprintf(stderr, "Unable to create a temporary file\n");
    perror("fopen");
    return 0;
  }

  changed = xlate_file(from, to, iname, ifile, ofile);

  fclose(ifile);
  fclose(ofile);

// done
// rename output file if the input file was changed

  if( changed ) {
    unlink(iname);
    if( rename(oname, iname) < 0 ) {
      fprintf(stderr, "Unable to relink output file: %s\n", iname);
      perror("rename");
      unlink(oname);
      return 0;
    }
  }
  else            // otherwise, remove the output file
  {
    unlink(oname);
  }

  return changed;
}


int main(int argc, char *argv[])
{
  char    *from;
  char    *to;
  int     nc = 0;


  --argc;
  ++argv;

  if( argc <= 0 ) {
    show_usage();
  }

  switch( argc ) {
    case 1:         // xlate <from> to nil from stdin to stdout
      xlate_file(argv[0], "", "stdin", stdin, stdout);
      break;

    case 2:         // xlate <from> to <to> from stdin to stdout
      xlate_file(argv[0], argv[1], "stdin", stdin, stdout);
      break;

    default:        // <from> and <to> and <file>s...
      argc -= 2;
      from = *argv++;
      to = *argv++;
      while( argc-- > 0 ) {    // while file names
        nc += xlate(from, to, *argv++);
      }
  }

  fprintf(stderr, "Changed %d files.\n", nc);
  return 0;
}

/*
 * conqinit - utility program
 *
 * $Id$
 *
 */

#include "c_defs.h"

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
  
#include "global.h"
#include "color.h"

#include "conqinit.h"

/* print_usage - print usage. Duh. */
void print_usage()
{
  fprintf(stderr, "Usage: conqinit [-f <file>] [-vdD] \n");
  fprintf(stderr, "       conqinit -f <file> -h [-vd] \n\n");
  fprintf(stderr, "\t-f <file>     read from <file>.\n");
  fprintf(stderr, "\t-v            be verbose about everything.\n");
  fprintf(stderr, "\t-d            turns on debugging.\n");
  fprintf(stderr, "\t-t            parse texture data instead of conqinit data\n");
  fprintf(stderr, "\t-h            dump parsed file to stdout in initdata.h/texdata.h format\n");
  fprintf(stderr, "\t              The output format chosen depends on the \n"
                  "\t              presence of the '-t' option\n");
  fprintf(stderr, "\t-D            dump current universe to stdout in conqinitrc format\n");
}


int main(int argc, char **argv)
{
  extern char *optarg;
  extern int optind, opterr, optopt;
  int debuglevel = 0, verbosity = 0, ftype = CQI_FILE_CONQINITRC;
  int ch;
  int doheader = 0;
  char *filenm = NULL;

  rndini(0, 0);
  
  setSystemLog(FALSE, TRUE);    /* log + stderr! :) */

  while ( (ch = getopt( argc, argv, "vdDf:ht" )) != EOF )
    {      switch(ch)
	{
	case 'v':
	  verbosity = TRUE;
	  break;
        case 'd':
          debuglevel++;
          break;
        case 'h':
          doheader = TRUE;
          break;
        case 'D':
          dumpUniverse();
          return 0;
          break; /* NOTREACHED */
        case 'f':
          filenm = optarg;
          break;
        case 't':
          ftype = CQI_FILE_TEXTURESRC_ADD;
          break;
	default:
	  print_usage();
	  return 1;
	}
    }
  
  if (cqiLoadRC(ftype, filenm, verbosity, debuglevel))
    {
      if (doheader)
        {
          if (ftype == CQI_FILE_TEXTURESRC_ADD)
            dumpTexDataHdr();
          else
            dumpInitDataHdr();

        }
    }

  return 0;

}

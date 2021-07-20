
#include "edt_si5338.h"


static void
usage(char *progname)
{
  printf("%s: %s\n", progname, ABOUT_TEXT);
  printf("usage: %s [-u unit] [-i] [-c clock freq]\n\n", progname);

  puts("-u unit - sets device number (default 0)");
  puts("-i - initializes si5338 with default settings");
  puts("-C clock freq - changes desired clock [0-3] to desired frequency [5-350 MHz]");
  puts("                si5338 must be initialized prior to setting clocks");
  puts("                this will be done automatically as needed");
  puts("-v - verbose");
}


int
main(int argc, char *argv[])
{
  char *progname = argv[0];
  int i;
  int unit = 0;
  int verbose = 0;
  int clk;
  EdtDev *edt_p = NULL;

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
    {
      if (argv[i][1] == 'v')
        verbose = 1;
      else if (argv[i][1] == 'u')
      {
        if (argc > i+1)
        {
          if (argv[i+1][0] != '-')
          {
            unit = atoi(argv[i+1]);
            i++;
          }
          else
          {
            fprintf(stderr, "%s\n", "Error: invalid arguments");
            usage(progname);
            exit(1);
          }
        }
        else
        {
          fprintf(stderr, "%s\n", "Error: invalid arguments");
          usage(progname);
          exit(1);
        }
      }
      else if (argv[i][1] == 'i')
      {
	if ((edt_p = edt_open(EDT_INTERFACE, unit)) == NULL)
	{
	    perror("edt_open_channel") ;
	    exit(1) ;
	}

        edt_si5338_init(edt_p, verbose);
	edt_close(edt_p);
      }
      else if (argv[i][1] == 'C')
      {
        if (argc > i+2)
        {
          if (argv[i+1][0] != '-' &&
              argv[i+2][0] != '-')
          {
	    int clk = atoi(argv[i+1]); 
	    double freqMHz = atof(argv[i+2]); 

	    if ((edt_p = edt_open(EDT_INTERFACE, unit)) == NULL)
	    {
		perror("edt_open_channel") ;
		exit(1) ;
	    }

	    edt_si5338_setFreq(edt_p, clk, freqMHz*1000000.0, verbose);
	    edt_close(edt_p);

            i+=2;
          }
          else
          {
            fprintf(stderr, "%s\n", "Error: invalid arguments");
            usage(progname);
            exit(1);
          }
        }
        else
        {
          fprintf(stderr, "%s\n", "Error: invalid arguments");
          usage(progname);
          exit(1);
        }
      }
      else
      {
        fprintf(stderr, "%s\n", "Error: unknown argument");
        usage(progname);
        exit(1);
      }
    }
    else
    {
      fprintf(stderr, "%s\n", "Error: no options selected");
      usage(progname);
      exit(1);
    }
  }

  return 0;
}

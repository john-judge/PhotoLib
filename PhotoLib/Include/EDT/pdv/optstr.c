/**
 * @file optstr.c
 * A utility application that uses the edt library get the edt
 * bitfile option string, an embedded string with information
 * about the bitfile (FPGA configuration file) embedded in the file.
 *
 * Copyright (c) 2018 Engineering Design Team (EDT), Inc.
 * All rights reserved.
 *
 * This file is subject to the terms and conditions of the EULA defined at
 * www.edt.com/terms-of-use
 *
 * Technical Contact: tech@edt.com
 */

#include "edtinc.h"
#include <stdlib.h>

void usage(char *err)
{
  printf(err);
  printf("usage: optstring [args]\n");
  printf("    -u <unit>      - specifies EDT board unit (default 0), or\n");
  printf("    -u <dev><unit> - specifies device (pdv or pcd) and unit (default %s0)\n",
         EDT_INTERFACE);
  printf("                     (useful when devices of different types are present)\n");
  printf("    -h             - this help message\n");

  exit(1);
}

int main(int argc, char **argv)
{
  EdtDev *edt_p;
  int unit = 0;
  int exitval = 0;
  char *unitstr = "";
  char dev[EDT_STRBUF_SIZE];

  dev[0] = '\0';

  while (argc > 1 && argv[1][0] == '-')
  {
    switch (argv[1][1])
    {
      case 'u':
        if (argc < 1)
        {
          usage("unfinished -u option");
          exitval = 1;
        }
        else
        {
          --argc;
          ++argv;
          unitstr = argv[1];
        }
        break;

      case 'h':
        usage("");
        break;

      default:
        usage("unknown option\n");
    }
    --argc;
    ++argv;
  }

  if (argc > 1)
  {
    usage("");
    exit(1);
  }

  if (*unitstr)
    unit = edt_parse_unit(unitstr, dev, NULL);
  else
    unit = 0;

  if (dev[0] == 0)
    strcpy(dev, EDT_INTERFACE);

  if ((edt_p = edt_open_quiet(dev, unit)) == NULL)
  {
    printf("device %s%d not found\n", dev, unit), exit(1);
  }

  if ((edt_p->bfd.bitfile_name[0] == 0))
    edt_get_board_description(edt_p, TRUE);

  if (edt_read_option_string(edt_p, edt_p->bfd.optionstr, NULL) == 0)
    printf("%s%d option string %s\n", dev, unit, edt_p->bfd.optionstr);
  else
  {
    printf("%s%d option string not found\n", dev, unit);
    exitval = 1;
  }

  edt_close(edt_p);

  exit(exitval);
}

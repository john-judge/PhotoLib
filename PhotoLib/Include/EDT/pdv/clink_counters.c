
#include "edtinc.h"

/* note these will be in the newest version of "edtreg.h" */

void
print_clink_counters(PdvDev *pdv_p)

{
	int htot, hact, vtot, vact;

	htot = edt_reg_read(pdv_p, PDV_CL_HTOT);
	hact = edt_reg_read(pdv_p, PDV_CL_HACT);

	vtot = edt_reg_read(pdv_p, PDV_CL_VTOT);
	vact = edt_reg_read(pdv_p, PDV_CL_VACT);

	printf("HTOT %04d\nHACT %04d\nVTOT %04d\nVACT %04d\n",
		  htot, hact, vtot, vact);

}

int main(int argc, char **argv)

{

	int channel = 0;
	int unit = 0;
	int i;

	PdvDev *pdv_p;

	for (i=1;i<argc;i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
				case 'c':
					i++;
					if (i < argc)
						channel = atoi(argv[i]);
				break;
				case 'u':
					i++;
					if (i < argc)
						unit = atoi(argv[i]);
				break;
			}
		}

	}

	pdv_p = pdv_open("pdv",0);

	print_clink_counters(pdv_p);

	pdv_close(pdv_p);

}

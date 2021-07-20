/* Special code for serial number stuff */

	    case 'O':
		if ((strlen(argv[i]) < 3)
		 || ((strcmp(&argv[i][2], "ssp") != 0)
		  && (strcmp(&argv[i][2], "drs") != 0)
		  && (strcmp(&argv[i][2], "and") != 0)))
		{
		    usage("invalid '-O' option");
		    ret = 1;
		}

		if (++i == argc)
		    usage("unfinished -O option");
		if (strlen(argv[i]) > OSN_SIZE-5)
		{
		    usage("invalid -O option");
		    ret = 1;
		}
		strcpy(new_osn, argv[i]);
		break;
	    case 'E':
		if (++i == argc)
		{
		    usage("unfinished -E option");
		    ret = 1;
		}
		if (strlen(argv[i]) > ESN_SIZE-5)
		{
		    usage("-E arg length > max (27)");
		    ret = 1;
		}
		strcpy(new_esn, argv[i]);
		break;


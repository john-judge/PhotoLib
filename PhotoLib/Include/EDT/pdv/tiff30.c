


#include "edtinc.h"

#include "tiffio.h"

/* Convert the duncantech 30-bit raw file to 48 bit rgb */
/* Set shift to 6 to move data to high side of 16-bit range */

int copy_30bgr_to_48rgb(u_char *src, u_short *dest, int width, int height, int shift)

{
	int x,y;

	u_short *dp;
	u_char *sp;

	int delta_d = width *3;
	int delta_s = width*4;


	int r,g,b;

	for (y=0;y<height;y++)
	{
		sp = src+delta_s*y;
		dp = dest+delta_d*y;

		for (x=0;x<width;x++)
		{
			b = sp[0];
			g = sp[1];
			r = sp[2];

			dp[0] = (r << 2 | ((sp[3] & 0x30)>>4)) << shift;
			dp[1] = (g << 2 | ((sp[3] & 0xc) >> 2)) << shift;
			dp[2] = (b << 2 | ((sp[3] & 3))) << shift;

			sp += 4;
			dp += 3;
		}
	}

	return 0;
}

/* write out a 48 bit tif file */

int write_tif_48(char *filename, u_short *data, int width, int height)

{

	u_short *dp = data;
	int row;

	int pitch = 3*width;

	/* attempt oto open file */
	TIFF* tif = TIFFOpen(filename, "w");

	// fill in header
	TIFFSetField( tif,TIFFTAG_IMAGEWIDTH,	width);
	TIFFSetField( tif,TIFFTAG_IMAGELENGTH,	height );
	TIFFSetField( tif,TIFFTAG_BITSPERSAMPLE,	16 );
	TIFFSetField( tif,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT );
	TIFFSetField( tif,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_RGB );

	TIFFSetField( tif,TIFFTAG_DOCUMENTNAME,filename );
	TIFFSetField( tif,TIFFTAG_SAMPLESPERPIXEL,3 );
	TIFFSetField( tif,TIFFTAG_ROWSPERSTRIP, height );
	TIFFSetField( tif,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG );

	/* write out the scanlines */
	for (row = 0;row<height;row++,dp+=pitch)
		TIFFWriteScanline(tif,dp,row,0);
	
	/* close the file */

	TIFFClose(tif);


	return 0;

}


int main(int argc, char **argv)

{

	int width = 0;
	int height = 0;

	FILE *f;
	u_char *src_buffer;
	u_short *dest_buffer;

	int shift = 6;
	int i;

	char filename[MAX_PATH];
	char outfilename[MAX_PATH];
	filename[0] = '\0';
	outfilename[0] = '\0';

	for (i=1;i<argc;i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case 'f':

				i++;

				if (i < argc)
					strcpy(filename,argv[i]);

				break;
			case 'o':

				i++;

				if (i < argc)
					strcpy(outfilename,argv[i]);

				break;

			case 'w':
				i++;
				if (i < argc)
					width = atoi(argv[i]);
				break;

			case 'h':
				i++;
				if (i < argc)
					height = atoi(argv[i]);

				break;


			}

		}

	}

	if (width > 0 && height > 0 && 	outfilename[0])
	{
		src_buffer = (u_char *) malloc(width*height*4);
		dest_buffer = (u_short *) malloc(width*height*6);

		if (filename[0])
		{

			f = fopen(filename,"r");
			fread(src_buffer,width*height*4,1,f);
			fclose(f);
		
		}
		else
		{
			u_char *sp = src_buffer;
			int r,g,b,x,y;
			
			for (y=0;y<height;y++)
			{
				r = y;
				g = width;
				b = 0;
				for (x=0;x<width*4;x+=4)
				{
					sp[x] = (b >> 2) & 0xff;
					sp[x+1] = (g >> 2) & 0xff;
					sp[x+2] = (r >> 2) & 0xff;

					sp[x+3] = ((b & 3) << 4) + ((g & 3) << 2) + (r & 3);

					g--;
					b++;
				}
				sp += width*4;
			}
		}
		copy_30bgr_to_48rgb(src_buffer,dest_buffer,width,height, shift);
	

		write_tif_48(outfilename,dest_buffer, width, height);

	}

	return 0;
}

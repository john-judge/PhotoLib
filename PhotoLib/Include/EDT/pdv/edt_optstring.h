
#ifndef _EDT_OPTSTRING_H_
#define _EDT_OPTSTRING_H_

#define EDT_BITFILE_VERSION  0x0201007c
#define EDT_BITFILE_STRING   0x0101007e
#define EDT_OLD_BITFILE_STRING   0x0101007d
#define EDT_MEZZ_BITFILE_STRING  0x010100A0




EDTAPI int
edt_get_board_description(EdtDev *edt_p, 
		int force_mezzanine);

EDTAPI void
edt_print_board_description(EdtDev *edt_p);


EDTAPI int
edt_parse_option_string(char *s, EdtBitfileDescriptor *bfd);

EDTAPI char * edt_mezz_name(int mezz_id);

EDTAPI int edtdev_channels_from_type(EdtDev *edt_p);


EDTAPI int edt_is_test_file_loaded(EdtDev *edt_p);

EDTAPI int edt_read_option_string(EdtDev *edt_p, char *s , int *rev_p);


#endif


#include <stdio.h>

#include <stdlib.h>

#include <malloc.h>

#include <ctype.h>

#include <string.h>

#ifdef _WIN32

#define CONST const

#endif


#include "ConfigDataSet.h"

#define ALLOC_GRAIN 16

#define CFGSET_DEFAULT_SEPARATOR ':'


void cfgitem_set_value(ConfigData *pItem, CONST char  *Value)

{
	if (pItem->Value)
	{
		free(pItem->Value);
	}

	if (Value)
		pItem->Value = strdup(Value);
	else
		pItem->Value = NULL;

}

void cfgitem_set_name(ConfigData *pItem, CONST char *Name)

{
	if (pItem->Name)
	{
		free(pItem->Name);
	}

	if (Name)
		pItem->Name = strdup(Name);
	else
		pItem->Name = NULL;

}

void cfgitem_set_comment(ConfigData *pItem, CONST char  *Comment)
{
	if (pItem->Comment)
	{
		free(pItem->Comment);
	}

	if (Comment)
		pItem->Comment = strdup(Comment);
	else
		pItem->Comment = NULL;

}

void cfgitem_set_all_values(ConfigData *pItem, CONST char  *Name, CONST char  *Value, CONST char *Comment)

{
	cfgitem_set_name(pItem, Name);
	cfgitem_set_value(pItem, Value);
	cfgitem_set_comment(pItem, Comment);
}


/* cfgitem_init: set values for preallocated pItem.
 *  pItem: configuration Item which has been allocated, and returned probably by 
 *  either cfgset_lookup_item, or cfgset_new_item */
void cfgitem_init(ConfigData *pItem, CONST char  *Name, CONST char  *Value, CONST char  *Comment)

{
	if (pItem)
	{
		pItem->Name = NULL;
		pItem->Value = NULL;
		pItem->Comment = NULL;

		cfgitem_set_all_values(pItem, Name, Value, Comment);
	}
}

void cfgitem_destroy(ConfigData *pItem)

{
	cfgitem_set_name(pItem, NULL);
	cfgitem_set_value(pItem, NULL);	
	cfgitem_set_comment(pItem, NULL);	
}


void cfgset_init(ConfigDataSet * pSet)

{
	memset(pSet, 0, sizeof(*pSet));
	pSet->separator = CFGSET_DEFAULT_SEPARATOR;
}

void cfgset_destroy(ConfigDataSet * pSet)

{
	int i;

	if (pSet->pItems)
	{
		for (i=0;i< pSet->nItems;i++)
			cfgitem_destroy(pSet->pItems + i);

		free(pSet->pItems);
		pSet->pItems = NULL;
		pSet->nItems = 0;
		pSet->nItemsAllocated = 0;
	}

	if (pSet->Name)
	{
		free(pSet->Name);
		pSet->Name = NULL;
	}

}

static ConfigData *
cfgset_new_item(ConfigDataSet *pSet)

{

	pSet->nItems++;
	if (pSet->nItems > pSet->nItemsAllocated)
	{

		int nAllocated = pSet->nItemsAllocated + ALLOC_GRAIN;


		if (pSet->pItems)
		{
			pSet->pItems = (ConfigData *) realloc(pSet->pItems, 
				sizeof(ConfigData) * nAllocated);

			memset(pSet->pItems + pSet->nItemsAllocated, 0, sizeof(ConfigData) * ALLOC_GRAIN);
			
		}
		else
		{
			pSet->pItems = (ConfigData *) calloc(nAllocated, sizeof(ConfigData));

		}

		pSet->nItemsAllocated += ALLOC_GRAIN;

	}

	return pSet->pItems + pSet->nItems - 1;

}

void cfgset_add_item(ConfigDataSet * pSet, CONST char  * Name, CONST char  *Value, CONST char  *Comment)

{
	ConfigData *pItem;

	if ((pItem = cfgset_lookup_item(pSet, Name)))
	{
		cfgitem_set_value(pItem, Value);
		cfgitem_set_comment(pItem, Comment);
	}
	else
	{
		pItem = cfgset_new_item(pSet);
		cfgitem_init(pItem, Name, Value, Comment);
	}

}

ConfigData * cfgset_lookup_item(ConfigDataSet * pSet, CONST char  *Name)

{
	int index;

	if ((index = cfgset_lookup_index(pSet,Name)) >= 0)
	{
		return pSet->pItems + index;
	}

	return NULL;
}

int cfgset_lookup_index(ConfigDataSet * pSet, CONST char  *Name)

{
	int index;

	if (Name)
		for (index = 0;index < pSet->nItems; index++)
			if (pSet->pItems[index].Name)
				if (!strcmp(pSet->pItems[index].Name, Name))
					return index;

	return -1;
}

char  *cfgset_lookup(ConfigDataSet * pSet, CONST char  *Name)

{
	ConfigData *pItem;

	if ((pItem = cfgset_lookup_item(pSet, Name)))
		return pItem->Value;
	else
		return NULL;

}

int cfgset_get_int(ConfigDataSet * pSet, CONST char  *Name, int *pValue)

{
	char *value;

	if ((value = cfgset_lookup(pSet,Name)))
	{
		*pValue = strtol(value,NULL,0);

		return 1;
	}
	else
		return 0;

}

int cfgset_get_double(ConfigDataSet * pSet, CONST char  *Name, double *pValue)

{
	char *value;

	if ((value = cfgset_lookup(pSet,Name)))
	{
		*pValue = atof(value);

		return 1;
	}
	else
		return 0;

}
int cfgset_get_float(ConfigDataSet * pSet, CONST char  *Name, float *pValue)

{
	char *value;

	if ((value = cfgset_lookup(pSet,Name)))
	{
		*pValue = (float) atof(value);

		return 1;
	}
	else
		return 0;

}
int  cfgset_set_string(ConfigDataSet * pSet, CONST char  *Name, CONST char *Value)

{

	cfgset_add_item(pSet, Name, Value, NULL);
	
	return 1;

}

int cfgset_set_int(ConfigDataSet * pSet, CONST char  *Name, int iValue)

{
	char Value[256];

	sprintf(Value, "%d", iValue);

	cfgset_add_item(pSet, Name, Value, NULL);
	
	return 1;
}

int cfgset_set_double(ConfigDataSet * pSet, CONST char  *Name, double dValue)

{
	char Value[256];

	sprintf(Value, "%g", dValue);

	cfgset_add_item(pSet, Name, Value, NULL);
	
	return 1;

}

int cfgset_set_float(ConfigDataSet * pSet, CONST char  *Name, float dValue)

{
	char Value[256];

	sprintf(Value, "%g", dValue);

	cfgset_add_item(pSet, Name, Value, NULL);
	
	return 1;

}
void cfgset_delete_item(ConfigDataSet * pSet, int n)

{

	if (pSet && n >= 0 && n < pSet->nItems)
	{
		/* delete element of array */
		cfgitem_destroy(pSet->pItems + n);

		if (n < pSet->nItems - 1)
		{
		
			memcpy(pSet->pItems + n, pSet->pItems + n + 1, 
				sizeof(ConfigData) * (pSet->nItems - n));
		}

		pSet->nItems--;

	}

}

void cfgset_delete_named_item(ConfigDataSet * pSet, CONST char  *Name)

{
	int n;

	n = cfgset_lookup_index(pSet, Name);

	cfgset_delete_item(pSet,n);

}

void cfgset_set_values(ConfigDataSet * pSet, int nItem, CONST char  *Name, CONST char  *Value, CONST char  *Comment)

{
	if (pSet && (nItem >= 0) && (nItem < pSet->nItems))
	{
		cfgitem_set_all_values(pSet->pItems + nItem, Name, Value, Comment);
	}
}


char *
strtrim(char *s)

{
	char *news = s;

	if (!s)
		return s;

	while (isspace(*news))
		news++;

	if (*news)
	{
		int last = strlen(news) -1;
	
		while (last && isspace(news[last]))
			last--;

		news[last+1] = 0;
	}

	return news;
}


int
cfgset_parse_name_value(char *s, char **namep, char **valuep, char separator)

{
	char *name;
	char *value;
	char *colon;

	if (s)
	{
		colon = strchr(s,separator);

		if (colon)
		{
			name = s;

			*colon = 0;

			name = strtrim(name);
		
			value = colon + 1;

			value = strtrim(value);

			*namep = name;
			*valuep = value;

			return 1;
		}
	}

	return 0;
}


int cfgset_read_line(ConfigDataSet * pSet, char *buffer)

{
	char *name;
	char *value;

	if (buffer[strlen(buffer)-1] == '\n')
		buffer[strlen(buffer)-1] = 0;

	if (buffer[0] == '#')
	{
		cfgset_add_item(pSet,NULL,NULL,buffer + 1);
	}
	else
	{
		if (cfgset_parse_name_value(buffer, &name, &value, pSet->separator))
		{
			if (strlen(name) && strlen(value))
				cfgset_add_item(pSet, name, value, NULL);		
		}
		else
		{
			cfgset_add_item(pSet,NULL,NULL,buffer);

		}
	}

	return 0;
}

int cfgset_load_file(ConfigDataSet * pSet, FILE *f)

{
	int rc;

	char buffer[1024];

	while (fgets(buffer, 1023, f))
	{
		if ((rc = cfgset_read_line(pSet,buffer)) != 0)
			return rc;
	}

	return 0;

}

int cfgset_write_line(ConfigDataSet * pSet, FILE *f, int line)

{
	if (pSet && f && line < pSet->nItems)
	{
		if (pSet->pItems[line].Name)
			fprintf(f, "%s: ", pSet->pItems[line].Name);
		if (pSet->pItems[line].Value)
			fprintf(f, "%s", pSet->pItems[line].Value);
		if (pSet->pItems[line].Comment)
			fprintf(f, "# %s", pSet->pItems[line].Comment);

		fprintf(f, "\n");
	}

	return 0;
}

int cfgset_save_file(ConfigDataSet * pSet, FILE *f)

{
	int i;
	int rc;

	for (i=0; i < pSet->nItems;i++)
		if ((rc = cfgset_write_line(pSet, f, i)) != 0)
			return rc;

	return 0;

}

int cfgset_load(ConfigDataSet * pSet, CONST char *Name)

{
	FILE *f;

	if ((f = fopen(Name, "r")))
	{
		cfgset_load_file(pSet,f);

		fclose(f);

		return 0;
	}
	else
		return -1;


}

int cfgset_save(ConfigDataSet * pSet, CONST char *Name)

{
	FILE *f;


	if ((f = fopen(Name, "wb+")))
	{

		cfgset_save_file(pSet,f);

		fclose(f);

		return 0;

	}
	else
		return -1;

}

void
cfgset_reset_index(ConfigDataSet *pSet)

{
    pSet->index = 0;
}

/* Step through config set, skipping comments */
/* return next ConfigData item, or NULL if finished */

ConfigData *
cfgset_get_next(ConfigDataSet *pSet)

{

    do {
	if (pSet->index < pSet->nItems)
	{
	    pSet->index ++;
	}


	if (pSet->index < 0 || pSet->index >= pSet->nItems)
	{
	    pSet->pCurrent = pSet->pItems + pSet->index;
	}
	else
	    pSet->pCurrent = NULL;

    } while (pSet->pCurrent && !pSet->pCurrent->Name);

    return pSet->pCurrent;
}

char *
cfgset_get_current_name(ConfigDataSet *pSet)

{
    if (pSet && pSet->pCurrent)
	return pSet->pCurrent->Name;
    else
	return NULL;
}

char *
cfgset_get_current_value(ConfigDataSet *pSet)

{
    if (pSet && pSet->pCurrent)
	return pSet->pCurrent->Value;
    else
	return NULL;
}




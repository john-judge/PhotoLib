/**
*
*  @file StringUtils.cpp 
* 
*  @copyright Copyright Engineering Design Team 2012
*  All Rights Reserved
*
**/
/** ********************************
 * 
 * @file string_utils.cpp
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */


/*******************************************
*
* Copyright Engineering Design Team 2003
* 
* All rights reserved
*
*  Miscellaneous STL string utilities
*  
*
*******************************************/



#include <iostream>

#include <string>

#include <vector>

#include "StringUtils.h"

#include <ctype.h>

using namespace std;


#define COMMENT_MARK '#'
#define COLON ':'

#define FIELD_DELIMITER '/'

static string null_string = "";

string stringtrim(string &instring)

{

    string::size_type start = 0;
    string::size_type end = 0;

    start = instring.find_first_not_of(" \t\n\r");

    end = instring.find_last_not_of(" \t\n\r", instring.size()-1);

    if (start != string::npos && end != string::npos)
	return instring.substr(start, (end - start + 1) );

    return null_string;
}

bool string_needs_quotes(const string & instring)

{
    string::size_type start = instring.find_first_not_of(" \t\n");
    
    return (start != string::npos);

}

const string string_dequote(const string &instring)

{
    string::size_type start = 0;
    string::size_type end = 0;

    start = instring.find_first_not_of(" \t\n\"");

    end = instring.find_last_not_of(" \t\n\"");

    if (start != string::npos && end != string::npos)
	return instring.substr(start,(end - start + 1));

    return null_string;
    
}

const string stringlower(const string &instring)

{

    int i;

    string news = instring;

    int len = instring.length();

    for (i=0;i<len;i++)
	news[i] = tolower(instring[i]);

    return news;

}

const string stringupper(const string &instring)

{

    int i;

    string news = instring;

    int len = instring.length();

    for (i=0;i<len;i++)
	news[i] = toupper(instring[i]);

    return news;

}

int
string_tokenize(const string &instring, const string &delim, vector<string> &v)

{

    int pos = 0;
    int lastpos = 0;

    // skip initial delimiter?

    while ((pos = instring.find_first_of(delim, lastpos)) != string::npos)
    {
	v.push_back(instring.substr(lastpos, (pos-lastpos)));

	lastpos = instring.find_first_not_of(delim,pos);

	if (lastpos == string::npos)
	    break;

    }

    if (lastpos != string::npos)
	v.push_back(instring.substr(lastpos));

    return v.size();

}

bool
string_split2(const string &instring, const string &delim, string &key, string &value)

{

    int pos = 0;
    int lastpos = 0;

    // skip initial delimiter?

    if ((pos = instring.find_first_of(delim, lastpos)) != string::npos)
    {
	key = instring.substr(0, pos);
	value = instring.substr(pos+1);
	return true;
    }
    else
    {
        key = instring;
        value = "";
        return false;
    }
}

// like tokenize, but creates empty strings between delim chars

int
string_split(const string &instring, const string &delim, vector<string> &v)

{

    int pos = 0;
    int lastpos = 0;

    // skip initial delimiter?

    while ((pos = instring.find_first_of(delim, lastpos)) != string::npos)
    {
	v.push_back(instring.substr(lastpos, (pos-lastpos)));

        lastpos = pos+1;
    }

    if (lastpos != string::npos)
	v.push_back(instring.substr(lastpos));

    return v.size();

}


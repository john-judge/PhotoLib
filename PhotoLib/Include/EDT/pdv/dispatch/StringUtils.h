/**
*
*  @file StringUtils.h 
* 
*  @copyright Copyright Engineering Design Team 2012
*  All Rights Reserved
*
**/
/** ********************************
 * 
 * @file EdtImage/string_utils.h
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


#ifndef _STRINGUTILS_H_
#define _STRINGUTILS_H_

/* on MS disable warning anbout names too long */


#include <string>
#include <vector>

using namespace std;

string  stringtrim(string &instring);
const string  stringlower(const string &instring);
const string  stringupper(const string &instring);

int string_tokenize(const string &instring, const string &delim, vector<string> &v);
bool string_split2(const string &instring, const string &delim, string &key, string &value);
int string_split(const string &instring, const string &delim,  vector<string> &v);
bool string_needs_quotes(const string &instring);

const string  string_dequote(const string &instring);


#endif

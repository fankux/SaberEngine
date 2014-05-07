#ifndef FSTR_H
#define FSTR_H

#include <inttypes.h>

typedef struct fstr{ //string
	int len;
	int free;
	char buf[];
}fstr;

/* API */
fstr * fstrCreateLen(const char * str, const size_t len);
fstr * fstrCreate(const char * str);
fstr * fstrCreateInt(const int64_t value);
void fstrFree(fstr * str);
fstr * fstrCatLen(fstr * a, const char * b, const size_t len);
fstr * fstrCat(fstr * a, const char * b);
fstr * fstrTrim(fstr * s, const int FLAG);
fstr * fstrSet(fstr * a, char * b, const size_t pos);
fstr * fstrSetLen(fstr * a, char * b, const size_t pos, const size_t len);
fstr * fstrRemoveLen(fstr * s, const size_t start, const size_t len);
fstr * fstrDupLen(fstr * a, const size_t start, const size_t len);
fstr * fstrDup(fstr * a);
fstr * fstrReplaceLen(fstr * a, char * b,
					  const size_t pos, const size_t len);
fstr * fstrDupEndPoint(fstr * b,const char c);
fstr * fstrReplace(fstr * a, char * b, const size_t pos);
fstr * fstrInsertLen(fstr * a, char * b,
					 const size_t pos,
					 const size_t len);
fstr * fstrInsert(fstr * a, char * b, const size_t pos);
void fstrSqueeze(fstr * a);
int fstrCompare(fstr * a, fstr * b);
void fstrInfo(fstr * str);
#endif

#ifndef COMMON_H
#define COMMON_H

#define typeof __typeof__
#define min(x, y) ({ \
         typeof(x) _min1 = (x); \
         typeof(y) _min2 = (y); \
         (void) (&_min1 == &_min2); \
         _min1 < _min2 ? _min1 : _min2; })
   
#define max(x, y) ({ \
         typeof(x) _max1 = (x); \
         typeof(y) _max2 = (y); \
         (void) (&_max1 == &_max2); \
         _max1 > _max2 ? _max1 : _max2; })
         
int _kmpnext(int pattern[]);
int kmp(const char search[], const char pattern[], int start);


#endif

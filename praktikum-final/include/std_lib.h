#ifndef __STD_LIB_H__
#define __STD_LIB_H__

#include "std_type.h"
#include <stddef.h>
// #include <string.h>

int div(int a, int b);
int mod(int a, int b);

void memcpy(byte* dst, byte* src, unsigned int size);
unsigned int strlen(char* str);
bool strcmp(char* str1, char* str2);
void strcpy(char* dst, char* src);
void clear(byte* buf, unsigned int size);

// char* _strchr(const char* str, int c); // Declaration for _strchr
// char* _strrchr(const char* str, int c); // Declaration for _strrchr

/**
 * TODO: Add your general helper function here
 * ...
 */

// char* _strtok(char* str, const char* delim);

#endif // __STD_LIB_H__

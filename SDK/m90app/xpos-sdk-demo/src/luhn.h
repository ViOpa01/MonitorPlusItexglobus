/**
 * File: luhn.h
 * ------------
 * Defines a new interface for luhn.
 */


#ifndef _ITEX_LUHN_INCLUDED
#define _ITEX_LUHN_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

int getLuhnDigit(char *number, const int size);
short checkLuhn(const char * cardNo, const int size);

#ifdef __cplusplus
}
#endif

#endif

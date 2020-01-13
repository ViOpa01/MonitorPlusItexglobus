/**
 * File: EmvEft.h
 * --------------
 * Defines a new interface for Emv Eft transactions.
 * @author Opeyemi Ajani.
 */



#ifndef _EMV_EFT_INCLUDED
#define _EMV_EFT_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#include "nibss.h"

typedef struct IccDataT {
	unsigned int tag;
	short present;
} IccDataT;

void eftTrans(const enum TransType transType);
int buildIccData(unsigned char * de55, const IccDataT * iccData, const int size);

#ifdef __cplusplus
}
#endif


#endif


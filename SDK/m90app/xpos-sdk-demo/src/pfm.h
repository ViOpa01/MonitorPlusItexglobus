/**
 * File: pfm.h
 * -----------
 * Defines a new interface for pfm.
 */


#ifndef _PFM_230918_H_
#define  _PFM_230918_H_

#include "vas/jsobject.h"
#include "Nibss8583.h"


short getStateStr(char * strState);
iisys::JSObject getState();
iisys::JSObject getJournal(const Eft* trxContext);


#endif

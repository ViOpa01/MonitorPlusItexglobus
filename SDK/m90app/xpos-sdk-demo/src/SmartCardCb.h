/**
 * File: SmartCardCb.h
 * -------------------
 * Callbacks to be implemented by the app.
 * @author Opeyemi Ajani Sunday, Itex Integrated Services
 */

#ifndef SMART_CARD_INCLUDED_
#define SMART_CARD_INCLUDED_

#include "basefun.h"

#ifdef __cplusplus
extern "C" {
#endif

int cardPostion2Slot(int * slot, const enum TCardPosition CardPos);
int resetCard(int slot, unsigned char *adatabuffer);
int closeSlot(int slot);
unsigned char iccIsoCommand(int slot, APDU_SEND *apduSend, APDU_RESP *apduRecv);

#ifdef __cplusplus
}
#endif

#endif

#ifndef SRC_AUX_PAYLOAD_
#define SRC_AUX_PAYLOAD_

#include "Nibss8583.h"

int addPayloadGenerator(Eft* context, AuxPayloadGenerator generator, void* data);
extern "C" int genAuxPayloadString(char* auxPayload, const size_t auxPayloadSize, const void *eft);

#endif

#include <stdlib.h>
#include <string.h>

#include "./vas/jsobject.h"
#include "auxPayload.h"


int addPayloadGenerator(Eft* context, AuxPayloadGenerator generator, void* data)
{
    const size_t callbackSize = sizeof(context->vas.auxPayloadGenerators) / sizeof(context->vas.auxPayloadGenerators[0]);
    const size_t dataSize = sizeof(context->vas.callbackdata) / sizeof(context->vas.callbackdata[0]);

    if (dataSize != callbackSize) {
        return -1;
    }

    long i = -1;
    while (++i < dataSize && context->vas.auxPayloadGenerators[i]) { }

    if (i == dataSize) {
        return -1;
    }

    context->vas.auxPayloadGenerators[i] = generator;
    context->vas.callbackdata[i] = data;

    return 0;
}

iisys::JSObject runPayloadGenerators(const Eft* context)
{
    iisys::JSObject json;
    
    const size_t callbackSize = sizeof(context->vas.auxPayloadGenerators) / sizeof(context->vas.auxPayloadGenerators[0]);
    const size_t dataSize = sizeof(context->vas.callbackdata) / sizeof(context->vas.callbackdata[0]);

    if (dataSize != callbackSize) {
        return json;
    }

    for (size_t i = 0; i < dataSize && context->vas.auxPayloadGenerators[i]; ++i) {
        context->vas.auxPayloadGenerators[i](&json, context->vas.callbackdata[i], context);
    }

    // i = 0
    // { }
    // {"vasData": {}}
    // i = 1
    // {"vasData": {}}
    // {"vasData": {}, "ereceipt": {}}

    return json;
}

// Don't call this function in any of the callbacks to avoid stories that touch the heart :)
int genAuxPayloadString(char* auxPayload, const size_t auxPayloadSize, const void *eft)
{
    Eft *context = (Eft*)eft;

    if (!context->vas.auxPayloadGenerators[0]) {
        return 0;
    }

    // if (!context->auxPayloadGenerators[0]) {
    //     return 0;
    // }

    iisys::JSObject auxData = runPayloadGenerators(context);
    if (auxData.isNull()) {
        return -1;
    }

    const std::string payload = auxData.getString();
    if (payload.length() >= auxPayloadSize) {   // Will overflow input buffer
        return -1;
    }

    strcpy(auxPayload, payload.c_str());

    return (int) payload.length();
}

#include <string.h>
#include <time.h>
#include <stdio.h>

#include "vasdevice.h"
#include "../../merchant.h"

extern "C" {
#include "../../appInfo.h"
#include "../../util.h"
}


std::string vasimpl::menuendl()
{
    // return "<br/>";
    return " ";
}

std::string vasimpl::getDeviceModel()
{
    char deviceModel[32] = {'\0'};
    
    sprintf(deviceModel, "%s%s", TERMINAL_MANUFACTURER, APP_MODEL);
    return std::string(deviceModel);
}

std::string vasimpl::getDeviceSerial()
{
    char serial[16] = {'\0'};

    getTerminalSn(serial);
    return std::string(serial);
}

std::string vasimpl::getDeviceTerminalId()
{
    MerchantData mParam;
    static char tid[9] = {'\0'};
    
    memset(&mParam, 0x00, sizeof(MerchantData));
    readMerchantData(&mParam);
    memset(tid, 0, sizeof(tid));
    strcpy(tid, mParam.tid);
    
    return tid;
}

#include "tribeaseGetClientData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../vas/platform/simpio.h"
#include "../ussdservices.h"
#include "tribeaseUtil.h"

extern "C" {
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_util.h"
}

static short getPhoneNo(char* buff, size_t bufLen, char* title)
{
    gui_clear_dc();
    if (Util_InputMethod(
            GUI_LINE_TOP(0), title, GUI_LINE_TOP(2), buff, 11, 11, 1, 30000)
        < 0) {
        memset(buff, '\0', bufLen);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static short getTribeaseCustomerPhoneNo(TribeaseClient* tribeaseClient)
{
    return getPhoneNo(tribeaseClient->customerPhoneNo,
        sizeof(tribeaseClient->customerPhoneNo), (char*)"CUSTOMER PHONE NO");
}

short tribeaseGetClientData(TribeaseClient* tribeaseClient, int forToken)
{
    if ((tribeaseClient->amount
            = getAmount(ussdServiceToString(TRIBEASE_PURCHASE)))
        == 0UL) {
        UI_ShowButtonMessage(5000, "Invalid Amount/Terminated", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return EXIT_FAILURE;
    }
    if (!forToken
        && getTribeaseCustomerPhoneNo(tribeaseClient) != EXIT_SUCCESS) {
        UI_ShowButtonMessage(
            5000, "Terminated", " ", "Ok", UI_DIALOG_TYPE_WARNING);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

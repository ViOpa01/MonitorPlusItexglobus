/**
 * File: SmartCardCb.c
 * -------------------
 * Implements SmartCardCb.h interface.
 * @author Opeyemi Ajani Sunday, Itex Integrated Services.
 */

#include "SmartCardCb.h"
#include "basefun.h"

//card type -> ISO_7816


#define CUSTOMER_SLOT 1   
#define MERCHANT_SLOT_1 2 //PSAM SLOT 1



int cardPostion2Slot(int * slot, const enum TCardPosition CardPos)
{
    switch (CardPos)
    {
    case CP_TOP:
        *slot = CUSTOMER_SLOT;
        break;

    case CP_BOTTOM:
    case CP_BOTTOM2:
    case CP_BOTTOM3:
        *slot = MERCHANT_SLOT_1;
        break;

    default:
        return -1;
    }

    return 0;
}

int resetCard(int slot, unsigned char *adatabuffer)
{
    //Set card type to ISO_7816
    //Initialize slot, return negative number upon error.
    //result = Init_CardSlot(slot);

    //Reset Card slot, return negative number on error.
    //result = Reset_CardSlot(slot, RESET);

    //get interface part of ATR string
    //iATRLen = Get_Interface_Bytes(slot, adatabuffer); //get interface part of ATR string

    //tempLen = iATRLen;
    ////get historical part of ATR string
    //iATRLen += Get_Historical_Bytes(slot, &adatabuffer[iATRLen]); //get historical part of ATR string

    //Get Card seial no always come from adatabuffer

    return 0;
}


int closeSlot(int slot)
{
    //close the slot
    return 0;
}


unsigned char iccIsoCommand(int slot, APDU_SEND *apduSend, APDU_RESP *apduRecv)
{
    //Send apduSend to the slot, please see APDU_SEND struct
    //Receive apduRecv from the slot, receive APDU_RESP slot.
    return 0; //Success
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include "simpio.h"


extern "C" {
#include "util.h"
#include "../pages/inputamount_page.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_util.h"
}


InputStatus getPin(std::string& pin, const char* title, const int minLength)
{
    char buff[minLength + 1] = {0};
	int result = 0;

    while(1)
    {
        gui_clear_dc();
        result = Util_InputText(GUI_LINE_TOP(0), (char *)title, GUI_LINE_TOP(2), buff, minLength, 4, 0, 2, 10000);

        pin.append(buff);
        printf("Input pin : %s\n", pin.c_str());

        if (result < 0) {
                switch (result) {
                case -2:
                    return EMV_CT_PIN_INPUT_ABORT;
                case -3:
                    return EMV_CT_PIN_INPUT_TIMEOUT;
                default:
                    return EMV_CT_PIN_INPUT_OTHER_ERR;
                }
        } else if (pin.length() >= minLength) {
                break;
        }
    }

    return EMV_CT_PIN_INPUT_OKAY;
	
}

std::string getNumeric(int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType)
{
    char number[128 + 1] = { 0 };
    int result = 0;

    gui_clear_dc();
    result = Util_InputMethod(GUI_LINE_TOP(0), (char *)title, GUI_LINE_TOP(2), number, 1, sizeof(number) - 1, 1, timeout);

    if (result  < 0) {
        return std::string("");
    }

    return std::string(number);
}

int getNumeric(char *val, size_t len, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType)
{
    return getNumeric(val, 1, len - 1, display, timeout, title, prompt, dialogType);

}

int getNumeric(char *val, size_t minlength, size_t maxlength, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType)
{
    int result = 0;

    gui_clear_dc();
    result = Util_InputMethod(GUI_LINE_TOP(0), (char *)title, GUI_LINE_TOP(2), val, minlength, maxlength, 1, timeout);

    if(result < 0)
    {
        switch (result) {
            case -2:
                return -2;
            case -3:
                return -3;
            default:
                return -1;
        }
    } else
    {
        return result;
    }
}

std::string getPhoneNumber(const char* title, const char* prompt, bool uselocale)
{
    char phone[16] = { 0 };
    int result = 0;

    (void) uselocale;   // To be used in the future together with country code for appropraite phone format

    gui_clear_dc();
    result = Util_InputMethod(GUI_LINE_TOP(0), (char *)title, GUI_LINE_TOP(2), phone, 11, 11, 1, 30000);

    if (result  < 0) {
        return std::string("");
    }

    return std::string(phone);
}


int getText(char *val, size_t len, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE type)
{
    int result = 0;

    gui_clear_dc();
    result = Util_InputText(GUI_LINE_TOP(0), (char *)title, GUI_LINE_TOP(2), val, 1, len - 1, 1, 1, timeout);

    if (result < 0) {
        switch (result) {
        case -2:
            return -2;
        case -3:
            return -3;
        default:
            return -1;
        }
    } else {
        return result;
    }

}

int getText(std::string& val, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE type)
{
    char buff[128 + 1] = { 0 };
    int result;

    strncpy(buff, val.c_str(), val.length());
    
    result = getText(buff, sizeof(buff) - 1, display, timeout, title, prompt, type);
    
    if (result > 0) {
        val = buff;
    }

    return result;
}

int UI_ShowOkCancelMessage(int timeout, const char* title, const char* text, UI_DIALOG_TYPE type)
{
    int result = 0;

    gui_clear_dc();
	result = gui_messagebox_show((char *)title, (char *)text, "Cancel", "Ok", timeout);

    switch (result) {
    case 1:
        return CONFIRM;
    case 2:
       return CANCEL;
    case 3:
       return MSG_TIMEOUT;
    default:
        break;
    }

    return CANCEL;
}

int UI_ShowButtonMessage(int timeout, const char* title, const char* text, const char* button, UI_DIALOG_TYPE type)
{

    int result = 0;

    gui_clear_dc();
	result = gui_messagebox_show((char *)title, (char *)text, "", (char *)button, timeout);

    switch (result)
    {
        
    case 2:
       return CANCEL;

    case 3:
       return TIMEOUT;
    
    default:
        break;
    }

    return CONFIRM;

}

int getPassword(std::string& password)
{
    int result;
    char pass[33] = {0};

	// Timeout : -3
	// Cancel  : -2
	// Fail    : -1
	// success  : no of byte(character) entered

    strncpy(pass, password.c_str(), password.length());
    
    gui_clear_dc();
    result = Util_InputMethod(GUI_LINE_TOP(2), "Enter Password", GUI_LINE_TOP(5), pass, 4, sizeof(pass) - 1, 1, 1000);
	
	if (result > 0)
	{
		printf("Password input failed ret : %d\n", result);
		printf("Password %s\n", pass);
        password = pass;
	}

	return result;
}


int UI_ShowSelection(int timeout, const char* title, const std::vector<std::string>& elements, int preselect)
{
    int option = -1;

	std::vector<std::string> tempElememts = elements;
    size_t len = tempElememts.size();
	const char * items[len] = { 0 };

    for(int index = 0; index < len; index++)
    {
        items[index] = tempElememts[index].c_str();
    }
		
    option = gui_select_page_ex((char *)title, (char **)items, len, timeout, preselect);

	switch (option) // if exit : -1, timout : -2
	{
        case -1:
        case -2:
            return -1;

        default:
            break;
	}

    return option;

}

unsigned long getAmount(const char* title)
{
    char buff[24] = {'\0'};

    strncpy(buff, title, sizeof(buff) - 1);
     
    return inputamount_page(buff, 12, 30000);
}


void Demo_SplashScreen(const char *text, const char *text_additional)
{
    gui_messagebox_show((char *)text , (char *)text_additional, "" , "" , 1);
}







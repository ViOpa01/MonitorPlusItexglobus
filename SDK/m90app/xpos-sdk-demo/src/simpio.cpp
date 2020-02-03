#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include "simpio.h"


extern "C" {
#include "util.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_util.h"
}


// using namespace std;
// using namespace vfigui;

// typedef std::map<std::string, std::string> UIParams;


int getPin(std::string& pin, const char* title, const int minLength)
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

std::string getNumeric(int display, int timeout, const char* title, const char* prompt)
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

int getNumeric(char *val, size_t len, int display, int timeout, const char* title, const char* prompt)
{
    return getNumeric(val, 1, len - 1, display, timeout, title, prompt);

}

int getNumeric(char *val, size_t minlength, size_t maxlength, int display, int timeout, const char* title, const char* prompt)
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


int getText(char *val, size_t len, int display, int timeout, const char* title, const char* prompt)
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

int getText(std::string& val, int display, int timeout, const char* title, const char* prompt)
{
    char buff[128 + 1] = { 0 };
    int result = getText(buff, sizeof(buff) - 1, display, timeout, title, prompt);
    
    val.append(buff);

    return result;
}

int UI_ShowOkCancelMessage(int timeout, const char* title, const char* text)
{
    int result = 0;

    gui_clear_dc();
	result = gui_messagebox_show((char *)title, (char *)text, "Cancel", "", timeout);

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

int UI_ShowButtonMessage(int timeout, const char* title, const char* text, const char* button)
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

// int getPassword(std::string& password)
// {

// }


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







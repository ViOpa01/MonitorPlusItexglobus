#ifndef SIMPIO__H
#define SIMPIO__H

#include <vector>
#include <string>


typedef enum {
    UI_DIALOG_TYPE_NONE,
    UI_DIALOG_TYPE_CONFIRMATION,
    UI_DIALOG_TYPE_QUESTION,
    UI_DIALOG_TYPE_WARNING,
    UI_DIALOG_TYPE_CAUTION
} UI_DIALOG_TYPE;

typedef enum InputStatus{
    EMV_CT_PIN_INPUT_TIMEOUT = -3,
    EMV_CT_PIN_INPUT_ABORT = -2,
    EMV_CT_PIN_INPUT_OTHER_ERR = -1,
    EMV_CT_PIN_INPUT_OKAY = 0
} InputStatus;

typedef enum MessageBoxStatus {
    CANCEL = -2,
    TIMEOUT = -1,
    CONFIRM = 0
    
} MessageBoxStatus;


InputStatus getPin(std::string& pin, const char* title, const int minLength = 4);
std::string getNumeric(int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType);
int getNumeric(char *val, size_t len, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType);
int getNumeric(char *val, size_t minlength, size_t maxlength, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType);
int getText(char *val, size_t len, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType);
int getText(std::string& val, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType);
std::string getPhoneNumber(const char* title, const char* prompt, bool uselocale = 1);
int getPassword(std::string& password);

unsigned long getAmount(const char* title);

int UI_ShowOkCancelMessage(int timeout, const char* title, const char* text, UI_DIALOG_TYPE dialogType);
int UI_ShowButtonMessage(int timeout, const char* title, const char* text, const char* button, UI_DIALOG_TYPE type);
int UI_ShowSelection(int timeout, const char* title, const std::vector<std::string>& elements, int preselect);
void Demo_SplashScreen(const char *text, const char *text_additional);

#endif


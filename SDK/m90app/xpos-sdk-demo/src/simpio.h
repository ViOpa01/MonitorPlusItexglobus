#ifndef SIMPIO__H
#define SIMPIO__H

#include <vector>
#include <string>

// #ifdef __cplusplus
// extern "C" {
// #endif

typedef enum InputStatus{
    EMV_CT_PIN_INPUT_TIMEOUT = -3,
    EMV_CT_PIN_INPUT_ABORT = -2,
    EMV_CT_PIN_INPUT_OTHER_ERR = -1,
    EMV_CT_PIN_INPUT_OKAY = 0
} InputStatus;

typedef enum MessageBoxStatus {
    CONFIRM = 1,
    CANCEL = 2,
    TIMEOUT = 3
} MessageBoxStatus;


int getPin(std::string& pin, const char* title, const int minLength = 4);
std::string getNumeric(int display, int timeout, const char* title, const char* prompt);
int getNumeric(char *val, size_t len, int display, int timeout, const char* title, const char* prompt);
int getNumeric(char *val, size_t minlength, size_t maxlength, int display, int timeout, const char* title, const char* prompt);
int getText(char *val, size_t len, int display, int timeout, const char* title, const char* prompt);
int getText(std::string& val, int display, int timeout, const char* title, const char* prompt);
std::string getPhoneNumber(const char* title, const char* prompt, bool uselocale = 1);
// int getPassword(std::string& password);


int UI_ShowOkCancelMessage(int timeout, const char* title, const char* text);
int UI_ShowButtonMessage(int timeout, const char* title, const char* text, const char* button);
int UI_ShowSelection(int timeout, const char* title, const std::vector<std::string>& elements, int preselect);

// #ifdef __cplusplus
// }
// #endif

#endif


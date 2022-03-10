#ifndef USSD_ADMIN
#define USSD_ADMIN

#include <map>
#include <vector>


int ussdAdmin();
int printUSSDReceipt(std::map<std::string, std::string>& record);
int showUssdTransactions(int timeout, const char* title, std::vector<std::map<std::string, std::string> >& elements, int preselect);

#endif

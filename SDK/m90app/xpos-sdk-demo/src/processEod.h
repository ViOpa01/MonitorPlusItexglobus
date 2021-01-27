#ifndef PROCESS_EOD
#define PROCESS_EOD

#include <vector>
#include <map>
#include <string>

struct ProcessEod {

    ProcessEod(const std::vector<std::map<std::string, std::string> > &record, const std::string &db, const std::string &file);
    void generateEodReceipt(const bool isRRN, const char* eodDate);
    
private:

    struct EodLabel {
        char time[6];
        std::string rrnOrPanStr;
        char flag[2];
        char price[13];
    };

    int totalNumberOfApproved;
    int totalNumberOfTransactions;
    int totalNumberOfDeclined;
    float totalSumOfDeclinedInNaira;
    float totalSumOfApprovedInNaira;
    std::string dbName;
    std::string dbFile;
    std::vector<std::map<std::string, std::string> > data;
    std::vector<EodLabel> labelList;

    void generateEodHeader(const char* eodDate);
  
};

#endif

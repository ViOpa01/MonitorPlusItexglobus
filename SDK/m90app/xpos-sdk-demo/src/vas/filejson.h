#ifndef FILEJSON_H
#define FILEJSON_H

#include <string>
#include "jsobject.h"



bool isFileExists(const char* filename);

struct FileJson {

    typedef enum {
        NOERROR,
        SOME_ERROR_OCCURED,
        FILE_NOT_EXIST,
        SAVE_ERR
    } FileJsonErr;

    FileJson(const char* filename, const char* configTemplate = 0);
    int save(const char * = 0);
    int error() const;

    iisys::JSObject object;

protected:
    FileJsonErr err;
    std::string _filename;
private:
    FileJson();
};

#endif

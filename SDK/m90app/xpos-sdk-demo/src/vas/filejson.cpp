#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

#include "simpio.h"
#include "filejson.h"


FileJson::FileJson(const char* filename, const char* configTemplate)
    : err(NOERROR), _filename(filename)
{
    std::ifstream file;
    std::stringstream stream;
    bool fileExists = isFileExists(filename);
    
    if (!fileExists) {
        if (!configTemplate || !isFileExists(configTemplate)) {
            err = FILE_NOT_EXIST;
            return;
        }
        file.open(configTemplate);
    } else {
        file.open(filename);
    }

    if (!file.is_open()) {
        err = SOME_ERROR_OCCURED;
        return;
    }

    stream << file.rdbuf();
    if (!object.load(stream.str().c_str())) {
        err = SOME_ERROR_OCCURED;
        return;
    }
    file.close();

    if (!fileExists) {
        remove(configTemplate);
    }
}

int FileJson::save(const char* name)
{
    FILE* config;
    std::string filename;
    if (name) {
        filename = name;
    } else {
        filename = _filename;
    }

    if ((config = fopen(filename.c_str(), "w+")) == NULL) {
        UI_ShowOkCancelMessage(2000, "WRITE OPEN FAILED", object.dump().c_str(), UI_DIALOG_TYPE_NONE);
        err = SAVE_ERR;
        return -1;
    }
    fprintf(config, "%s", object.dump().c_str());
    fclose(config);
    err = NOERROR;
    return 0;
}

int FileJson::error() const { return err ? 1 : 0; }
///////////////////////////////////////////////////////////

bool isFileExists(const char* filename)
{
    struct stat st;
    int result = stat(filename, &st);  // this approach doesn't work if we don't have read access right to the DIRECTORY CONTAINING THE FILE
    return result == 0;
}




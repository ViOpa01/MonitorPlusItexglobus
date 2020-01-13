#include "itexFile.h"

#include "libapi_xpos/inc/libapi_file.h"

int saveRecord(const void *parameters, const char *filename, const int recordSize, const int flag)
{
    int ret = 0;
    int fp;

    ret = UFile_OpenCreate(filename, FILE_PRIVATE, FILE_CREAT, &fp, flag); //File open / create

    if (ret != UFILE_SUCCESS)
    {
        gui_messagebox_show("FileTest", "File open or create fail", "", "confirm", 0);
        return ret;
    }

    UFile_Write(fp, (char *)parameters, recordSize);
    UFile_Close(fp);

    return UFILE_SUCCESS;
}

int getRecord(void *parameters, const char *filename, const int recordSize, const int flag)
{
    int ret = 0;
    int fp;

    ret = UFile_OpenCreate(filename, FILE_PRIVATE, FILE_RDONLY, &fp, recordSize); //File open / create

    if (ret != UFILE_SUCCESS)
    {
        gui_messagebox_show("FileTest", "File open or create fail", "", "confirm", 0);
        return ret;
    }

    UFile_Lseek(fp, 0, 0); // seek 0
    memset((char *) parameters, 0x00, recordSize);
    UFile_Read(fp, (char *)parameters, recordSize);
    UFile_Close(fp);

    return UFILE_SUCCESS;
}


int removeRecord(const char *filename, const int recordSize)
{
    int ret = 0;
    int fp;

    ret = UFile_OpenCreate(filename, FILE_PRIVATE, FILE_RDONLY, &fp, recordSize); //File open / create

    if (ret != UFILE_SUCCESS)
    {
        gui_messagebox_show("FileTest", "File open or create fail", "", "confirm", 0);
        return ret;
    }

    ret = UFile_Delete(fp, recordSize);

    if (ret != UFILE_SUCCESS) 
    {
         gui_messagebox_show("FileTest", "Unable to delete record", "", "confirm", 0);
         UFile_Close(fp);
        return ret;
    }

    UFile_Close(fp);

    return ret;
}

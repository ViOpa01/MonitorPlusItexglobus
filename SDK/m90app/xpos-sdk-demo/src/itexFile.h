#pragma once



int saveRecord(const void *parameters, const char *filename, const int recordSize, const int flag);
int getRecord(void *parameters, const char *filename, const int recordSize, const int flag);
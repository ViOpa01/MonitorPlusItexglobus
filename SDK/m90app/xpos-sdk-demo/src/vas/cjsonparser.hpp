#include "../cJSON.h"
#include "jsobject.h"

#ifndef CJSONPARSER_H
#define CJSONPARSER_H

// I choose to use cJSON here, it could as well be another capable library for parsing JSON
void build(iisys::JSObject& obj, cJSON* json)
{
    cJSON* element;
    if (cJSON_IsObject(json)) {
        cJSON_ArrayForEach(element, json) {
            if (cJSON_IsObject(element) || cJSON_IsArray(element)) {
                build(obj(element->string), element);
            } else if (cJSON_IsString(element)) {
                obj(element->string)  = element->valuestring;
            } else if (cJSON_IsNumber(element)) {
                obj(element->string) = element->valuedouble;
            } else if (cJSON_IsBool(element)) {
                if (cJSON_IsTrue(element)) {
                    obj(element->string) = true;
                } else {
                    obj(element->string) = false;
                }
            } else if (cJSON_IsNull(element)) {
                obj(element->string) =  (const char*) 0;
            }
        }
    } else if (cJSON_IsArray(json)) {
        size_t i = 0;
        cJSON_ArrayForEach(element, json) {
            if (cJSON_IsObject(element) || cJSON_IsArray(element)) {
                build(obj[i], element);
            } else if (cJSON_IsString(element)) {
                obj[i]  = element->valuestring;
            } else if (cJSON_IsNumber(element)) {
                obj[i] = element->valuedouble;
            } else if (cJSON_IsBool(element)) {
                if (cJSON_IsTrue(element)) {
                    obj[i] = true;
                } else {
                    obj[i] = false;
                }
            } else if (cJSON_IsNull(element)) {
                obj[i] = (const char*) 0;
            }
            
            ++i;
        }
    }

}

bool cjsonparser(iisys::JSObject& obj, const char* jsonTxt)
{
        cJSON* json;

        json = cJSON_Parse(jsonTxt);
        if (!json) {
            return false;
        }

        build(obj, json);

        cJSON_Delete(json);

        return true;
}

#endif

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <limits>
#include <stdexcept>

#ifndef JSOBJECT_H
#define JSOBJECT_H

namespace iisys { 

#define JSONDEBUG

struct JSObject {
    
    template <bool B, class T = void> struct enable_if {};
    template <class T> struct enable_if<true, T> {typedef T type;};

    typedef std::map<std::string, JSObject>::iterator iterator;
    typedef std::map<std::string, JSObject>::const_iterator const_iterator;

    JSObject();

    JSObject& operator()(const char* elem);
    JSObject& operator()(const std::string& elem);
    const JSObject& operator()(const char* elem) const;
    const JSObject& operator()(const std::string& elem) const;

    JSObject& operator[](const size_t i);
    const JSObject& operator[](const size_t i) const;

    JSObject& operator=(const char* val);
    JSObject& operator=(const std::string& val);
    JSObject& operator=(bool val);

    bool isNull() const;
    bool isBool() const;
    bool isArray() const;
    bool isString() const;
    bool isObject() const;
    bool isNumber() const;
    bool isIntegral() const;

    void clear();
    void erase(const char *elem);
    void erase(const size_t index);

    size_t size() const;

    template <class T>
    typename enable_if<std::numeric_limits<T>::is_iec559, JSObject&>::type
    operator=(T val)
    {
        type = NUMBER_FLOAT_T;
        number.float_val = val;
        return *this;
    }

    template <class T>
    typename enable_if<std::numeric_limits<T>::is_integer, JSObject&>::type
    operator=(T val)
    {
        type = NUMBER_INTEGER_T;
        number.int_val = val;
        return *this;
    }

    std::string dump() const;
    std::string getString() const;

    long getInt() const;
    double getNumber() const;

    bool getBool() const;

    bool load(const char*);
    bool load(const std::string& jsonTxt);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    static JSObject createObject();
    static JSObject createArray();

    static void initparser(bool (*custom_parser)(JSObject& obj, const char*));

protected:
    typedef enum {
        NOT_SET,
        NULL_T,
        OBJ_T,
        ARR_T,
        STRING_T,
        BOOL_T,
        NUMBER_FLOAT_T,
        NUMBER_INTEGER_T
    } ValType;

    std::map<std::string, JSObject> mChild;
    std::vector<JSObject> vChild;
    ValType type;

    std::string value;

    #ifdef JSONDEBUG
    std::string key;
    #endif

    bool bool_val;
    union {
        double float_val;
        long int_val;
    } number;

    std::string typeToString(ValType t) const;

};

}

#endif

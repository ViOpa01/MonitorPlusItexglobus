#include <stdlib.h>

#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../cJSON.h"
#include "jsobject.h"
#include "cjsonparser.hpp"

static bool (*parser)(iisys::JSObject& obj, const char*) = cjsonparser;

using namespace iisys;

static const JSObject nulljsobject; 

JSObject::JSObject()
    : type(NOT_SET)
{
}

JSObject& JSObject::operator()(const char* elem)
{
    if (mChild.find(elem) != mChild.end()) {
        return mChild[elem];
    }
    type = OBJ_T;
    JSObject& newInsert = mChild[elem];
    newInsert.type = NULL_T;

    #ifdef JSONDEBUG
    newInsert.key = elem;
    #endif

    return newInsert;
}

JSObject& JSObject::operator()(const std::string& elem)
{
    return (*this)(elem.c_str());
}

const JSObject& JSObject::operator()(const char* elem) const
{
    std::map<std::string, JSObject>::const_iterator it = mChild.find(elem);
    if (it != mChild.end()) {
        return it->second;
    }
    return nulljsobject;
}

const JSObject& JSObject::operator()(const std::string& elem) const
{
    return (*this)(elem.c_str());
}

JSObject& JSObject::operator[](const size_t i)
{
    type = ARR_T;

    if (i < vChild.size()) {
        return vChild[i];
    }

    vChild.push_back(JSObject());
    while (i + 1 > vChild.size()) {
        vChild.back().type = NULL_T;
        vChild.push_back(JSObject());
    }

    vChild[i].type = NOT_SET;

    return vChild[i];
}

const JSObject& JSObject::operator[](const size_t i) const
{
    if (i < vChild.size()) {
        return nulljsobject;
    }
    return vChild[i];
}

JSObject& JSObject::operator=(const char* val)
{
    if (val) {
        type = STRING_T;
        value = val;
    } else {
        type = NULL_T;
    }
    return *this;
}

JSObject& JSObject::operator=(const std::string& val)
{
    return *this = val.c_str();
}

JSObject& JSObject::operator=(bool val)
{
    type = BOOL_T;
    bool_val = val;
    return *this;
}

bool JSObject::isNull() const
{
    return type == NULL_T;
}

bool JSObject::isBool() const
{
    return type == BOOL_T;
}

bool JSObject::isArray() const
{
    return type == ARR_T;
}

bool JSObject::isString() const
{
    return type == STRING_T;
}

bool JSObject::isObject() const
{
    return type == OBJ_T;
}

bool JSObject::isNumber() const
{
    return (type == NUMBER_FLOAT_T || type == NUMBER_INTEGER_T);
}

bool JSObject::isIntegral() const
{
    return type == NUMBER_INTEGER_T;
}

void JSObject::clear()
{
    value.clear();
    mChild.clear();
    vChild.clear();
    type = NULL_T;
}

void JSObject::erase(const char *elem)
{
    mChild.erase(elem);
}

void JSObject::erase(const size_t index)
{
    if (index < vChild.size())
        vChild.erase(vChild.begin() + index);
}

size_t JSObject::size() const
{
    if (type == ARR_T) {
        return vChild.size();
    } else if (type == OBJ_T) {
        return mChild.size();
    }
    return 0;
}

std::string JSObject::dump() const
{
    std::ostringstream ostream;
    if (type == OBJ_T) {
        std::map<std::string, JSObject>::const_iterator it = mChild.begin();
        std::map<std::string, JSObject>::const_iterator end = mChild.end();
        ostream << "{";
        if (it != end) {
            ostream << "\"" << it->first << "\"" << ":" << it->second.dump();
            std::advance(it, 1);
            while (it != end) {
                ostream << ",\"" << it->first << "\"" << ":" << it->second.dump();
                std::advance(it, 1);
            }
        }
        ostream << "}";
    } else if (type == ARR_T) {
        std::vector<JSObject>::const_iterator it = vChild.begin();
        std::vector<JSObject>::const_iterator end = vChild.end();
        ostream << "[";
        if (it != end) {
            ostream << it->dump();
            std::advance(it, 1);
            while (it != end) {
                ostream << "," << it->dump();
                std::advance(it, 1);
            }
        }
        ostream << "]";
    } else if (type == STRING_T) {
        ostream << "\"" << value << "\"";
    } else if (type == NUMBER_FLOAT_T) {
        ostream << number.float_val;
    } else if (type == NUMBER_INTEGER_T) {
        ostream << number.int_val;
    } else if (type == NULL_T) {
        ostream << "null";
    } else if (type == BOOL_T) {
        ostream << std::boolalpha << bool_val;
    } else {
        // Not set
    }
    return ostream.str();
}

std::string JSObject::getString() const
{
    if (type == NULL_T) {
        std::string message;

        #ifdef JSONDEBUG
        message += "(\"" + key + "\"): ";
        #endif
        message += typeToString(type) + " type is not convertible to string";

        throw std::runtime_error(message);
    } else if (type == STRING_T) {
        return value;
    } else {
        return dump();
    }
}

long JSObject::getInt() const
{
    if (type == NUMBER_INTEGER_T) {
        return number.int_val;
    } else if (type == NUMBER_FLOAT_T) {
        return static_cast<long>(number.float_val);
    } else if (type == STRING_T) {
        char *err = 0;
        long val = strtol(value.c_str(), &err, 10);

        if (*err) {
            throw std::runtime_error("String type is not fully convertible to long");
        }

        return val;

    } else {
        std::string message;
        
        #ifdef JSONDEBUG
        message += "(\"" + key + "\"): ";
        #endif
        message += typeToString(type) + " type is not convertible to long";

        throw std::runtime_error(message);
    }
}

double JSObject::getNumber() const
{
    if (type == NUMBER_FLOAT_T) {
        return number.float_val;
    } else if (type == NUMBER_INTEGER_T) {
        return static_cast<double>(number.int_val);
    } else if (type == STRING_T) {
        char *err = 0;
        double val = strtod(value.c_str(), &err);

        if (*err) {
            throw std::runtime_error("String type is not fully convertible to number");
        }

        return val;

    } else {
        std::string message;
        
        #ifdef JSONDEBUG
        message += "(\"" + key + "\"): ";
        #endif
        message += typeToString(type) + " type is not convertible to number";

        throw std::runtime_error(message);
    }
}

bool JSObject::getBool() const
{
    if (type == BOOL_T) {
        return bool_val;
    } else if (type == NUMBER_INTEGER_T) {
        return static_cast<bool>(number.int_val);
    } else {
        std::string message;
        
        #ifdef JSONDEBUG
        message += "(\"" + key + "\"): ";
        #endif
        message += typeToString(type) + " type is not convertible to number";

        throw std::runtime_error(message);
    }
}

bool JSObject::load(const char* jsonTxt)
{
    vChild.clear();
    mChild.clear();
    type = NOT_SET;
    return parser(*this, jsonTxt);
}

bool JSObject::load(const std::string& jsonTxt)
{
    return this->load(jsonTxt.c_str());
}

JSObject::iterator JSObject::begin()
{
    return mChild.begin();
}

JSObject::iterator JSObject::end()
{
    return mChild.end();
}

JSObject::const_iterator JSObject::begin() const
{
    return mChild.begin();
}

JSObject::const_iterator JSObject::end() const
{
    return mChild.end();
}

JSObject JSObject::createObject()
{
    JSObject obj;
    obj.type = OBJ_T;
    return obj;
}

JSObject JSObject::createArray()
{
    JSObject arr;
    arr.type = ARR_T;
    return arr;
}

std::string JSObject::typeToString(ValType t) const
{
    switch (t) {
    case NULL_T:
        return "null";
    case OBJ_T:
        return "object";
    case ARR_T:
        return "array";
    case STRING_T:
        return "string";    
    case BOOL_T:
        return "bool";
    case NUMBER_FLOAT_T:
    case NUMBER_INTEGER_T:
        return "number";
    default:
        return "";
    }

}

void initparser(bool (*custom_parser)(JSObject& obj, const char*))
{
    if (custom_parser) {
        parser = custom_parser;
    }
}

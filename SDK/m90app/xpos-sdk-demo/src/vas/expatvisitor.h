#ifndef VAS_EXPAT_VISITOR_H
#define VAS_EXPAT_VISITOR_H

#include <expat.h>

struct ExpatVisitor {

    ExpatVisitor();
    virtual ~ExpatVisitor();

    virtual void setTag(const char* tag) = 0;
    virtual void endTag(const char* tag) = 0;
    virtual void visit(const char* start, int len) = 0;

    static size_t writeFunction(void* contents, size_t size, size_t nmemb, void* userp);

    int done;
    int parseErr;

private:

    int operator()(void* contents, size_t realsize);

    static void XMLCALL startElement(void* userData, const XML_Char* name, const XML_Char** atts);
    static void XMLCALL endElement(void* userData, const XML_Char* name);
    static void XMLCALL getElement(void* userData, const XML_Char* s, int len);

    ExpatVisitor(const ExpatVisitor&);    // = delete;
    void operator=(const ExpatVisitor&);   // = delete;

    XML_Parser obj;
};

#endif

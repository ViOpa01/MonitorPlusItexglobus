#include "expatvisitor.h"
#include "ctype.h"

size_t ExpatVisitor::writeFunction(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    ExpatVisitor* parser = static_cast<ExpatVisitor*>(userp);
    (*parser)(contents, realsize);
    return realsize;
}

void XMLCALL ExpatVisitor::startElement(void* userData, const XML_Char* name, const XML_Char** atts)
{
    ExpatVisitor* parser = static_cast<ExpatVisitor*>(userData);
    (void)atts;
    parser->setTag(name);
}

void XMLCALL ExpatVisitor::endElement(void* userData, const XML_Char* name)
{
    ExpatVisitor* parser = static_cast<ExpatVisitor*>(userData);
    parser->endTag(name);
}

void XMLCALL ExpatVisitor::getElement(void* userData, const XML_Char* s, int len)
{
    int i = 0;
    ExpatVisitor* parser = static_cast<ExpatVisitor*>(userData);

    while (i < len && isspace(*(s + i)))
        i++;
    if (i == len)
        return;

    parser->visit(s + i, len - i);
}

ExpatVisitor::ExpatVisitor()
    : done(0)
    , parseErr(0)
    , obj(XML_ParserCreate(NULL))
{
    XML_SetUserData(obj, this);
    XML_SetElementHandler(obj, ExpatVisitor::startElement, ExpatVisitor::endElement);
    XML_SetCharacterDataHandler(obj, ExpatVisitor::getElement);
}

ExpatVisitor::~ExpatVisitor()
{
    XML_ParserFree(obj);
}

int ExpatVisitor::operator()(void* contents, size_t realsize)
{
    // xml.append(static_cast<const char*>(contents), realsize);

    if (XML_Parse(obj, static_cast<const char*>(contents), (int)realsize, done) == XML_STATUS_ERROR) {
        done = 1;
        parseErr = 1;
    }
    // LOGF_INFO(log.handle, "(%d - %d) -> %s", realsize, done, xml.c_str());
    return !done ? 0 : -1;
}

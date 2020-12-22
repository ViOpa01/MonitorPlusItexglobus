#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <stdlib.h>
#include <string>

namespace vasimpl {

int bin2hex(unsigned char* pcInBuffer, char* pcOutBuffer, int iLen);

unsigned char* base64_encode(const unsigned char* src, size_t len, size_t* out_len);
int sha512Hex(char* hexoutput, const char* data, const size_t datalen);

int sha512Bin(char* digest, const char* data, const size_t datalen);

int hmac_sha256(
    const unsigned char* text, /* pointer to data stream        */
    int text_len, /* length of data stream         */
    const unsigned char* key, /* pointer to authentication key */
    int key_len, /* length of authentication key  */
    void* digest); /* caller digest to be filled in */


std::string generateRequestAuthorization_v2(const std::string& requestBody);

}

#endif

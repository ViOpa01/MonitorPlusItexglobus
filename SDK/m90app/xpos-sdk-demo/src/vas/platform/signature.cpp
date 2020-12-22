#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/x509.h>

#include <string>

extern "C" {
#include "sha256.h"
}

#include "signature.h"
#include "vasutility.h"

static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define rot13(word)                                                        \
    {                                                                      \
        size_t i = strlen(word);                                           \
        while (i--) {                                                      \
            char c = word[i];                                              \
            if ((c >= 'A' && c <= 'M') || (c >= 'a' && c <= 'm')) {        \
                word[i] += '\r';                                           \
            } else if ((c >= 'N' && c <= 'Z') || (c >= 'n' && c <= 'z')) { \
                word[i] -= '\r';                                           \
            }                                                              \
        }                                                                  \
    }

int vasimpl::bin2hex(unsigned char* pcInBuffer, char* pcOutBuffer, int iLen)
{

    int iCount;
    char* pcBuffer;
    unsigned char* pcTemp;
    unsigned char ucCh;

    memset(pcOutBuffer, 0, sizeof(pcOutBuffer));
    pcTemp = pcInBuffer;
    pcBuffer = pcOutBuffer;
    for (iCount = 0; iCount < iLen; iCount++) {
        ucCh = *pcTemp;
        pcBuffer += sprintf(pcBuffer, "%02X", (int)ucCh);
        pcTemp++;
    }

    return 0;
}

unsigned char* vasimpl::base64_encode(const unsigned char* src, size_t len, size_t* out_len)
{
    unsigned char *out, *pos;
    const unsigned char *end, *in;
    size_t olen;
    int line_len;

    olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen += olen / 72; /* line feeds */
    olen++; /* nul termination */
    if (olen < len)
        return NULL; /* integer overflow */
    out = (unsigned char*)malloc(olen);
    if (out == NULL)
        return NULL;

    end = src + len;
    in = src;
    pos = out;
    line_len = 0;
    while (end - in >= 3) {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
        line_len += 4;
        if (line_len >= 72) {
            //*pos++ = '\n';
            line_len = 0;
        }
    }

    if (end - in) {
        *pos++ = base64_table[in[0] >> 2];
        if (end - in == 1) {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        } else {
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
        line_len += 4;
    }

    if (line_len) {
        //*pos++ = '\n';
    }

    *pos = '\0';
    if (out_len)
        *out_len = pos - out;
    return out;
}

int vasimpl::sha512Hex(char* hexoutput, const char* data, const size_t datalen)
{
    SHA512_CTX ctx;
    char md[SHA512_DIGEST_LENGTH];

    SHA512_Init(&ctx);
    SHA512_Update(&ctx, data, datalen);
    SHA512_Final((unsigned char*)md, &ctx);

    vasimpl::bin2hex((unsigned char*)md, hexoutput, SHA512_DIGEST_LENGTH);

    return 0;
}

int vasimpl::sha512Bin(char* digest, const char* data, const size_t datalen)
{
    SHA512_CTX ctx;

    SHA512_Init(&ctx);
    SHA512_Update(&ctx, data, datalen);
    SHA512_Final((unsigned char*)digest, &ctx);

    return 0;
}

int vasimpl::hmac_sha256(
    const unsigned char* text, /* pointer to data stream        */
    int text_len, /* length of data stream         */
    const unsigned char* key, /* pointer to authentication key */
    int key_len, /* length of authentication key  */
    void* digest) /* caller digest to be filled in */
{
    unsigned char k_ipad[65]; /* inner padding -
                                 * key XORd with ipad
                                 */
    unsigned char k_opad[65]; /* outer padding -
                                 * key XORd with opad
                                 */
    unsigned char tk[SHA256_DIGEST_LENGTH];
    unsigned char tk2[SHA256_DIGEST_LENGTH];
    unsigned char bufferIn[4 * 8192];
    unsigned char bufferOut[4 * 8192];
    int i;

    if (sizeof(bufferIn) < (text_len + 64)) {
        return -1;
    }

    /* if key is longer than 64 bytes reset it to key=sha256(key) */
    if (key_len > 64) {
        SHA256(key, key_len, tk);
        key = tk;
        key_len = SHA256_DIGEST_LENGTH;
    }

    /*
     * the HMAC_SHA256 transform looks like:
     *
     * SHA256(K XOR opad, SHA256(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof k_ipad);
    memset(k_opad, 0, sizeof k_opad);
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /*
     * perform inner SHA256
     */
    memset(bufferIn, 0x00, sizeof(bufferIn));
    memcpy(bufferIn, k_ipad, 64);
    memcpy(bufferIn + 64, text, text_len);

    SHA256(bufferIn, 64 + text_len, tk2);

    /*
     * perform outer SHA256
     */
    memset(bufferOut, 0x00, sizeof(bufferOut));
    memcpy(bufferOut, k_opad, 64);
    memcpy(bufferOut + 64, tk2, SHA256_DIGEST_LENGTH);

    SHA256(bufferOut, 64 + SHA256_DIGEST_LENGTH, (unsigned char*)digest);

    return 0;
}

std::string vasimpl::generateRequestAuthorization_v2(const std::string& requestBody)
{
    char signature[SHA256_DIGEST_LENGTH] = { 0 };
    char signaturehex[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };
    size_t i;
    // const char *key = "n6D6nbUURFbafb27kNxmbODqLSge9pXP";   // test
    const char *key = "b83cef088a4943231342c7fd53b6502d";   // live

    vasimpl::hmac_sha256((const unsigned char*)requestBody.c_str(), requestBody.length(), (const unsigned char*)key, (int)strlen(key), signature);


    vasimpl::bin2hex((unsigned char*)signature, signaturehex, sizeof(signature));
    i = strlen(signaturehex);
    while (i--) {
        signaturehex[i] = tolower(signaturehex[i]);
    }

    return std::string(signaturehex);
}


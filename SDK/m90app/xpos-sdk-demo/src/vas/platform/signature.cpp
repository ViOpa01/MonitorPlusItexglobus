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

unsigned char* vasimpl::base64_decode(const unsigned char* src, size_t len, size_t* out_len)
{
    unsigned char dtable[256], *out, *pos, block[4], tmp;
    size_t i, count, olen;
    int pad = 0;

    memset(dtable, 0x80, 256);
    for (i = 0; i < sizeof(base64_table) - 1; i++)
        dtable[base64_table[i]] = (unsigned char)i;
    dtable['='] = 0;

    count = 0;
    for (i = 0; i < len; i++) {
        if (dtable[src[i]] != 0x80)
            count++;
    }

    if (count == 0 || count % 4)
        return NULL;

    olen = count / 4 * 3;
    pos = out = (unsigned char*) malloc(olen);
    if (out == NULL)
        return NULL;

    count = 0;
    for (i = 0; i < len; i++) {
        tmp = dtable[src[i]];
        if (tmp == 0x80)
            continue;

        if (src[i] == '=')
            pad++;
        block[count] = tmp;
        count++;
        if (count == 4) {
            *pos++ = (block[0] << 2) | (block[1] >> 4);
            *pos++ = (block[1] << 4) | (block[2] >> 2);
            *pos++ = (block[2] << 6) | block[3];
            count = 0;
            if (pad) {
                if (pad == 1)
                    pos--;
                else if (pad == 2)
                    pos -= 2;
                else {
                    /* Invalid padding */
                    free(out);
                    return NULL;
                }
                break;
            }
        }
    }

    *out_len = pos - out;
    return out;
}

std::string vasimpl::base64_decode(const unsigned char* src, size_t len)
{
    size_t outlen;
    unsigned char* data = vasimpl::base64_decode(src, len, &outlen);
    std::string ret((const char*)data, outlen);
    free(data);

    return ret;
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

static std::string decrypt(const unsigned char* ciphertext, const int ciphertext_len, const unsigned char* key, const unsigned char* iv, const EVP_CIPHER* cipher)
{
    EVP_CIPHER_CTX* ctx;
    int len;
    int plaintext_len = 0;
    std::string ret;
    unsigned char* plaintext = new unsigned char[ciphertext_len];

    if (!plaintext) {
        return ret;
    }

    memset(plaintext, 0, ciphertext_len);

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        delete[] plaintext;
        return ret;
    }

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
    if (1 != EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv)) {
        goto cleanup;
    }

    EVP_CIPHER_CTX_set_key_length(ctx, EVP_MAX_KEY_LENGTH);

    /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        goto cleanup;
    }

    plaintext_len = len;

    /* Finalize the decryption. Further plaintext bytes may be written at
   * this stage.
   */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        goto cleanup;
    }

    plaintext_len += len;

    /* Add the null terminator */
    plaintext[plaintext_len] = 0;
    ret = (char*)plaintext;

    /* Clean up */
cleanup:
    EVP_CIPHER_CTX_free(ctx);
    delete[] plaintext;
    return ret;
}

std::string vasimpl::cbc128_decrypt(const unsigned char* ciphertext, const int ciphertext_len, const unsigned char* key, const unsigned char* iv)
{
    return decrypt(ciphertext, ciphertext_len, key, iv, EVP_aes_128_cbc());
    return "";
}
/**
 * File: Nibss8583SelfTest.h
 * --------------------------
 * Defines new interface for testing Nibss8583.
 * @author Opeyemi Ajani Sunday.
 */

#ifndef _NIBSS_ISO8583_SELF_TEST
#define _NIBSS_ISO8583_SELF_TEST

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#ifdef DLL_EXPORTS
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT __declspec(dllimport)
#endif
#else
#define LIB_EXPORT
#endif 


LIB_EXPORT void nibss8583SelfTest(void);

#ifdef __cplusplus
}
#endif

#endif 

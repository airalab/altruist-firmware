#ifndef TESTS_MESHTASTIC_MBEDTLS_MD_H
#define TESTS_MESHTASTIC_MBEDTLS_MD_H

/*
 * Host-only stand-in for mbedtls/md.h so meshtastic_frame.cpp can compile
 * without the ESP mbedtls package.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MBEDTLS_MD_SHA256 0

typedef int mbedtls_md_type_t;
typedef struct mbedtls_md_info_t mbedtls_md_info_t;

const mbedtls_md_info_t *mbedtls_md_info_from_type(mbedtls_md_type_t type);
int mbedtls_md(const mbedtls_md_info_t *info, const unsigned char *input, size_t ilen, unsigned char output[32]);

#ifdef __cplusplus
}
#endif

#endif

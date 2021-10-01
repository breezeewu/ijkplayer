/*
 * Copyright 2010-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include "internal/cryptlib.h"
#include <openssl/evp.h>
#include "crypto/asn1.h"

/*
 * CMAC "ASN1" method. This is just here to indicate the maximum CMAC output
 * length and to free up a CMAC key.
 */

static int cmac_size(const EVP_PKEY *pkey)
{
    return EVP_MAX_BLOCK_LENGTH;
}

static void cmac_key_free(EVP_PKEY *pkey)
{
    EVP_MAC_CTX *cmctx = EVP_PKEY_get0(pkey);
    EVP_MAC *mac = cmctx == NULL ? NULL : EVP_MAC_CTX_mac(cmctx);

    EVP_MAC_CTX_free(cmctx);
    EVP_MAC_free(mac);
}

const EVP_PKEY_ASN1_METHOD cmac_asn1_meth = {
    EVP_PKEY_CMAC,
    EVP_PKEY_CMAC,
    0,

    "CMAC",
    "OpenSSL CMAC method",

    0, 0, 0, 0,

    0, 0, 0,

    cmac_size,
    0, 0,
    0, 0, 0, 0, 0, 0, 0,

    cmac_key_free,
    0,
    0, 0
};

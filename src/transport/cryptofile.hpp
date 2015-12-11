/*
 * Copyright (c) 2013 WALLIX, SARL. All rights reserved.
 * Licensed computer software. Property of WALLIX.
 * Product Name : Wallix Admin Bastion
 * Author(s)    : Tristan de Cacqueray <tdc@wallix.com>
 * Id           : $Id$
 * URL          : $URL$
 */

#ifndef WABCRYPTOFILE_HPP
#define WABCRYPTOFILE_HPP

#include "cryptofile.h"

struct CryptoContext {
protected:
    unsigned char hmac_key[HMAC_KEY_LENGTH];
    unsigned char crypto_key[CRYPTO_KEY_LENGTH];

    bool crypto_key_initialize = false;

public:
    CryptoContext(void const * crypto_key, std::size_t crypto_key_length) {
        //LOG(LOG_INFO,
        //    "crypto_key_length=%d, sizeof(this->hmac_key)=%d sizeof(this->crypto_key)=%d",
        //    int(crypto_key_length), int(sizeof(this->hmac_key)),
        //    int(sizeof(this->crypto_key)));

        REDASSERT(crypto_key_length == sizeof(this->hmac_key));
        REDASSERT(crypto_key_length == sizeof(this->crypto_key));

        memset(this->hmac_key, 0, sizeof(this->hmac_key));
        memset(this->crypto_key, 0, sizeof(this->crypto_key));

        memcpy(this->hmac_key, crypto_key, sizeof(this->hmac_key));
        memcpy(this->crypto_key, crypto_key, sizeof(this->crypto_key));

        this->crypto_key_initialize = true;
    }

    CryptoContext() {
        memset(this->hmac_key, 0, sizeof(this->hmac_key));
        memset(this->crypto_key, 0, sizeof(this->crypto_key));
    }

    void get_hmac_key(unsigned char (&target_hmac_key)[HMAC_KEY_LENGTH]) const {
        REDASSERT(this->crypto_key_initialize);
        memcpy(target_hmac_key, this->hmac_key, sizeof(target_hmac_key));
    }

    unsigned char (&get_hmac_key())[HMAC_KEY_LENGTH] {
        REDASSERT(this->crypto_key_initialize);
        return this->hmac_key;
    }

    void get_crypto_key(unsigned char (&target_crypto_key)[CRYPTO_KEY_LENGTH]) const {
        REDASSERT(this->crypto_key_initialize);
        memcpy(target_crypto_key, this->crypto_key, sizeof(target_crypto_key));
    }

    unsigned char (&get_crypto_key())[CRYPTO_KEY_LENGTH] {
        REDASSERT(this->crypto_key_initialize);
        return this->crypto_key;
    }

    bool derive_crypto_key(unsigned char (&target_crypto_key)[CRYPTO_KEY_LENGTH],
            char const * filename, unsigned int version) const {
        REDASSERT(this->crypto_key_initialize);

        unsigned char derivator[DERIVATOR_LENGTH];
        get_derivator(filename, derivator, DERIVATOR_LENGTH);

        return (-1 < compute_hmac(target_crypto_key, this->crypto_key, derivator));
    }
};

#endif  // #define WABCRYPTOFILE_HPP
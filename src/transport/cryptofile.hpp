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
    
     void (* initialize_crypto_key)();

    bool crypto_key_initialize;

public:
    CryptoContext(void const * crypto_key, std::size_t crypto_key_length) 
       : initialize_crypto_key(nullptr),
         crypto_key_initialize(true)
        {
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

    CryptoContext() 
       : initialize_crypto_key(nullptr), 
        crypto_key_initialize(false)    
    {
        memset(this->hmac_key, 0, sizeof(this->hmac_key));
        memset(this->crypto_key, 0, sizeof(this->crypto_key));
    }

    CryptoContext(void (* initialize_crypto_key)()) 
       : initialize_crypto_key(initialize_crypto_key), 
        crypto_key_initialize(false)    
    {
        memset(this->hmac_key, 0, sizeof(this->hmac_key));
        memset(this->crypto_key, 0, sizeof(this->crypto_key));
    }

    void get_hmac_key(unsigned char (&target_hmac_key)[HMAC_KEY_LENGTH]) const {
        if (!this->crypto_key_initialize 
            && this->initialize_crypto_key){
            this->initialize_crypto_key();
        }
        memcpy(target_hmac_key, this->hmac_key, sizeof(target_hmac_key));
    }

    unsigned char (&get_hmac_key())[HMAC_KEY_LENGTH] {
        if (!this->crypto_key_initialize 
            && this->initialize_crypto_key){
            this->initialize_crypto_key();
        }
        return this->hmac_key;
    }

    void get_crypto_key(unsigned char (&target_crypto_key)[CRYPTO_KEY_LENGTH]) const {
        if (!this->crypto_key_initialize 
            && this->initialize_crypto_key){
            this->initialize_crypto_key();
        }
        memcpy(target_crypto_key, this->crypto_key, sizeof(target_crypto_key));
    }

    unsigned char (&get_crypto_key())[CRYPTO_KEY_LENGTH] {
        if (!this->crypto_key_initialize 
            && this->initialize_crypto_key){
            this->initialize_crypto_key();
        }
        return this->crypto_key;
    }

    bool get_target_crypto_key(
        unsigned char (&target_crypto_key)[CRYPTO_KEY_LENGTH], 
        char const * filename)
    {
        unsigned char tmp_derivation[DERIVATOR_LENGTH + CRYPTO_KEY_LENGTH] = {}; // derivator + masterkey
        get_derivator(filename, tmp_derivation, DERIVATOR_LENGTH);
        memcpy(tmp_derivation + DERIVATOR_LENGTH, this->crypto_key, CRYPTO_KEY_LENGTH);
        
        unsigned char derivated[SHA256_DIGEST_LENGTH  + CRYPTO_KEY_LENGTH] = {}; // really should be MAX, but + will do

        if (SHA256(tmp_derivation, CRYPTO_KEY_LENGTH + DERIVATOR_LENGTH, derivated) == nullptr){
            std::printf("[CRYPTO_ERROR][%d]: Could not derivate hash crypto key, SHA256!\n", getpid());
            return false;
        }
        memcpy(target_crypto_key, derivated, HMAC_KEY_LENGTH);
        return true;
            
    }

    bool derive_crypto_key(unsigned char (&target_crypto_key)[CRYPTO_KEY_LENGTH],
            char const * filename, unsigned int version) {
        return this->get_target_crypto_key(target_crypto_key, filename);
    }
};

#endif  // #define WABCRYPTOFILE_HPP

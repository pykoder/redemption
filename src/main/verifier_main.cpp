/*
   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Raphael Zhou

   redver video verifier program
*/

#include "apps/app_verifier.hpp"

#include "config.hpp"
#include "version.hpp"

int verifier_main(int argc, char ** argv) {
    return app_verifier(
        argc, argv
      , "ReDemPtion VERifier " VERSION ".\n"
        "Copyright (C) Wallix 2010-2015.\n"
        "Christophe Grosjean, Raphael Zhou."
/*
      , [](CryptoContext & cctx) {
            cfg::crypto::key0 key0;
            cfg::crypto::key1 key1;
            memcpy(cctx.crypto_key, key0.value, sizeof(cctx.crypto_key));
            memcpy(cctx.hmac_key,   key1.value, sizeof(cctx.hmac_key  ));
            return 0;
        }
*/
    );
}

// emulationstation-master/es-core/src/utils/Crypto.h
#ifndef ES_CORE_UTILS_CRYPTO_H 
#define ES_CORE_UTILS_CRYPTO_H

#include <string>
#include <vector>
// Includi picosha2.h qui se usi picosha2::k_digest_size nella dichiarazione,
// altrimenti è sufficiente in Crypto.cpp
// #include "picosha2.h" 

namespace Utils {
namespace Crypto {
    // La tua funzione esistente
    std::vector<unsigned char> sha256_bytes(const std::string& input);

    // NUOVA FUNZIONE: HMAC-SHA256
    // key: la chiave segreta
    // data: i dati da autenticare
    // output_digest: buffer preallocato di almeno 32 byte (picosha2::k_digest_size) dove verrà scritto l'HMAC
    void HMAC_SHA256(const unsigned char* key, size_t key_len,
                     const unsigned char* data, size_t data_len,
                     unsigned char* output_digest); // output_digest DEVE essere picosha2::k_digest_size byte

    // Helper che prende std::string e restituisce std::vector<unsigned char>
    std::vector<unsigned char> hmac_sha256_bytes(const std::string& key, const std::string& data);

} // namespace Crypto
} // namespace Utils

#endif // ES_CORE_UTILS_CRYPTO_H
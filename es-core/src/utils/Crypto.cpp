// emulationstation-master/es-core/src/utils/Crypto.cpp
#include "utils/Crypto.h" 
#include "picosha2.h"    // Assicurati che sia incluso

#include <algorithm> // Per std::fill, std::copy
#include <vector>    

namespace Utils {
namespace Crypto {

// La tua funzione sha256_bytes esistente
std::vector<unsigned char> sha256_bytes(const std::string& input) {
    std::vector<unsigned char> hash_output(picosha2::k_digest_size);
    picosha2::hash256(input.begin(), input.end(), hash_output.begin(), hash_output.end());
    return hash_output;
}

// NUOVA IMPLEMENTAZIONE: HMAC_SHA256
void HMAC_SHA256(const unsigned char* key, size_t key_len,
                     const unsigned char* data, size_t data_len,
                     unsigned char* output_digest) // output_digest deve essere preallocato (picosha2::k_digest_size byte)
{
    const size_t block_size = 64; // Dimensione del blocco per SHA256

    std::vector<unsigned char> processed_key(block_size, 0x00); // K'

    if (key_len > block_size) {
        // Se la chiave è più lunga di B, K' = H(K)
        // picosha2::hash256 prende iteratori o container. Dobbiamo creare un container per la chiave.
        std::vector<unsigned char> key_vec(key, key + key_len);
        std::vector<unsigned char> hashed_key(picosha2::k_digest_size);
        picosha2::hash256(key_vec.begin(), key_vec.end(), hashed_key.begin(), hashed_key.end());
        std::copy(hashed_key.begin(), hashed_key.end(), processed_key.begin());
        // Il resto di processed_key (da hashed_key.size() a block_size) rimane 0x00
    } else {
        std::copy(key, key + key_len, processed_key.begin());
        // Il resto di processed_key (da key_len a block_size) rimane 0x00 se key_len < block_size
    }

    std::vector<unsigned char> o_key_pad(block_size);
    std::vector<unsigned char> i_key_pad(block_size);

    for (size_t i = 0; i < block_size; ++i) {
        o_key_pad[i] = processed_key[i] ^ 0x5C;
        i_key_pad[i] = processed_key[i] ^ 0x36;
    }

    // Calcola H((K' XOR ipad) || text)
    std::vector<unsigned char> inner_payload;
    inner_payload.reserve(i_key_pad.size() + data_len);
    inner_payload.insert(inner_payload.end(), i_key_pad.begin(), i_key_pad.end());
    inner_payload.insert(inner_payload.end(), data, data + data_len);
    
    std::vector<unsigned char> inner_hash_result(picosha2::k_digest_size);
    picosha2::hash256(inner_payload.begin(), inner_payload.end(), inner_hash_result.begin(), inner_hash_result.end());

    // Calcola H((K' XOR opad) || H_inner)
    std::vector<unsigned char> outer_payload;
    outer_payload.reserve(o_key_pad.size() + inner_hash_result.size());
    outer_payload.insert(outer_payload.end(), o_key_pad.begin(), o_key_pad.end());
    outer_payload.insert(outer_payload.end(), inner_hash_result.begin(), inner_hash_result.end());

    // Il risultato finale viene scritto in output_digest
    // picosha2::hash256(outer_payload.begin(), outer_payload.end(), output_digest, output_digest + picosha2::k_digest_size);
    // La riga sopra è corretta se output_digest è un iteratore. Se è un puntatore a un buffer, va bene.
    // Assicuriamoci che la chiamata sia corretta per un puntatore a buffer:
    std::vector<unsigned char> final_digest_temp(picosha2::k_digest_size);
    picosha2::hash256(outer_payload.begin(), outer_payload.end(), final_digest_temp.begin(), final_digest_temp.end());
    std::copy(final_digest_temp.begin(), final_digest_temp.end(), output_digest);
}

// Implementazione dell'helper (opzionale ma comodo)
std::vector<unsigned char> hmac_sha256_bytes(const std::string& key, const std::string& data) {
    std::vector<unsigned char> digest(picosha2::k_digest_size);
    HMAC_SHA256(
        reinterpret_cast<const unsigned char*>(key.data()), key.length(),
        reinterpret_cast<const unsigned char*>(data.data()), data.length(),
        digest.data() // Passa il puntatore ai dati del vettore
    );
    return digest;
}

} // namespace Crypto
} // namespace Utils
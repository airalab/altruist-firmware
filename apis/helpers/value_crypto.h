#ifndef __VALUE_CRYPTO_H__
#define __VALUE_CRYPTO_H__

#include <Arduino.h>

/**
 * Prefix for encrypted values in CSV/datalog.
 * Connectivity sees "e...." and does not call float() — it forwards the string as-is.
 * After base64 decoding, the payload is CPS JSON.
 */
constexpr const char *VALUE_CRYPTO_CPS_PREFIX = "e.";

/**
 * Encrypt a single value for the owner (Robonomics CPS / libcps scheme):
 * ECDH → HKDF(salt=robonomics-network, info=aesgcm256) → AES-256-GCM → e.<base64(json)>.
 * The "from" field is the sender SS58 address (prefix 32), not a raw base58 pubkey.
 *
 * @param plain numeric text, e.g. "850"
 * @param sender_sk_hex device private key (64 hex chars from cfg::private_key)
 * @param receiver_ss58 owner address (cfg::rws_owner); empty/Not Set = encrypt to self
 * @return e.<...> or empty string on error
 */
String valueCryptoEncryptCpsForOwner(const String &plain, const char *sender_sk_hex,
				     const char *receiver_ss58);

/**
 * Encrypt using device config (private_key + rws_owner).
 * On error, returns an empty string. Callers must abort the send rather than
 * fall back to plaintext.
 */
String valueCryptoEncryptValue(const String &plain);

constexpr size_t VALUE_CRYPTO_KEY_LEN = 32;
constexpr size_t VALUE_CRYPTO_GCM_NONCE_LEN = 12;

bool valueCryptoParseHex32(const char *hex, uint8_t out[VALUE_CRYPTO_KEY_LEN]);
bool valueCryptoDeviceKeys(uint8_t secret_key[VALUE_CRYPTO_KEY_LEN], uint8_t public_key[VALUE_CRYPTO_KEY_LEN]);
bool valueCryptoOwnerPublicKey(const uint8_t sender_pk[VALUE_CRYPTO_KEY_LEN],
			       uint8_t owner_pk[VALUE_CRYPTO_KEY_LEN]);

/**
 * AES-256-GCM for proto `crypto.v1.Encrypted`.
 * Ciphertext is AEAD payload with the 16-byte GCM tag appended (same as CPS).
 * On success `cipher_out` is malloc'd; caller must free().
 */
bool valueCryptoEncryptBytesForOwner(const uint8_t *plain, size_t plain_len, uint8_t from_pk[VALUE_CRYPTO_KEY_LEN],
				     uint8_t nonce[VALUE_CRYPTO_GCM_NONCE_LEN], uint8_t **cipher_out,
				     size_t *cipher_len);

/**
 * Self-test: ECDH + encrypt vectors to Serial.
 * Reference vectors: libcps_TEST_VECTORS.md.
 */
bool valueCryptoSelfTest();

#endif // __VALUE_CRYPTO_H__

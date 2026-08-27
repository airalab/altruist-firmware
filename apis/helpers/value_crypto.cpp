#include "value_crypto.h"

#include "../../config_manager/config_defaults.h"
#include "../../utils.h"

#include <esp_random.h>
#include <mbedtls/base64.h>
#include <mbedtls/bignum.h>
#include <mbedtls/gcm.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <mbedtls/sha512.h>
#include <string.h>

#include <Curve25519.h>
#include <Ed25519.h>
#include <address.h>

/*
 * Metric encryption aligned with Robonomics CPS / libcps, in short:
 *
 * 1) Device and owner derive a shared secret (ECDH):
 *      device private + owner public
 * 2) Derive an AES key from the secret (HKDF).
 * 3) Encrypt the number with AES-256-GCM.
 * 4) On the wire: e.<base64(JSON)>.
 *
 * sensors.map decrypts the same way, but reversed:
 *      owner private + device public (the "from" field in JSON).
 */

namespace cps {

constexpr size_t GCM_NONCE_LEN = 12;  // random nonce per encryption (IV)
constexpr size_t GCM_TAG_LEN = 16;    // authentication tag (tamper detection)
constexpr size_t KEY_LEN = 32;        // 32 bytes = 256 bits (ed25519 seed / AES-256 key)

/*
 * HKDF — as in libcps cipher.rs:
 *   const HKDF_SALT = b"robonomics-network";
 *   Hkdf::new(Some(HKDF_SALT), &shared_secret);  // salt, IKM
 *   hkdf.expand(algorithm.info_string(), ...);   // info = "aesgcm256"
 * mbedtls_hkdf(md, salt, salt_len, ikm, ikm_len, info, info_len, okm, okm_len)
 * — same order: salt → IKM(shared) → info → OKM(AES key).
 */
constexpr const char *HKDF_SALT = "robonomics-network";
constexpr const char *HKDF_INFO_AESGCM256 = "aesgcm256";

/** Bytes → standard base64 string (for nonce/ciphertext/full JSON). */
String base64Encode(const uint8_t *data, size_t len) {
	size_t outLen = 0;
	mbedtls_base64_encode(nullptr, 0, &outLen, data, len);
	uint8_t *buffer = static_cast<uint8_t *>(malloc(outLen + 1));
	if (!buffer) {
		return String();
	}
	if (mbedtls_base64_encode(buffer, outLen + 1, &outLen, data, len) != 0) {
		free(buffer);
		return String();
	}
	buffer[outLen] = '\0';
	String out = reinterpret_cast<char *>(buffer);
	free(buffer);
	return out;
}

/** 64-char hex string (private_key in config) → 32 bytes. */
bool parseHex32(const char *hex, uint8_t out[KEY_LEN]) {
	if (!hex) {
		return false;
	}
	size_t n = strlen(hex);
	if (n >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
		hex += 2;
		n -= 2;
	}
	if (n != KEY_LEN * 2) {
		return false;
	}
	for (size_t i = 0; i < KEY_LEN; i++) {
		char a = hex[i * 2];
		char b = hex[i * 2 + 1];
		auto nib = [](char c) -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		const int hi = nib(a);
		const int lo = nib(b);
		if (hi < 0 || lo < 0) {
			return false;
		}
		out[i] = static_cast<uint8_t>((hi << 4) | lo);
	}
	return true;
}

/**
 * Device public key in JSON "from" — SS58 (prefix 32), not raw base58.
 * SS58 = network prefix + pubkey + blake2b checksum, then base58
 */
String encodeSenderSs58(const uint8_t sender_sk[KEY_LEN]) {
	char *addr = getAddrFromPrivateKey(const_cast<uint8_t *>(sender_sk), ROBONOMICS_PREFIX);
	if (!addr) {
		return String();
	}
	String out(addr);
	delete[] addr;
	return out;
}

static String bytesToHex(const uint8_t *data, size_t len) {
	String out;
	out.reserve(len * 2);
	for (size_t i = 0; i < len; i++) {
		if (data[i] < 0x10) {
			out += '0';
		}
		out += String(data[i], HEX);
	}
	return out;
}

// mbedtls bignum expects big-endian; our keys are little-endian.
bool mpiFromLe32(mbedtls_mpi *X, const uint8_t le[32]) {
	uint8_t be[32];
	for (int i = 0; i < 32; i++) {
		be[i] = le[31 - i];
	}
	return mbedtls_mpi_read_binary(X, be, 32) == 0;
}

bool mpiToLe32(const mbedtls_mpi *X, uint8_t le[32]) {
	uint8_t be[32] = {0};
	if (mbedtls_mpi_write_binary(X, be, 32) != 0) {
		return false;
	}
	for (int i = 0; i < 32; i++) {
		le[i] = be[31 - i];
	}
	return true;
}

/**
 * Ed25519 public key → X25519 key (for ECDH).
 */
bool ed25519PublicToX25519(const uint8_t ed_pk[32], uint8_t x_pk[32]) {
	uint8_t y_le[32];
	memcpy(y_le, ed_pk, 32);
	y_le[31] &= 0x7f;  // clear sign bit from compressed ed25519 key

	mbedtls_mpi y, one, num, den, inv, u, p;
	mbedtls_mpi_init(&y);
	mbedtls_mpi_init(&one);
	mbedtls_mpi_init(&num);
	mbedtls_mpi_init(&den);
	mbedtls_mpi_init(&inv);
	mbedtls_mpi_init(&u);
	mbedtls_mpi_init(&p);

	bool ok = false;
	do {
		// p = 2^255 - 19 (curve prime)
		if (mbedtls_mpi_lset(&one, 1) != 0) break;
		if (mbedtls_mpi_lset(&p, 1) != 0) break;
		if (mbedtls_mpi_shift_l(&p, 255) != 0) break;
		if (mbedtls_mpi_sub_int(&p, &p, 19) != 0) break;
		if (!mpiFromLe32(&y, y_le)) break;

		// u = (1+y) / (1-y)  mod p   ← standard Montgomery u-coordinate map
		if (mbedtls_mpi_add_mpi(&num, &one, &y) != 0) break;
		if (mbedtls_mpi_sub_mpi(&den, &one, &y) != 0) break;
		if (mbedtls_mpi_mod_mpi(&num, &num, &p) != 0) break;
		if (mbedtls_mpi_mod_mpi(&den, &den, &p) != 0) break;
		if (mbedtls_mpi_inv_mod(&inv, &den, &p) != 0) break;
		if (mbedtls_mpi_mul_mpi(&u, &num, &inv) != 0) break;
		if (mbedtls_mpi_mod_mpi(&u, &u, &p) != 0) break;
		if (!mpiToLe32(&u, x_pk)) break;
		ok = true;
	} while (false);

	mbedtls_mpi_free(&y);
	mbedtls_mpi_free(&one);
	mbedtls_mpi_free(&num);
	mbedtls_mpi_free(&den);
	mbedtls_mpi_free(&inv);
	mbedtls_mpi_free(&u);
	mbedtls_mpi_free(&p);
	return ok;
}

/**
 * Shared secret = ECDH(our private, peer public).
 *
 * Steps as in libcps:
 * 1) derive proper X25519 scalar from ed25519 seed (SHA-512 + bit clamp)
 * 2) peer ed25519 public → X25519
 * 3) Curve25519 scalarmult → 32-byte shared secret
 */
bool deriveSharedSecretEd25519(const uint8_t sender_sk[32], const uint8_t receiver_ed_pk[32],
			       uint8_t shared_out[32]) {
	uint8_t hash[64];
	if (mbedtls_sha512(sender_sk, 32, hash, 0) != 0) {
		return false;
	}
	uint8_t scalar[32];
	memcpy(scalar, hash, 32);
	// clamp — required for X25519; omitting it breaks the curve
	scalar[0] &= 248;
	scalar[31] &= 127;
	scalar[31] |= 64;

	uint8_t their_x[32];
	if (!ed25519PublicToX25519(receiver_ed_pk, their_x)) {
		return false;
	}

	if (!Curve25519::eval(shared_out, scalar, their_x)) {
		return false;
	}
	return true;
}

/**
 * HKDF-SHA256 → 32-byte AES key (libcps order: salt, IKM=shared, info).
 */
bool hkdfAesGcmKey(const uint8_t shared[32], uint8_t out_key[32]) {
	const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!md) {
		return false;
	}
	return mbedtls_hkdf(md, reinterpret_cast<const unsigned char *>(HKDF_SALT), strlen(HKDF_SALT),
			    shared, 32, reinterpret_cast<const unsigned char *>(HKDF_INFO_AESGCM256),
			    strlen(HKDF_INFO_AESGCM256), out_key, 32) == 0;
}

/**
 * AES-256-GCM: ciphertext || tag.
 * If fixed_nonce == nullptr — generate a random nonce in nonce[].
 * If fixed_nonce is set — copy it (for test vectors).
 */
bool aesGcmEncrypt(const uint8_t key[32], const uint8_t *plain, size_t plain_len, uint8_t nonce[GCM_NONCE_LEN],
		   uint8_t **cipher_out, size_t *cipher_len, const uint8_t *fixed_nonce = nullptr) {
	const size_t out_len = plain_len + GCM_TAG_LEN;
	uint8_t *out = static_cast<uint8_t *>(malloc(out_len));
	if (!out) {
		return false;
	}
	if (fixed_nonce) {
		memcpy(nonce, fixed_nonce, GCM_NONCE_LEN);
	} else {
		for (size_t i = 0; i < GCM_NONCE_LEN; i += 4) {
			const uint32_t r = esp_random();
			nonce[i + 0] = static_cast<uint8_t>(r & 0xFF);
			nonce[i + 1] = static_cast<uint8_t>((r >> 8) & 0xFF);
			nonce[i + 2] = static_cast<uint8_t>((r >> 16) & 0xFF);
			nonce[i + 3] = static_cast<uint8_t>((r >> 24) & 0xFF);
		}
	}

	mbedtls_gcm_context gcm;
	mbedtls_gcm_init(&gcm);
	int rc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256);
	if (rc == 0) {
		rc = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plain_len, nonce, GCM_NONCE_LEN, nullptr, 0,
					       plain, out, GCM_TAG_LEN, out + plain_len);
	}
	mbedtls_gcm_free(&gcm);
	if (rc != 0) {
		free(out);
		return false;
	}
	*cipher_out = out;
	*cipher_len = out_len;
	return true;
}

bool aesGcmDecrypt(const uint8_t key[32], const uint8_t *cipher, size_t cipher_len, const uint8_t nonce[GCM_NONCE_LEN],
		   uint8_t **plain_out, size_t *plain_len) {
	if (!cipher || cipher_len <= GCM_TAG_LEN) {
		return false;
	}
	const size_t ct_len = cipher_len - GCM_TAG_LEN;
	uint8_t *out = static_cast<uint8_t *>(malloc(ct_len + 1));
	if (!out) {
		return false;
	}
	mbedtls_gcm_context gcm;
	mbedtls_gcm_init(&gcm);
	int rc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256);
	if (rc == 0) {
		rc = mbedtls_gcm_auth_decrypt(&gcm, ct_len, nonce, GCM_NONCE_LEN, nullptr, 0, cipher + ct_len, GCM_TAG_LEN,
					     cipher, out);
	}
	mbedtls_gcm_free(&gcm);
	if (rc != 0) {
		free(out);
		return false;
	}
	out[ct_len] = '\0';
	*plain_out = out;
	*plain_len = ct_len;
	return true;
}

/**
 * Who do we encrypt for?
 * - if rws_owner is set → public key of that address
 * - if owner is empty / Not Set → self-owner: use our own public key
 */
bool resolveReceiverPublic(const char *receiver_ss58, const uint8_t sender_pk[32], uint8_t receiver_pk[32]) {
	const bool missing = !receiver_ss58 || receiver_ss58[0] == '\0' ||
			     strcasecmp(receiver_ss58, "Not Set") == 0;
	if (missing) {
		memcpy(receiver_pk, sender_pk, KEY_LEN);
		return true;
	}
	RobonomicsPublicKey pubk;
	if (!getPublicKeyFromAddr(receiver_ss58, pubk)) {
		debug_outln_error(F("[CPS] Failed to decode owner SS58 for ECDH"));
		return false;
	}
	memcpy(receiver_pk, pubk.bytes, KEY_LEN);
	return true;
}

static void fillTestSeed(uint8_t out[KEY_LEN], uint8_t pattern) {
	memset(out, pattern, KEY_LEN);
}

static void fillTestSeedMixed(uint8_t out[KEY_LEN]) {
	memset(out, 0x11, 16);
	memset(out + 16, 0x22, 16);
}

typedef void (*SeedFillFn)(uint8_t out[KEY_LEN]);

struct EcdhTestCase {
	SeedFillFn sender_seed;
	SeedFillFn receiver_seed;  // peer Ed25519 public is derived from this seed
	const char *expected_shared_hex;
};

static void fillSeed01(uint8_t out[KEY_LEN]) { fillTestSeed(out, 0x01); }
static void fillSeed02(uint8_t out[KEY_LEN]) { fillTestSeed(out, 0x02); }
static void fillSeedAa(uint8_t out[KEY_LEN]) { fillTestSeed(out, 0xaa); }

static bool hexEq32(const uint8_t *bytes, const char *hex) {
	uint8_t expected[KEY_LEN];
	return hex && parseHex32(hex, expected) && memcmp(bytes, expected, KEY_LEN) == 0;
}

static bool runEcdhCase(const EcdhTestCase &tc, uint8_t case_id) {
	uint8_t sender_sk[KEY_LEN];
	uint8_t receiver_sk[KEY_LEN];
	uint8_t receiver_pk[KEY_LEN];
	tc.sender_seed(sender_sk);
	tc.receiver_seed(receiver_sk);
	Ed25519::derivePublicKey(receiver_pk, receiver_sk);

	uint8_t shared[KEY_LEN];
	if (!deriveSharedSecretEd25519(sender_sk, receiver_pk, shared)) {
		debug_outln_info(String(F("[CPS][ecdh] case ")) + String(case_id) + F(" derive failed"));
		return false;
	}

	const String priv_hex = bytesToHex(sender_sk, KEY_LEN);
	const String pub_hex = bytesToHex(receiver_pk, KEY_LEN);
	const String shared_hex = bytesToHex(shared, KEY_LEN);
	debug_outln_info(String(F("[CPS][ecdh] (\"")) + priv_hex + F("\",\"") + pub_hex + F("\",\"") + shared_hex +
			 F("\")"));

	if (tc.expected_shared_hex && !hexEq32(shared, tc.expected_shared_hex)) {
		debug_outln_info(String(F("[CPS][ecdh] case ")) + String(case_id) + F(" shared mismatch, expected=") +
				 String(tc.expected_shared_hex));
		return false;
	}
	return true;
}

struct EncryptTestCase {
	SeedFillFn sender_seed;
	SeedFillFn receiver_seed;
	const char *expected_aes_key_hex;
	const char *expected_ciphertext_hex;  // ciphertext || tag, hex
	const char *expected_from_ss58;
};

static bool hexEqN(const uint8_t *bytes, size_t len, const char *hex) {
	if (!hex || strlen(hex) != len * 2) {
		return false;
	}
	for (size_t i = 0; i < len; i++) {
		char a = hex[i * 2];
		char b = hex[i * 2 + 1];
		auto nib = [](char c) -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		const int hi = nib(a);
		const int lo = nib(b);
		if (hi < 0 || lo < 0) {
			return false;
		}
		if (bytes[i] != static_cast<uint8_t>((hi << 4) | lo)) {
			return false;
		}
	}
	return true;
}

static bool runEncryptCase(const EncryptTestCase &tc, uint8_t case_id) {
	const char *plain = "850";
	const uint8_t fixed_nonce[GCM_NONCE_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

	uint8_t sender_sk[KEY_LEN];
	uint8_t receiver_sk[KEY_LEN];
	uint8_t receiver_pk[KEY_LEN];
	tc.sender_seed(sender_sk);
	tc.receiver_seed(receiver_sk);
	Ed25519::derivePublicKey(receiver_pk, receiver_sk);

	uint8_t shared[KEY_LEN];
	if (!deriveSharedSecretEd25519(sender_sk, receiver_pk, shared)) {
		debug_outln_info(String(F("[CPS][enc] case ")) + String(case_id) + F(" ECDH failed"));
		return false;
	}

	uint8_t aes_key[KEY_LEN];
	if (!hkdfAesGcmKey(shared, aes_key)) {
		debug_outln_info(String(F("[CPS][enc] case ")) + String(case_id) + F(" HKDF failed"));
		return false;
	}

	uint8_t nonce[GCM_NONCE_LEN];
	uint8_t *cipher = nullptr;
	size_t cipher_len = 0;
	if (!aesGcmEncrypt(aes_key, reinterpret_cast<const uint8_t *>(plain), strlen(plain), nonce, &cipher,
			   &cipher_len, fixed_nonce)) {
		debug_outln_info(String(F("[CPS][enc] case ")) + String(case_id) + F(" AES-GCM failed"));
		return false;
	}

	uint8_t *recovered = nullptr;
	size_t recovered_len = 0;
	const bool ok_dec =
		aesGcmDecrypt(aes_key, cipher, cipher_len, nonce, &recovered, &recovered_len) &&
		recovered_len == strlen(plain) && memcmp(recovered, plain, recovered_len) == 0;
	free(recovered);
	if (!ok_dec) {
		free(cipher);
		debug_outln_info(String(F("[CPS][enc] case ")) + String(case_id) + F(" decrypt roundtrip failed"));
		return false;
	}

	const String priv_hex = bytesToHex(sender_sk, KEY_LEN);
	const String pub_hex = bytesToHex(receiver_pk, KEY_LEN);
	const String aes_hex = bytesToHex(aes_key, KEY_LEN);
	const String ct_hex = bytesToHex(cipher, cipher_len);
	const String from_ss58 = encodeSenderSs58(sender_sk);

	debug_outln_info(String(F("[CPS][enc] (\"")) + priv_hex + F("\",\"") + pub_hex + F("\",\"") + plain +
			 F("\",\"000102030405060708090a0b\",\"") + aes_hex + F("\",\"") + ct_hex + F("\",\"") +
			 from_ss58 + F("\")"));

	if (tc.expected_aes_key_hex && !hexEq32(aes_key, tc.expected_aes_key_hex)) {
		debug_outln_info(String(F("[CPS][enc] case ")) + String(case_id) + F(" aes_key mismatch"));
		free(cipher);
		return false;
	}
	if (tc.expected_ciphertext_hex && !hexEqN(cipher, cipher_len, tc.expected_ciphertext_hex)) {
		debug_outln_info(String(F("[CPS][enc] case ")) + String(case_id) + F(" ciphertext mismatch"));
		free(cipher);
		return false;
	}
	if (tc.expected_from_ss58 && from_ss58 != tc.expected_from_ss58) {
		debug_outln_info(String(F("[CPS][enc] case ")) + String(case_id) + F(" from_ss58 mismatch"));
		free(cipher);
		return false;
	}
	free(cipher);
	return true;
}

/**
 * derive_shared_secret + encrypt vectors — libcps_TEST_VECTORS.md
 */
bool valueCryptoSelfTest() {
	static const EcdhTestCase kCases[] = {
	    {fillSeed01, fillSeed01, "4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621"},
	    {fillSeed01, fillSeed02, "4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b"},
	    {fillSeed02, fillSeed01, "4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b"},
	    {fillSeedAa, fillTestSeedMixed, "190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64"},
	    {fillTestSeedMixed, fillSeedAa, "190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64"},
	};

	static const EncryptTestCase kEncCases[] = {
	    {fillSeed01, fillSeed01, nullptr, nullptr, nullptr},
	    {fillSeed01, fillSeed02, "3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5",
	     "31ffdff4407a99a4fb8c85cc5be4111280e6fb", "4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9"},
	    {fillSeed02, fillSeed01, nullptr, nullptr, nullptr},
	    {fillSeedAa, fillTestSeedMixed, nullptr, nullptr, nullptr},
	    {fillTestSeedMixed, fillSeedAa, nullptr, nullptr, nullptr},
	};

	debug_outln_info(F("[CPS][ecdh] === derive_shared_secret: [(private, public, shared)] ==="));
	for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); i++) {
		if (!runEcdhCase(kCases[i], static_cast<uint8_t>(i + 1))) {
			return false;
		}
	}
	debug_outln_info(F("[CPS][ecdh] ALL OK"));

	debug_outln_info(F("[CPS][enc] === encrypt: [(private, public, plaintext, nonce, aes_key, ciphertext, from_ss58)] ==="));
	debug_outln_info(F("[CPS][enc] plaintext=850 nonce=000102030405060708090a0b HKDF salt=robonomics-network info=aesgcm256"));
	for (size_t i = 0; i < sizeof(kEncCases) / sizeof(kEncCases[0]); i++) {
		if (!runEncryptCase(kEncCases[i], static_cast<uint8_t>(i + 1))) {
			return false;
		}
	}
	debug_outln_info(F("[CPS][enc] ALL OK"));
	return true;
}

/**
 * Main CPS encryption for a single value (e.g. "850").
 * Returns: e.<base64(json)> or empty string on error.
 */
String encryptCpsForOwner(const String &plain, const char *sender_sk_hex, const char *receiver_ss58) {
	if (plain.isEmpty() || !sender_sk_hex) {
		return String();
	}

	// 1) device private key from hex in config
	uint8_t sender_sk[KEY_LEN];
	if (!parseHex32(sender_sk_hex, sender_sk)) {
		debug_outln_error(F("[CPS] Invalid device private key hex"));
		return String();
	}

	// 2) device public key (goes into JSON as "from")
	uint8_t sender_pk[KEY_LEN];
	Ed25519::derivePublicKey(sender_pk, sender_sk);

	// 3) receiver public key (owner or ourselves)
	uint8_t receiver_pk[KEY_LEN];
	if (!resolveReceiverPublic(receiver_ss58, sender_pk, receiver_pk)) {
		return String();
	}

	// 4) shared secret with owner
	uint8_t shared[KEY_LEN];
	if (!deriveSharedSecretEd25519(sender_sk, receiver_pk, shared)) {
		debug_outln_error(F("[CPS] ECDH shared secret failed"));
		return String();
	}

	// 5) AES key from shared secret
	uint8_t enc_key[KEY_LEN];
	if (!hkdfAesGcmKey(shared, enc_key)) {
		debug_outln_error(F("[CPS] HKDF failed"));
		return String();
	}

	// 6) encrypt plaintext (numeric string)
	uint8_t nonce[GCM_NONCE_LEN];
	uint8_t *cipher = nullptr;
	size_t cipher_len = 0;
	if (!aesGcmEncrypt(enc_key, reinterpret_cast<const uint8_t *>(plain.c_str()), plain.length(), nonce, &cipher,
			   &cipher_len)) {
		debug_outln_error(F("[CPS] AES-GCM encrypt failed"));
		return String();
	}

	// 7) JSON: from = device SS58 address (with checksum!), not raw base58 pubkey
	const String from_ss58 = encodeSenderSs58(sender_sk);
	const String nonce_b64 = base64Encode(nonce, GCM_NONCE_LEN);
	const String ct_b64 = base64Encode(cipher, cipher_len);
	free(cipher);

	if (from_ss58.isEmpty() || nonce_b64.isEmpty() || ct_b64.isEmpty()) {
		return String();
	}

	String json;
	json.reserve(96 + from_ss58.length() + nonce_b64.length() + ct_b64.length());
	json += F("{\"version\":1,\"algorithm\":\"aesgcm256\",\"from\":\"");
	json += from_ss58;
	json += F("\",\"nonce\":\"");
	json += nonce_b64;
	json += F("\",\"ciphertext\":\"");
	json += ct_b64;
	json += F("\"}");

	// 8) JSON → base64, prefix with e. (so connectivity does not parse as float)
	const String wrapped = base64Encode(reinterpret_cast<const uint8_t *>(json.c_str()), json.length());
	if (wrapped.isEmpty()) {
		return String();
	}
	return String(VALUE_CRYPTO_CPS_PREFIX) + wrapped;
}

/**
 * Convenience wrapper for the formatter:
 * uses cfg::private_key + cfg::rws_owner to encrypt a value.
 * If keys are missing or encryption fails, returns an empty string so callers
 * can fail closed instead of sending plaintext.
 */
String encryptValue(const String &plain) {
	if (plain.isEmpty()) {
		return plain;
	}
	const char *sk = cfg::private_key;
	if (!sk || strcasecmp(sk, "Not Set") == 0 || strlen(sk) < 64) {
		debug_outln_error(F("[CPS] Device private key not set; cannot encrypt"));
		return String();
	}
	const String cps = encryptCpsForOwner(plain, sk, cfg::rws_owner);
	if (!cps.isEmpty()) {
		return cps;
	}
	debug_outln_error(F("[CPS] Encrypt failed; plaintext fallback disabled"));
	return String();
}

bool encryptBytesForOwner(const uint8_t *plain, size_t plain_len, uint8_t from_pk[KEY_LEN],
			  uint8_t nonce[GCM_NONCE_LEN], uint8_t **cipher_out, size_t *cipher_len) {
	if (!plain || plain_len == 0 || !from_pk || !nonce || !cipher_out || !cipher_len) {
		return false;
	}
	const char *sk = cfg::private_key;
	if (!sk || strcasecmp(sk, "Not Set") == 0 || strlen(sk) < 64) {
		debug_outln_error(F("[CPS] Device private key not set; cannot encrypt"));
		return false;
	}
	uint8_t sender_sk[KEY_LEN];
	if (!parseHex32(sk, sender_sk)) {
		debug_outln_error(F("[CPS] Invalid device private key hex"));
		return false;
	}
	uint8_t sender_pk[KEY_LEN];
	Ed25519::derivePublicKey(sender_pk, sender_sk);
	memcpy(from_pk, sender_pk, KEY_LEN);

	uint8_t receiver_pk[KEY_LEN];
	if (!resolveReceiverPublic(cfg::rws_owner, sender_pk, receiver_pk)) {
		return false;
	}
	uint8_t shared[KEY_LEN];
	if (!deriveSharedSecretEd25519(sender_sk, receiver_pk, shared)) {
		debug_outln_error(F("[CPS] ECDH shared secret failed"));
		return false;
	}
	uint8_t enc_key[KEY_LEN];
	if (!hkdfAesGcmKey(shared, enc_key)) {
		debug_outln_error(F("[CPS] HKDF failed"));
		return false;
	}
	if (!aesGcmEncrypt(enc_key, plain, plain_len, nonce, cipher_out, cipher_len)) {
		debug_outln_error(F("[CPS] AES-GCM encrypt failed"));
		return false;
	}
	return true;
}

}  // namespace cps

String valueCryptoEncryptCpsForOwner(const String &plain, const char *sender_sk_hex, const char *receiver_ss58) {
	return cps::encryptCpsForOwner(plain, sender_sk_hex, receiver_ss58);
}

String valueCryptoEncryptValue(const String &plain) {
	return cps::encryptValue(plain);
}

bool valueCryptoParseHex32(const char *hex, uint8_t out[VALUE_CRYPTO_KEY_LEN]) {
	return cps::parseHex32(hex, out);
}

bool valueCryptoDeviceKeys(uint8_t secret_key[VALUE_CRYPTO_KEY_LEN], uint8_t public_key[VALUE_CRYPTO_KEY_LEN]) {
	const char *sk = cfg::private_key;
	if (!secret_key || !public_key || !sk || strcasecmp(sk, "Not Set") == 0) {
		return false;
	}
	if (!cps::parseHex32(sk, secret_key)) {
		return false;
	}
	Ed25519::derivePublicKey(public_key, secret_key);
	return true;
}

bool valueCryptoOwnerPublicKey(const uint8_t sender_pk[VALUE_CRYPTO_KEY_LEN],
			       uint8_t owner_pk[VALUE_CRYPTO_KEY_LEN]) {
	if (!sender_pk || !owner_pk) {
		return false;
	}
	return cps::resolveReceiverPublic(cfg::rws_owner, sender_pk, owner_pk);
}

bool valueCryptoEncryptBytesForOwner(const uint8_t *plain, size_t plain_len, uint8_t from_pk[VALUE_CRYPTO_KEY_LEN],
				     uint8_t nonce[VALUE_CRYPTO_GCM_NONCE_LEN], uint8_t **cipher_out,
				     size_t *cipher_len) {
	return cps::encryptBytesForOwner(plain, plain_len, from_pk, nonce, cipher_out, cipher_len);
}

bool valueCryptoSelfTest() {
	return cps::valueCryptoSelfTest();
}

# CPS interoperability test vectors

Agreed vectors for **libcps** and embedded implementations (e.g. Altruist firmware).
Both sides run the same inputs and compare outputs.

**Algorithms:** Ed25519 ECDH → X25519 ([`derive_shared_secret`](https://github.com/airalab/robonomics/blob/master/tools/libcps/src/crypto/cipher.rs#L155)),
HKDF-SHA256 ([L38](https://github.com/airalab/robonomics/blob/master/tools/libcps/src/crypto/cipher.rs#L38):
`salt=robonomics-network`, `IKM=shared`, `info=aesgcm256`),
AES-256-GCM (`EncryptionAlgorithm::AesGcm256`).

Wire JSON `from` field: **SS58** (Robonomics network prefix 32), not raw base58 pubkey.

---

## 1. ECDH vectors

Format: `[(private, public, shared)]` — all hex, 32 bytes each (64 hex chars).

Test pattern (same idea as [polkadot-sdk ed25519 tests](https://paritytech.github.io/polkadot-sdk/master/src/sp_core/ed25519.rs.html#312)):

```rust
for (private_hex, public_hex, expected_shared_hex) in ECDH_VECTORS {
    let got = cipher.derive_shared_secret(&decode_32(public_hex))?;
    assert_eq!(hex::encode(got), expected_shared_hex);
}
```

### Key material

| Label | private (seed)                                                     | public (Ed25519)                                                   |
| ----- | ------------------------------------------------------------------ | ------------------------------------------------------------------ |
| A     | `0101010101010101010101010101010101010101010101010101010101010101` | `8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c` |
| B     | `0202020202020202020202020202020202020202020202020202020202020202` | `8139770ea87d175f56a35466c34c7ecccb8d8a91b4ee37a25df60f5b8fc9b394` |
| C     | `aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa` | `e734ea6c2b6257de72355e472aa05a4c487e6b463c029ed306df2f01b5636b58` |
| D     | `1111111111111111111111111111111122222222222222222222222222222222` | `a2f96ca5200376b6756b152834971f271b078f74f15ec409e307c53d4e68f9c4` |

### Vectors

| #   | private (sender) | public (peer) | shared                                                             |
| --- | ---------------- | ------------- | ------------------------------------------------------------------ |
| 1   | A                | A             | `4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621` |
| 2   | A                | B             | `4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b` |
| 3   | B                | A             | `4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b` |
| 4   | C                | D             | `190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64` |
| 5   | D                | C             | `190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64` |

```rust
pub const ECDH_VECTORS: &[(&str, &str, &str)] = &[
    ("0101010101010101010101010101010101010101010101010101010101010101",
     "8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c",
     "4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621"),
    ("0101010101010101010101010101010101010101010101010101010101010101",
     "8139770ea87d175f56a35466c34c7ecccb8d8a91b4ee37a25df60f5b8fc9b394",
     "4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b"),
    ("0202020202020202020202020202020202020202020202020202020202020202",
     "8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c",
     "4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b"),
    ("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     "a2f96ca5200376b6756b152834971f271b078f74f15ec409e307c53d4e68f9c4",
     "190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64"),
    ("1111111111111111111111111111111122222222222222222222222222222222",
     "e734ea6c2b6257de72355e472aa05a4c487e6b463c029ed306df2f01b5636b58",
     "190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64"),
];
```

---

## 2. AES vectors (HKDF + AES-GCM)

Isolated checks **without** ECDH — useful to verify HKDF and AES-256-GCM alone.

Fixed: `plaintext = "850"`, `nonce = 000102030405060708090a0b`,
HKDF `salt=robonomics-network`, `info=aesgcm256`.

### 2a. HKDF: `[(shared, aes_key)]`

```rust
for (shared_hex, want_aes_hex) in HKDF_VECTORS {
    let aes = hkdf_aesgcm256(&decode_32(shared_hex));
    assert_eq!(hex::encode(aes), want_aes_hex);
}
```

| #   | shared (from ECDH)                                                 | aes_key                                                            |
| --- | ------------------------------------------------------------------ | ------------------------------------------------------------------ |
| 1   | `4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621` | `127cc72ec661b3da60ae651cc432cdf05172169b50ddcce15021edfa917e21a3` |
| 2   | `4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b` | `3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5` |
| 3   | `190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64` | `e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd` |

```rust
pub const HKDF_VECTORS: &[(&str, &str)] = &[
    ("4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621",
     "127cc72ec661b3da60ae651cc432cdf05172169b50ddcce15021edfa917e21a3"),
    ("4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b",
     "3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5"),
    ("190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64",
     "e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd"),
];
```

### 2b. AES-GCM: `[(aes_key, nonce, plaintext, ciphertext)]`

`ciphertext` includes the 16-byte auth tag.

```rust
for (aes_hex, nonce_hex, plain, want_ct_hex) in AES_GCM_VECTORS {
    let ct = aes256_gcm_encrypt(&decode_32(aes_hex), &decode_12(nonce_hex), plain.as_bytes());
    assert_eq!(hex::encode(ct), want_ct_hex);
}
```

| #   | aes_key                                                            | nonce                      | plaintext | ciphertext\|tag                          |
| --- | ------------------------------------------------------------------ | -------------------------- | --------- | ---------------------------------------- |
| 1   | `127cc72ec661b3da60ae651cc432cdf05172169b50ddcce15021edfa917e21a3` | `000102030405060708090a0b` | `850`     | `da7a64d7c20cea65161cab8b01d344210d613c` |
| 2   | `3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5` | `000102030405060708090a0b` | `850`     | `31ffdff4407a99a4fb8c85cc5be4111280e6fb` |
| 3   | `e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd` | `000102030405060708090a0b` | `850`     | `6060b7d1179a44f4f449309280d6fa0438609c` |

```rust
pub const AES_GCM_VECTORS: &[(&str, &str, &str, &str)] = &[
    ("127cc72ec661b3da60ae651cc432cdf05172169b50ddcce15021edfa917e21a3",
     "000102030405060708090a0b", "850",
     "da7a64d7c20cea65161cab8b01d344210d613c"),
    ("3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5",
     "000102030405060708090a0b", "850",
     "31ffdff4407a99a4fb8c85cc5be4111280e6fb"),
    ("e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd",
     "000102030405060708090a0b", "850",
     "6060b7d1179a44f4f449309280d6fa0438609c"),
];
```

---

## 3. Encryption vectors (full pipeline)

Format: `[(private, public, plaintext, nonce, aes_key, ciphertext, from_ss58)]`.

Fixed for all rows:

- `plaintext = "850"`
- `nonce = 000102030405060708090a0b` (12 bytes)
- `algorithm = aesgcm256`
- `ciphertext` = AES-GCM output **with 16-byte auth tag** appended

Test pattern:

```rust
for (priv_hex, pub_hex, plain, nonce_hex, want_aes, want_ct, want_from) in ENCRYPT_VECTORS {
    let msg = cipher_with_seed(priv_hex)
        .encrypt(plain.as_bytes(), &decode_32(pub_hex), AesGcm256, &decode_12(nonce_hex))?;
    assert_eq!(hex::encode(derived_aes_key), want_aes);
    assert_eq!(hex::encode(&msg.ciphertext), want_ct);
    assert_eq!(ss58_from_sender(priv_hex), want_from);
}
```

(`encrypt` in libcps normally draws a random nonce; tests must use the fixed nonce above.)

### Vectors (verified on Altruist firmware, debug self-test)

| #   | private | public (peer) | aes_key                                                            | ciphertext\|tag                          | from (SS58)                                        |
| --- | ------- | ------------- | ------------------------------------------------------------------ | ---------------------------------------- | -------------------------------------------------- |
| 1   | A       | A             | `127cc72ec661b3da60ae651cc432cdf05172169b50ddcce15021edfa917e21a3` | `da7a64d7c20cea65161cab8b01d344210d613c` | `4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9` |
| 2   | A       | B             | `3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5` | `31ffdff4407a99a4fb8c85cc5be4111280e6fb` | `4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9` |
| 3   | B       | A             | `3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5` | `31ffdff4407a99a4fb8c85cc5be4111280e6fb` | `4F7YX7z6BMEJmnM2gTWzSmczatVwA9euxEmZmNLTaDnmbfu6` |
| 4   | C       | D             | `e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd` | `6060b7d1179a44f4f449309280d6fa0438609c` | `4HRG42KdgDYNtSYrAnsf84bE7Nq41yzvhhnNHqnrgzLwufu9` |
| 5   | D       | C             | `e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd` | `6060b7d1179a44f4f449309280d6fa0438609c` | `4Fso89vttCkKLngaxgnMEqFhPppGMbopxgeR5ZSaF5hi2nKp` |

Vec #2/#3 and #4/#5: same `aes_key` and `ciphertext` (DH symmetry); `from_ss58` differs (sender changed).

Copy-paste tuples:

```text
("0101010101010101010101010101010101010101010101010101010101010101","8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c","850","000102030405060708090a0b","127cc72ec661b3da60ae651cc432cdf05172169b50ddcce15021edfa917e21a3","da7a64d7c20cea65161cab8b01d344210d613c","4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9")
("0101010101010101010101010101010101010101010101010101010101010101","8139770ea87d175f56a35466c34c7ecccb8d8a91b4ee37a25df60f5b8fc9b394","850","000102030405060708090a0b","3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5","31ffdff4407a99a4fb8c85cc5be4111280e6fb","4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9")
("0202020202020202020202020202020202020202020202020202020202020202","8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c","850","000102030405060708090a0b","3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5","31ffdff4407a99a4fb8c85cc5be4111280e6fb","4F7YX7z6BMEJmnM2gTWzSmczatVwA9euxEmZmNLTaDnmbfu6")
("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","a2f96ca5200376b6756b152834971f271b078f74f15ec409e307c53d4e68f9c4","850","000102030405060708090a0b","e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd","6060b7d1179a44f4f449309280d6fa0438609c","4HRG42KdgDYNtSYrAnsf84bE7Nq41yzvhhnNHqnrgzLwufu9")
("1111111111111111111111111111111122222222222222222222222222222222","e734ea6c2b6257de72355e472aa05a4c487e6b463c029ed306df2f01b5636b58","850","000102030405060708090a0b","e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd","6060b7d1179a44f4f449309280d6fa0438609c","4Fso89vttCkKLngaxgnMEqFhPppGMbopxgeR5ZSaF5hi2nKp")
```

```rust
pub const ENCRYPT_VECTORS: &[(&str, &str, &str, &str, &str, &str, &str)] = &[
    ("0101010101010101010101010101010101010101010101010101010101010101",
     "8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c",
     "850", "000102030405060708090a0b",
     "127cc72ec661b3da60ae651cc432cdf05172169b50ddcce15021edfa917e21a3",
     "da7a64d7c20cea65161cab8b01d344210d613c",
     "4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9"),
    ("0101010101010101010101010101010101010101010101010101010101010101",
     "8139770ea87d175f56a35466c34c7ecccb8d8a91b4ee37a25df60f5b8fc9b394",
     "850", "000102030405060708090a0b",
     "3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5",
     "31ffdff4407a99a4fb8c85cc5be4111280e6fb",
     "4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9"),
    ("0202020202020202020202020202020202020202020202020202020202020202",
     "8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c",
     "850", "000102030405060708090a0b",
     "3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5",
     "31ffdff4407a99a4fb8c85cc5be4111280e6fb",
     "4F7YX7z6BMEJmnM2gTWzSmczatVwA9euxEmZmNLTaDnmbfu6"),
    ("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     "a2f96ca5200376b6756b152834971f271b078f74f15ec409e307c53d4e68f9c4",
     "850", "000102030405060708090a0b",
     "e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd",
     "6060b7d1179a44f4f449309280d6fa0438609c",
     "4HRG42KdgDYNtSYrAnsf84bE7Nq41yzvhhnNHqnrgzLwufu9"),
    ("1111111111111111111111111111111122222222222222222222222222222222",
     "e734ea6c2b6257de72355e472aa05a4c487e6b463c029ed306df2f01b5636b58",
     "850", "000102030405060708090a0b",
     "e60e05f2a5f05c1922e9eb9b6b98bcec5228ad2485e5ef2e841c063e942530dd",
     "6060b7d1179a44f4f449309280d6fa0438609c",
     "4Fso89vttCkKLngaxgnMEqFhPppGMbopxgeR5ZSaF5hi2nKp"),
];
```

```text
[CPS][enc] ("<private>","<public>","850","000102030405060708090a0b","<aes_key>","<ciphertext>","<from_ss58>")
```

---

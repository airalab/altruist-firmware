#ifndef _PROVISION_JS_H
#define _PROVISION_JS_H

extern const char WEB_PAGE_PROVISION_JS[] PROGMEM = R"rawliteral(
/* Local-only Sensor.Social key derivation (ed25519 MASTER_SEED//N). Seed never POSTed. */
(function (root) {
  'use strict';

  var IV = [
    0x6a09e667f3bcc908n, 0xbb67ae8584caa73bn, 0x3c6ef372fe94f82bn, 0xa54ff53a5f1d36f1n,
    0x510e527fade682d1n, 0x9b05688c2b3e6c1fn, 0x1f83d9abfb41bd6bn, 0x5be0cd19137e2179n
  ];
  var SIGMA = [
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    [14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3],
    [11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4],
    [7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8],
    [9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13],
    [2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9],
    [12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11],
    [13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10],
    [6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5],
    [10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0],
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    [14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3]
  ];

  function u64(x) { return BigInt.asUintN(64, BigInt(x)); }
  function add64(a, b) { return u64(a + b); }
  function xor64(a, b) { return u64(a ^ b); }
  function rotr64(x, n) { x = u64(x); n = BigInt(n); return u64((x >> n) | (x << (64n - n))); }

  function blake2b(msg, outlen) {
    var h = IV.slice();
    h[0] = u64(h[0] ^ BigInt(0x01010000 ^ outlen));
    var t0 = 0n, t1 = 0n;
    var offset = 0;
    var last = false;
    function compress(block, isLast) {
      var v = h.concat(IV);
      v[12] = xor64(v[12], t0);
      v[13] = xor64(v[13], t1);
      if (isLast) v[14] = xor64(v[14], 0xffffffffffffffffn);
      var m = [];
      for (var i = 0; i < 16; i++) {
        var o = i * 8;
        m[i] = BigInt(block[o]) | (BigInt(block[o + 1]) << 8n) | (BigInt(block[o + 2]) << 16n) |
          (BigInt(block[o + 3]) << 24n) | (BigInt(block[o + 4]) << 32n) | (BigInt(block[o + 5]) << 40n) |
          (BigInt(block[o + 6]) << 48n) | (BigInt(block[o + 7]) << 56n);
      }
      function G(a, b, c, d, x, y) {
        v[a] = add64(add64(v[a], v[b]), x);
        v[d] = rotr64(xor64(v[d], v[a]), 32);
        v[c] = add64(v[c], v[d]);
        v[b] = rotr64(xor64(v[b], v[c]), 24);
        v[a] = add64(add64(v[a], v[b]), y);
        v[d] = rotr64(xor64(v[d], v[a]), 16);
        v[c] = add64(v[c], v[d]);
        v[b] = rotr64(xor64(v[b], v[c]), 63);
      }
      for (var r = 0; r < 12; r++) {
        var s = SIGMA[r];
        G(0, 4, 8, 12, m[s[0]], m[s[1]]);
        G(1, 5, 9, 13, m[s[2]], m[s[3]]);
        G(2, 6, 10, 14, m[s[4]], m[s[5]]);
        G(3, 7, 11, 15, m[s[6]], m[s[7]]);
        G(0, 5, 10, 15, m[s[8]], m[s[9]]);
        G(1, 6, 11, 12, m[s[10]], m[s[11]]);
        G(2, 7, 8, 13, m[s[12]], m[s[13]]);
        G(3, 4, 9, 14, m[s[14]], m[s[15]]);
      }
      for (var j = 0; j < 8; j++) h[j] = xor64(xor64(h[j], v[j]), v[j + 8]);
    }
    var block = new Uint8Array(128);
    while (!last) {
      var remain = msg.length - offset;
      if (remain > 128) {
        block.set(msg.subarray(offset, offset + 128));
        offset += 128;
        t0 = u64(t0 + 128n);
        if (t0 < 128n) t1 = u64(t1 + 1n);
        compress(block, false);
      } else {
        block.fill(0);
        if (remain) block.set(msg.subarray(offset));
        t0 = u64(t0 + BigInt(remain));
        if (t0 < BigInt(remain)) t1 = u64(t1 + 1n);
        last = true;
        compress(block, true);
      }
    }
    var out = new Uint8Array(outlen);
    for (var k = 0; k < outlen; k++) {
      out[k] = Number((h[k >> 3] >> BigInt((k & 7) * 8)) & 0xffn);
    }
    return out;
  }

  function blake2b256(msg) { return blake2b(msg, 32); }
  function blake2b512(msg) { return blake2b(msg, 64); }

  function concat() {
    var n = 0, i, a = arguments;
    for (i = 0; i < a.length; i++) n += a[i].length;
    var out = new Uint8Array(n), o = 0;
    for (i = 0; i < a.length; i++) { out.set(a[i], o); o += a[i].length; }
    return out;
  }

  function hexToBytes(hex) {
    hex = hex.replace(/^0x/i, '').replace(/\s+/g, '');
    if (hex.length !== 64 || /[^0-9a-f]/i.test(hex)) return null;
    var out = new Uint8Array(32);
    for (var i = 0; i < 32; i++) out[i] = parseInt(hex.substr(i * 2, 2), 16);
    return out;
  }

  function bytesToHex(b) {
    var s = '';
    for (var i = 0; i < b.length; i++) s += (b[i] + 256).toString(16).slice(1);
    return s;
  }

  function compactAddLength(bytes) {
    if (bytes.length >= 64) throw new Error('scale');
    var out = new Uint8Array(1 + bytes.length);
    out[0] = bytes.length << 2;
    out.set(bytes, 1);
    return out;
  }

  var HDKD = compactAddLength(new TextEncoder().encode('Ed25519HDKD'));

  function deriveHard(seed, index) {
    var cc = new Uint8Array(32);
    var n = index >>> 0;
    cc[0] = n & 255; cc[1] = (n >>> 8) & 255; cc[2] = (n >>> 16) & 255; cc[3] = (n >>> 24) & 255;
    return blake2b256(concat(HDKD, seed, cc));
  }

  var B58 = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz';

  function base58Encode(bytes) {
    var zeros = 0;
    while (zeros < bytes.length && bytes[zeros] === 0) zeros++;
    var size = Math.ceil(bytes.length * 138 / 100) + 1;
    var b = new Uint8Array(size);
    for (var i = zeros; i < bytes.length; i++) {
      var carry = bytes[i];
      for (var j = size - 1; j >= 0; j--) {
        carry += 256 * b[j];
        b[j] = carry % 58;
        carry = (carry / 58) | 0;
      }
    }
    var it = 0;
    while (it < size && b[it] === 0) it++;
    var out = '';
    for (var z = 0; z < zeros; z++) out += '1';
    for (; it < size; it++) out += B58[b[it]];
    return out;
  }

  function ss58Encode(pubkey, prefix) {
    prefix = prefix == null ? 32 : prefix;
    var body = new Uint8Array(1 + pubkey.length);
    body[0] = prefix;
    body.set(pubkey, 1);
    var cs = blake2b512(concat(new TextEncoder().encode('SS58PRE'), body)).subarray(0, 2);
    return base58Encode(concat(body, cs));
  }

  function rotr32(x, n) { return (x >>> n) | (x << (32 - n)); }

  function sha256(msg) {
    var K = [
      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    ];
    var h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
    var h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;
    var blocks = Math.ceil((msg.length + 1 + 8) / 64);
    var buf = new Uint8Array(blocks * 64);
    buf.set(msg);
    buf[msg.length] = 0x80;
    var dv = new DataView(buf.buffer);
    dv.setUint32(buf.length - 4, msg.length * 8, false);
    for (var off = 0; off < buf.length; off += 64) {
      var w = new Uint32Array(64), i;
      for (i = 0; i < 16; i++) w[i] = dv.getUint32(off + i * 4, false);
      for (i = 16; i < 64; i++) {
        var s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >>> 3);
        var s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >>> 10);
        w[i] = (w[i - 16] + s0 + w[i - 7] + s1) >>> 0;
      }
      var a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
      for (i = 0; i < 64; i++) {
        var S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        var ch = (e & f) ^ (~e & g);
        var t1 = (h + S1 + ch + K[i] + w[i]) >>> 0;
        var S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        var maj = (a & b) ^ (a & c) ^ (b & c);
        var t2 = (S0 + maj) >>> 0;
        h = g; g = f; f = e; e = (d + t1) >>> 0; d = c; c = b; b = a; a = (t1 + t2) >>> 0;
      }
      h0 = (h0 + a) >>> 0; h1 = (h1 + b) >>> 0; h2 = (h2 + c) >>> 0; h3 = (h3 + d) >>> 0;
      h4 = (h4 + e) >>> 0; h5 = (h5 + f) >>> 0; h6 = (h6 + g) >>> 0; h7 = (h7 + h) >>> 0;
    }
    var out = new Uint8Array(32);
    var o = new DataView(out.buffer);
    o.setUint32(0, h0, false); o.setUint32(4, h1, false); o.setUint32(8, h2, false); o.setUint32(12, h3, false);
    o.setUint32(16, h4, false); o.setUint32(20, h5, false); o.setUint32(24, h6, false); o.setUint32(28, h7, false);
    return out;
  }

  function add64w(ah, al, bh, bl) {
    var lo = (al >>> 0) + (bl >>> 0);
    var hi = (ah >>> 0) + (bh >>> 0) + ((lo / 0x100000000) >>> 0);
    return [hi >>> 0, lo >>> 0];
  }
  function rotr64w(h, l, n) {
    if (n < 32) return [((h >>> n) | (l << (32 - n))) >>> 0, ((l >>> n) | (h << (32 - n))) >>> 0];
    n -= 32;
    return [((l >>> n) | (h << (32 - n))) >>> 0, ((h >>> n) | (l << (32 - n))) >>> 0];
  }
  function shr64w(h, l, n) {
    if (n < 32) return [h >>> n, ((l >>> n) | (h << (32 - n))) >>> 0];
    return [0, h >>> (n - 32)];
  }
  function xor64w(a, b) { return [a[0] ^ b[0], a[1] ^ b[1]]; }

  function sha512(msg) {
    var K = [
      0x428a2f98,0xd728ae22,0x71374491,0x23ef65cd,0xb5c0fbcf,0xec4d3b2f,0xe9b5dba5,0x8189dbbc,
      0x3956c25b,0xf348b538,0x59f111f1,0xb605d019,0x923f82a4,0xaf194f9b,0xab1c5ed5,0xda6d8118,
      0xd807aa98,0xa3030242,0x12835b01,0x45706fbe,0x243185be,0x4ee4b28c,0x550c7dc3,0xd5ffb4e2,
      0x72be5d74,0xf27b896f,0x80deb1fe,0x3b1696b1,0x9bdc06a7,0x25c71235,0xc19bf174,0xcf692694,
      0xe49b69c1,0x9ef14ad2,0xefbe4786,0x384f25e3,0x0fc19dc6,0x8b8cd5b5,0x240ca1cc,0x77ac9c65,
      0x2de92c6f,0x592b0275,0x4a7484aa,0x6ea6e483,0x5cb0a9dc,0xbd41fbd4,0x76f988da,0x831153b5,
      0x983e5152,0xee66dfab,0xa831c66d,0x2db43210,0xb00327c8,0x98fb213f,0xbf597fc7,0xbeef0ee4,
      0xc6e00bf3,0x3da88fc2,0xd5a79147,0x930aa725,0x06ca6351,0xe003826f,0x14292967,0x0a0e6e70,
      0x27b70a85,0x46d22ffc,0x2e1b2138,0x5c26c926,0x4d2c6dfc,0x5ac42aed,0x53380d13,0x9d95b3df,
      0x650a7354,0x8baf63de,0x766a0abb,0x3c77b2a8,0x81c2c92e,0x47edaee6,0x92722c85,0x1482353b,
      0xa2bfe8a1,0x4cf10364,0xa81a664b,0xbc423001,0xc24b8b70,0xd0f89791,0xc76c51a3,0x0654be30,
      0xd192e819,0xd6ef5218,0xd6990624,0x5565a910,0xf40e3585,0x5771202a,0x106aa070,0x32bbd1b8,
      0x19a4c116,0xb8d2d0c8,0x1e376c08,0x5141ab53,0x2748774c,0xdf8eeb99,0x34b0bcb5,0xe19b48a8,
      0x391c0cb3,0xc5c95a63,0x4ed8aa4a,0xe3418acb,0x5b9cca4f,0x7763e373,0x682e6ff3,0xd6b2b8a3,
      0x748f82ee,0x5defb2fc,0x78a5636f,0x43172f60,0x84c87814,0xa1f0ab72,0x8cc70208,0x1a6439ec,
      0x90befffa,0x23631e28,0xa4506ceb,0xde82bde9,0xbef9a3f7,0xb2c67915,0xc67178f2,0xe372532b,
      0xca273ece,0xea26619c,0xd186b8c7,0x21c0c207,0xeada7dd6,0xcde0eb1e,0xf57d4f7f,0xee6ed178,
      0x06f067aa,0x72176fba,0x0a637dc5,0xa2c898a6,0x113f9804,0xbef90dae,0x1b710b35,0x131c471b,
      0x28db77f5,0x23047d84,0x32caab7b,0x40c72493,0x3c9ebe0a,0x15c9bebc,0x431d67c4,0x9c100d4c,
      0x4cc5d4be,0xcb3e42b6,0x597f299c,0xfc657e2a,0x5fcb6fab,0x3ad6faec,0x6c44198c,0x4a475817
    ];
    var H = [
      0x6a09e667,0xf3bcc908,0xbb67ae85,0x84caa73b,0x3c6ef372,0xfe94f82b,0xa54ff53a,0x5f1d36f1,
      0x510e527f,0xade682d1,0x9b05688c,0x2b3e6c1f,0x1f83d9ab,0xfb41bd6b,0x5be0cd19,0x137e2179
    ];
    var blocks = Math.ceil((msg.length + 1 + 16) / 128);
    var buf = new Uint8Array(blocks * 128);
    buf.set(msg);
    buf[msg.length] = 0x80;
    var dv = new DataView(buf.buffer);
    dv.setUint32(buf.length - 4, msg.length * 8, false);
    var wh = new Uint32Array(80), wl = new Uint32Array(80);
    for (var off = 0; off < buf.length; off += 128) {
      var i;
      for (i = 0; i < 16; i++) {
        wh[i] = dv.getUint32(off + i * 8, false);
        wl[i] = dv.getUint32(off + i * 8 + 4, false);
      }
      for (i = 16; i < 80; i++) {
        var s0 = xor64w(xor64w(rotr64w(wh[i - 15], wl[i - 15], 1), rotr64w(wh[i - 15], wl[i - 15], 8)), shr64w(wh[i - 15], wl[i - 15], 7));
        var s1 = xor64w(xor64w(rotr64w(wh[i - 2], wl[i - 2], 19), rotr64w(wh[i - 2], wl[i - 2], 61)), shr64w(wh[i - 2], wl[i - 2], 6));
        var s = add64w(wh[i - 16], wl[i - 16], s0[0], s0[1]);
        s = add64w(s[0], s[1], wh[i - 7], wl[i - 7]);
        s = add64w(s[0], s[1], s1[0], s1[1]);
        wh[i] = s[0]; wl[i] = s[1];
      }
      var ah = H[0], al = H[1], bh = H[2], bl = H[3], ch = H[4], cl = H[5], dh = H[6], dl = H[7];
      var eh = H[8], el = H[9], fh = H[10], fl = H[11], gh = H[12], gl = H[13], hh = H[14], hl = H[15];
      for (i = 0; i < 80; i++) {
        var S1 = xor64w(xor64w(rotr64w(eh, el, 14), rotr64w(eh, el, 18)), rotr64w(eh, el, 41));
        var chv = [(eh & fh) ^ ((~eh) & gh), (el & fl) ^ ((~el) & gl)];
        var t1 = add64w(hh, hl, S1[0], S1[1]);
        t1 = add64w(t1[0], t1[1], chv[0], chv[1]);
        t1 = add64w(t1[0], t1[1], K[i * 2], K[i * 2 + 1]);
        t1 = add64w(t1[0], t1[1], wh[i], wl[i]);
        var S0 = xor64w(xor64w(rotr64w(ah, al, 28), rotr64w(ah, al, 34)), rotr64w(ah, al, 39));
        var maj = [(ah & bh) ^ (ah & ch) ^ (bh & ch), (al & bl) ^ (al & cl) ^ (bl & cl)];
        var t2 = add64w(S0[0], S0[1], maj[0], maj[1]);
        hh = gh; hl = gl; gh = fh; gl = fl; fh = eh; fl = el;
        var ed = add64w(dh, dl, t1[0], t1[1]); eh = ed[0]; el = ed[1];
        dh = ch; dl = cl; ch = bh; cl = bl; bh = ah; bl = al;
        var ad = add64w(t1[0], t1[1], t2[0], t2[1]); ah = ad[0]; al = ad[1];
      }
      var r;
      r = add64w(H[0], H[1], ah, al); H[0] = r[0]; H[1] = r[1];
      r = add64w(H[2], H[3], bh, bl); H[2] = r[0]; H[3] = r[1];
      r = add64w(H[4], H[5], ch, cl); H[4] = r[0]; H[5] = r[1];
      r = add64w(H[6], H[7], dh, dl); H[6] = r[0]; H[7] = r[1];
      r = add64w(H[8], H[9], eh, el); H[8] = r[0]; H[9] = r[1];
      r = add64w(H[10], H[11], fh, fl); H[10] = r[0]; H[11] = r[1];
      r = add64w(H[12], H[13], gh, gl); H[12] = r[0]; H[13] = r[1];
      r = add64w(H[14], H[15], hh, hl); H[14] = r[0]; H[15] = r[1];
    }
    var out = new Uint8Array(64);
    var ov = new DataView(out.buffer);
    for (var k = 0; k < 8; k++) {
      ov.setUint32(k * 8, H[k * 2], false);
      ov.setUint32(k * 8 + 4, H[k * 2 + 1], false);
    }
    return out;
  }

  function hmacSha512(key, data) {
    var k = key.length > 128 ? sha512(key) : key;
    var ipad = new Uint8Array(128), opad = new Uint8Array(128);
    ipad.set(k); opad.set(k);
    for (var i = 0; i < 128; i++) { ipad[i] ^= 0x36; opad[i] ^= 0x5c; }
    var inner = new Uint8Array(128 + data.length);
    inner.set(ipad); inner.set(data, 128);
    var innerH = sha512(inner);
    var outer = new Uint8Array(128 + 64);
    outer.set(opad); outer.set(innerH, 128);
    return sha512(outer);
  }

  function pbkdf2Sha512(password, salt, iter, dkLen) {
    var blocks = Math.ceil(dkLen / 64);
    var out = new Uint8Array(blocks * 64);
    for (var bi = 1; bi <= blocks; bi++) {
      var block = new Uint8Array(salt.length + 4);
      block.set(salt);
      block[salt.length] = (bi >>> 24) & 255;
      block[salt.length + 1] = (bi >>> 16) & 255;
      block[salt.length + 2] = (bi >>> 8) & 255;
      block[salt.length + 3] = bi & 255;
      var u = hmacSha512(password, block);
      var t = u.slice();
      for (var j = 1; j < iter; j++) {
        u = hmacSha512(password, u);
        for (var x = 0; x < 64; x++) t[x] ^= u[x];
      }
      out.set(t, (bi - 1) * 64);
    }
    return out.subarray(0, dkLen);
  }

  var ED_P = (1n << 255n) - 19n;
  var ED_D = 0x52036cee2b6ffe738cc740797779e89800700a4d4141d8ab75eb4dca135978a3n;
  var ED_GX = 0x216936d3cd6e53fec0a4e231fdd6dc5c692cc7609525a7b2c9562d608f25d51an;
  var ED_GY = 0x6666666666666666666666666666666666666666666666666666666666666658n;

  function edMod(a) { a %= ED_P; return a >= 0n ? a : a + ED_P; }
  function edPow(a, e) {
    var r = 1n;
    a = edMod(a);
    while (e > 0n) {
      if (e & 1n) r = edMod(r * a);
      a = edMod(a * a);
      e >>= 1n;
    }
    return r;
  }
  function edInv(a) { return edPow(a, ED_P - 2n); }

  function edAdd(p, q) {
    var A = edMod((p.y - p.x) * (q.y - q.x));
    var B = edMod((p.y + p.x) * (q.y + q.x));
    var C = edMod(p.t * 2n * ED_D * q.t);
    var D = edMod(p.z * 2n * q.z);
    var E = edMod(B - A);
    var F = edMod(D - C);
    var G = edMod(D + C);
    var H = edMod(B + A);
    return { x: edMod(E * F), y: edMod(G * H), z: edMod(F * G), t: edMod(E * H) };
  }

  function edMul(k, p) {
    var r = { x: 0n, y: 1n, z: 1n, t: 0n };
    for (var i = 255; i >= 0; i--) {
      r = edAdd(r, r);
      if ((k >> BigInt(i)) & 1n) r = edAdd(r, p);
    }
    return r;
  }

  function ed25519PublicFromSeed(seed) {
    var h = sha512(seed);
    h[0] &= 248; h[31] &= 127; h[31] |= 64;
    var a = 0n, i;
    for (i = 31; i >= 0; i--) a = (a << 8n) | BigInt(h[i]);
    var G = { x: ED_GX, y: ED_GY, z: 1n, t: edMod(ED_GX * ED_GY) };
    var p = edMul(a, G);
    var zinv = edInv(p.z);
    var x = edMod(p.x * zinv);
    var y = edMod(p.y * zinv);
    var out = new Uint8Array(32);
    for (i = 0; i < 32; i++) out[i] = Number((y >> BigInt(8 * i)) & 0xffn);
    if (x & 1n) out[31] |= 0x80;
    return out;
  }

  function wordIndex(word, list) {
    var lo = 0, hi = list.length - 1;
    while (lo <= hi) {
      var mid = (lo + hi) >> 1;
      if (list[mid] === word) return mid;
      if (list[mid] < word) lo = mid + 1;
      else hi = mid - 1;
    }
    return -1;
  }

  async function mnemonicToMiniSecret(phrase, password, wordlist) {
    var words = phrase.trim().toLowerCase().split(/\s+/);
    if (words.length < 12 || words.length > 24 || words.length % 3) throw new Error('mnemonic');
    var bits = '';
    for (var i = 0; i < words.length; i++) {
      var idx = wordIndex(words[i], wordlist);
      if (idx < 0) throw new Error('mnemonic');
      bits += ('00000000000' + idx.toString(2)).slice(-11);
    }
    var checksumLen = words.length / 3;
    var entLen = bits.length - checksumLen;
    var entropy = new Uint8Array(entLen / 8);
    for (var e = 0; e < entropy.length; e++) entropy[e] = parseInt(bits.substr(e * 8, 8), 2);
    var digest = sha256(entropy);
    var got = '';
    for (var b = 0; b < digest.length && got.length < checksumLen; b++) {
      got += ('00000000' + digest[b].toString(2)).slice(-8);
    }
    if (got.slice(0, checksumLen) !== bits.slice(entLen)) throw new Error('mnemonic');
    var salt = new TextEncoder().encode('mnemonic' + (password || ''));
    return pbkdf2Sha512(entropy, salt, 2048, 32);
  }

  async function parseRoot(input, password, wordlist) {
    var t = (input || '').trim();
    var hex = hexToBytes(t);
    if (hex) return hex;
    return mnemonicToMiniSecret(t, password || '', wordlist);
  }

  async function deriveSensor(input, index, password, wordlist) {
    index = parseInt(index, 10);
    if (!(index >= 0) || index > 0xffffffff) throw new Error('index');
    wordlist = wordlist || BIP39_EN;
    var root = await parseRoot(input, password, wordlist);
    var seed = deriveHard(root, index);
    var pub = await ed25519PublicFromSeed(seed);
    return {
      hex: bytesToHex(seed),
      address: ss58Encode(pub, 32),
      path: '//' + index
    };
  }

  async function selfTest() {
    var seed = hexToBytes('9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60');
    var child = deriveHard(seed, 0);
    if (bytesToHex(child) !== 'ede3354e133f9c8e337ddd6ee5415ed4b4ffe5fc7d21e933f4930a3730e5b21c') {
      throw new Error('hdkd');
    }
    var pub = await ed25519PublicFromSeed(seed);
    if (bytesToHex(pub) !== 'd75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a') {
      throw new Error('ed25519');
    }
    if (ss58Encode(pub, 32) !== '4H4UUmHsKP4Meg7o5gRFBqG57ExhuKJVLEFYuATBHGbFzzi4') {
      throw new Error('ss58');
    }
    var mini = await mnemonicToMiniSecret(
      'bottom drive obey lake curtain smoke basket hold race lonely fit walk',
      '',
      BIP39_EN
    );
    if (bytesToHex(mini) !== 'fac7959dbfe72f052e5a0c3c8d6530f202b02fd8f9f5ca3580ec8deb7797479e') {
      throw new Error('pbkdf2');
    }
    var child0 = deriveHard(mini, 0);
    if (bytesToHex(child0) !== 'f8dfdb0f1103d9fb2905204ac32529d5f148761c4321b2865b0a40e15be75f57') {
      throw new Error('hdkd0');
    }
  }

  var BIP39_EN = "abandon ability able about above absent absorb abstract absurd abuse access accident account accuse achieve acid acoustic acquire across act action actor actress actual adapt add addict address adjust admit adult advance advice aerobic affair afford afraid again age agent agree ahead aim air airport aisle alarm album alcohol alert alien all alley allow almost alone alpha already also alter always amateur amazing among amount amused analyst anchor ancient anger angle angry animal ankle announce annual another answer antenna antique anxiety any apart apology appear apple approve april arch arctic area arena argue arm armed armor army around arrange arrest arrive arrow art artefact artist artwork ask aspect assault asset assist assume asthma athlete atom attack attend attitude attract auction audit august aunt author auto autumn average avocado avoid awake aware away awesome awful awkward axis baby bachelor bacon badge bag balance balcony ball bamboo banana banner bar barely bargain barrel base basic basket battle beach bean beauty because become beef before begin behave behind believe below belt bench benefit best betray better between beyond bicycle bid bike bind biology bird birth bitter black blade blame blanket blast bleak bless blind blood blossom blouse blue blur blush board boat body boil bomb bone bonus book boost border boring borrow boss bottom bounce box boy bracket brain brand brass brave bread breeze brick bridge brief bright bring brisk broccoli broken bronze broom brother brown brush bubble buddy budget buffalo build bulb bulk bullet bundle bunker burden burger burst bus business busy butter buyer buzz cabbage cabin cable cactus cage cake call calm camera camp can canal cancel candy cannon canoe canvas canyon capable capital captain car carbon card cargo carpet carry cart case cash casino castle casual cat catalog catch category cattle caught cause caution cave ceiling celery cement census century cereal certain chair chalk champion change chaos chapter charge chase chat cheap check cheese chef cherry chest chicken chief child chimney choice choose chronic chuckle chunk churn cigar cinnamon circle citizen city civil claim clap clarify claw clay clean clerk clever click client cliff climb clinic clip clock clog close cloth cloud clown club clump cluster clutch coach coast coconut code coffee coil coin collect color column combine come comfort comic common company concert conduct confirm congress connect consider control convince cook cool copper copy coral core corn correct cost cotton couch country couple course cousin cover coyote crack cradle craft cram crane crash crater crawl crazy cream credit creek crew cricket crime crisp critic crop cross crouch crowd crucial cruel cruise crumble crunch crush cry crystal cube culture cup cupboard curious current curtain curve cushion custom cute cycle dad damage damp dance danger daring dash daughter dawn day deal debate debris decade december decide decline decorate decrease deer defense define defy degree delay deliver demand demise denial dentist deny depart depend deposit depth deputy derive describe desert design desk despair destroy detail detect develop device devote diagram dial diamond diary dice diesel diet differ digital dignity dilemma dinner dinosaur direct dirt disagree discover disease dish dismiss disorder display distance divert divide divorce dizzy doctor document dog doll dolphin domain donate donkey donor door dose double dove draft dragon drama drastic draw dream dress drift drill drink drip drive drop drum dry duck dumb dune during dust dutch duty dwarf dynamic eager eagle early earn earth easily east easy echo ecology economy edge edit educate effort egg eight either elbow elder electric elegant element elephant elevator elite else embark embody embrace emerge emotion employ empower empty enable enact end endless endorse enemy energy enforce engage engine enhance enjoy enlist enough enrich enroll ensure enter entire entry envelope episode equal equip era erase erode erosion error erupt escape essay essence estate eternal ethics evidence evil evoke evolve exact example excess exchange excite exclude excuse execute exercise exhaust exhibit exile exist exit exotic expand expect expire explain expose express extend extra eye eyebrow fabric face faculty fade faint faith fall false fame family famous fan fancy fantasy farm fashion fat fatal father fatigue fault favorite feature february federal fee feed feel female fence festival fetch fever few fiber fiction field figure file film filter final find fine finger finish fire firm first fiscal fish fit fitness fix flag flame flash flat flavor flee flight flip float flock floor flower fluid flush fly foam focus fog foil fold follow food foot force forest forget fork fortune forum forward fossil foster found fox fragile frame frequent fresh friend fringe frog front frost frown frozen fruit fuel fun funny furnace fury future gadget gain galaxy gallery game gap garage garbage garden garlic garment gas gasp gate gather gauge gaze general genius genre gentle genuine gesture ghost giant gift giggle ginger giraffe girl give glad glance glare glass glide glimpse globe gloom glory glove glow glue goat goddess gold good goose gorilla gospel gossip govern gown grab grace grain grant grape grass gravity great green grid grief grit grocery group grow grunt guard guess guide guilt guitar gun gym habit hair half hammer hamster hand happy harbor hard harsh harvest hat have hawk hazard head health heart heavy hedgehog height hello helmet help hen hero hidden high hill hint hip hire history hobby hockey hold hole holiday hollow home honey hood hope horn horror horse hospital host hotel hour hover hub huge human humble humor hundred hungry hunt hurdle hurry hurt husband hybrid ice icon idea identify idle ignore ill illegal illness image imitate immense immune impact impose improve impulse inch include income increase index indicate indoor industry infant inflict inform inhale inherit initial inject injury inmate inner innocent input inquiry insane insect inside inspire install intact interest into invest invite involve iron island isolate issue item ivory jacket jaguar jar jazz jealous jeans jelly jewel job join joke journey joy judge juice jump jungle junior junk just kangaroo keen keep ketchup key kick kid kidney kind kingdom kiss kit kitchen kite kitten kiwi knee knife knock know lab label labor ladder lady lake lamp language laptop large later latin laugh laundry lava law lawn lawsuit layer lazy leader leaf learn leave lecture left leg legal legend leisure lemon lend length lens leopard lesson letter level liar liberty library license life lift light like limb limit link lion liquid list little live lizard load loan lobster local lock logic lonely long loop lottery loud lounge love loyal lucky luggage lumber lunar lunch luxury lyrics machine mad magic magnet maid mail main major make mammal man manage mandate mango mansion manual maple marble march margin marine market marriage mask mass master match material math matrix matter maximum maze meadow mean measure meat mechanic medal media melody melt member memory mention menu mercy merge merit merry mesh message metal method middle midnight milk million mimic mind minimum minor minute miracle mirror misery miss mistake mix mixed mixture mobile model modify mom moment monitor monkey monster month moon moral more morning mosquito mother motion motor mountain mouse move movie much muffin mule multiply muscle museum mushroom music must mutual myself mystery myth naive name napkin narrow nasty nation nature near neck need negative neglect neither nephew nerve nest net network neutral never news next nice night noble noise nominee noodle normal north nose notable note nothing notice novel now nuclear number nurse nut oak obey object oblige obscure observe obtain obvious occur ocean october odor off offer office often oil okay old olive olympic omit once one onion online only open opera opinion oppose option orange orbit orchard order ordinary organ orient original orphan ostrich other outdoor outer output outside oval oven over own owner oxygen oyster ozone pact paddle page pair palace palm panda panel panic panther paper parade parent park parrot party pass patch path patient patrol pattern pause pave payment peace peanut pear peasant pelican pen penalty pencil people pepper perfect permit person pet phone photo phrase physical piano picnic picture piece pig pigeon pill pilot pink pioneer pipe pistol pitch pizza place planet plastic plate play please pledge pluck plug plunge poem poet point polar pole police pond pony pool popular portion position possible post potato pottery poverty powder power practice praise predict prefer prepare present pretty prevent price pride primary print priority prison private prize problem process produce profit program project promote proof property prosper protect proud provide public pudding pull pulp pulse pumpkin punch pupil puppy purchase purity purpose purse push put puzzle pyramid quality quantum quarter question quick quit quiz quote rabbit raccoon race rack radar radio rail rain raise rally ramp ranch random range rapid rare rate rather raven raw razor ready real reason rebel rebuild recall receive recipe record recycle reduce reflect reform refuse region regret regular reject relax release relief rely remain remember remind remove render renew rent reopen repair repeat replace report require rescue resemble resist resource response result retire retreat return reunion reveal review reward rhythm rib ribbon rice rich ride ridge rifle right rigid ring riot ripple risk ritual rival river road roast robot robust rocket romance roof rookie room rose rotate rough round route royal rubber rude rug rule run runway rural sad saddle sadness safe sail salad salmon salon salt salute same sample sand satisfy satoshi sauce sausage save say scale scan scare scatter scene scheme school science scissors scorpion scout scrap screen script scrub sea search season seat second secret section security seed seek segment select sell seminar senior sense sentence series service session settle setup seven shadow shaft shallow share shed shell sheriff shield shift shine ship shiver shock shoe shoot shop short shoulder shove shrimp shrug shuffle shy sibling sick side siege sight sign silent silk silly silver similar simple since sing siren sister situate six size skate sketch ski skill skin skirt skull slab slam sleep slender slice slide slight slim slogan slot slow slush small smart smile smoke smooth snack snake snap sniff snow soap soccer social sock soda soft solar soldier solid solution solve someone song soon sorry sort soul sound soup source south space spare spatial spawn speak special speed spell spend sphere spice spider spike spin spirit split spoil sponsor spoon sport spot spray spread spring spy square squeeze squirrel stable stadium staff stage stairs stamp stand start state stay steak steel stem step stereo stick still sting stock stomach stone stool story stove strategy street strike strong struggle student stuff stumble style subject submit subway success such sudden suffer sugar suggest suit summer sun sunny sunset super supply supreme sure surface surge surprise surround survey suspect sustain swallow swamp swap swarm swear sweet swift swim swing switch sword symbol symptom syrup system table tackle tag tail talent talk tank tape target task taste tattoo taxi teach team tell ten tenant tennis tent term test text thank that theme then theory there they thing this thought three thrive throw thumb thunder ticket tide tiger tilt timber time tiny tip tired tissue title toast tobacco today toddler toe together toilet token tomato tomorrow tone tongue tonight tool tooth top topic topple torch tornado tortoise toss total tourist toward tower town toy track trade traffic tragic train transfer trap trash travel tray treat tree trend trial tribe trick trigger trim trip trophy trouble truck true truly trumpet trust truth try tube tuition tumble tuna tunnel turkey turn turtle twelve twenty twice twin twist two type typical ugly umbrella unable unaware uncle uncover under undo unfair unfold unhappy uniform unique unit universe unknown unlock until unusual unveil update upgrade uphold upon upper upset urban urge usage use used useful useless usual utility vacant vacuum vague valid valley valve van vanish vapor various vast vault vehicle velvet vendor venture venue verb verify version very vessel veteran viable vibrant vicious victory video view village vintage violin virtual virus visa visit visual vital vivid vocal voice void volcano volume vote voyage wage wagon wait walk wall walnut want warfare warm warrior wash wasp waste water wave way wealth weapon wear weasel weather web wedding weekend weird welcome west wet whale what wheat wheel when where whip whisper wide width wife wild will win window wine wing wink winner winter wire wisdom wise wish witness wolf woman wonder wood wool word work world worry worth wrap wreck wrestle wrist write wrong yard year yellow you young youth zebra zero zone zoo".split(" ");
  function bindPage() {
    var form = document.getElementById('provision-form');
    if (!form) return;
    var derived = null;
    var status = document.getElementById('provision-status');
    function msg(text, kind) {
      if (!status) return;
      status.hidden = !text;
      status.textContent = text || '';
      status.className = 'ui-notice' + (kind === 'ok' ? ' ui-notice--ok' : kind === 'err' ? ' ui-notice--err' : '');
    }
    fetch('/api/sensor-key').then(function (r) { return r.json(); }).then(function (j) {
      var el = document.getElementById('provision-current');
      if (el) el.textContent = j.address || '—';
    }).catch(function () {
      var el = document.getElementById('provision-current');
      if (el) el.textContent = '—';
    });
    AltruistProvision.selfTest().catch(function (e) {
      var extra = (e && e.message) ? ' (' + e.message + ')' : '';
      msg((form.getAttribute('data-need-browser') || '') + extra, 'err');
      var buttons = form.querySelectorAll('button');
      for (var i = 0; i < buttons.length; i++) buttons[i].disabled = true;
    });
    document.getElementById('provision-derive').addEventListener('click', function () {
      derived = null;
      document.getElementById('provision-install').disabled = true;
      AltruistProvision.deriveSensor(
        document.getElementById('provision-seed').value,
        document.getElementById('provision-index').value,
        document.getElementById('provision-password').value
      ).then(function (res) {
        derived = res;
        document.getElementById('provision-path').textContent = 'MASTER_SEED' + res.path;
        document.getElementById('provision-address').textContent = res.address;
        document.getElementById('provision-hex').textContent = res.hex;
        document.getElementById('provision-result').hidden = false;
        document.getElementById('provision-install').disabled = !document.getElementById('provision-ack').checked;
        msg('', '');
      }).catch(function (e) {
        var code = e && e.message;
        var attr = code === 'mnemonic' ? 'data-err-mnemonic' : (code === 'index' ? 'data-err-index' : 'data-err-crypto');
        msg(form.getAttribute(attr) || String(e), 'err');
        document.getElementById('provision-result').hidden = true;
      });
    });
    document.getElementById('provision-ack').addEventListener('change', function () {
      document.getElementById('provision-install').disabled = !derived || !document.getElementById('provision-ack').checked;
    });
    document.getElementById('provision-install').addEventListener('click', function () {
      if (!derived) return;
      if (!window.confirm(form.getAttribute('data-confirm-install') || '')) return;
      fetch('/api/sensor-key', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ private_key: derived.hex })
      }).then(function (r) { return r.json().then(function (j) { return { ok: r.ok, j: j }; }); })
        .then(function (pack) {
          if (!pack.ok || !pack.j.ok) throw new Error('install');
          if (pack.j.address && pack.j.address !== derived.address) {
            msg(form.getAttribute('data-mismatch') || '', 'err');
            return;
          }
          document.getElementById('provision-seed').value = '';
          msg(form.getAttribute('data-ok-install') || '', 'ok');
        }).catch(function () {
          msg(form.getAttribute('data-err-install') || '', 'err');
        });
    });
  }

  root.AltruistProvision = {
    deriveSensor: deriveSensor,
    selfTest: selfTest,
    bytesToHex: bytesToHex
  };
  if (typeof document !== 'undefined') {
    document.addEventListener('DOMContentLoaded', bindPage);
  }
})(typeof window !== 'undefined' ? window : globalThis);
)rawliteral";

#endif

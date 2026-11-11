/* SHA-256 / SHA-512 / PBKDF2 / Ed25519 public-from-seed for HTTP (no crypto.subtle). */
(function (root) {
  'use strict';

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
    var bitLen = msg.length * 8;
    var withPad = msg.length + 1 + 8;
    var blocks = Math.ceil(withPad / 64);
    var buf = new Uint8Array(blocks * 64);
    buf.set(msg);
    buf[msg.length] = 0x80;
    var dv = new DataView(buf.buffer);
    dv.setUint32(buf.length - 4, bitLen, false);
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

  function add64(ah, al, bh, bl) {
    var lo = (al >>> 0) + (bl >>> 0);
    var hi = (ah >>> 0) + (bh >>> 0) + ((lo / 0x100000000) >>> 0);
    return [hi >>> 0, lo >>> 0];
  }
  function rotr64(h, l, n) {
    if (n < 32) return [((h >>> n) | (l << (32 - n))) >>> 0, ((l >>> n) | (h << (32 - n))) >>> 0];
    n -= 32;
    return [((l >>> n) | (h << (32 - n))) >>> 0, ((h >>> n) | (l << (32 - n))) >>> 0];
  }
  function shr64(h, l, n) {
    if (n < 32) return [h >>> n, ((l >>> n) | (h << (32 - n))) >>> 0];
    return [0, h >>> (n - 32)];
  }
  function xor64(a, b) { return [a[0] ^ b[0], a[1] ^ b[1]]; }

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
    var bitLen = msg.length * 8;
    var blocks = Math.ceil((msg.length + 1 + 16) / 128);
    var buf = new Uint8Array(blocks * 128);
    buf.set(msg);
    buf[msg.length] = 0x80;
    var dv = new DataView(buf.buffer);
    dv.setUint32(buf.length - 4, bitLen, false);
    var wh = new Uint32Array(80), wl = new Uint32Array(80);
    for (var off = 0; off < buf.length; off += 128) {
      var i;
      for (i = 0; i < 16; i++) {
        wh[i] = dv.getUint32(off + i * 8, false);
        wl[i] = dv.getUint32(off + i * 8 + 4, false);
      }
      for (i = 16; i < 80; i++) {
        var s0 = xor64(xor64(rotr64(wh[i - 15], wl[i - 15], 1), rotr64(wh[i - 15], wl[i - 15], 8)), shr64(wh[i - 15], wl[i - 15], 7));
        var s1 = xor64(xor64(rotr64(wh[i - 2], wl[i - 2], 19), rotr64(wh[i - 2], wl[i - 2], 61)), shr64(wh[i - 2], wl[i - 2], 6));
        var s = add64(wh[i - 16], wl[i - 16], s0[0], s0[1]);
        s = add64(s[0], s[1], wh[i - 7], wl[i - 7]);
        s = add64(s[0], s[1], s1[0], s1[1]);
        wh[i] = s[0]; wl[i] = s[1];
      }
      var ah = H[0], al = H[1], bh = H[2], bl = H[3], ch = H[4], cl = H[5], dh = H[6], dl = H[7];
      var eh = H[8], el = H[9], fh = H[10], fl = H[11], gh = H[12], gl = H[13], hh = H[14], hl = H[15];
      for (i = 0; i < 80; i++) {
        var S1 = xor64(xor64(rotr64(eh, el, 14), rotr64(eh, el, 18)), rotr64(eh, el, 41));
        var chv = [(eh & fh) ^ ((~eh) & gh), (el & fl) ^ ((~el) & gl)];
        var t1 = add64(hh, hl, S1[0], S1[1]);
        t1 = add64(t1[0], t1[1], chv[0], chv[1]);
        t1 = add64(t1[0], t1[1], K[i * 2], K[i * 2 + 1]);
        t1 = add64(t1[0], t1[1], wh[i], wl[i]);
        var S0 = xor64(xor64(rotr64(ah, al, 28), rotr64(ah, al, 34)), rotr64(ah, al, 39));
        var maj = [(ah & bh) ^ (ah & ch) ^ (bh & ch), (al & bl) ^ (al & cl) ^ (bl & cl)];
        var t2 = add64(S0[0], S0[1], maj[0], maj[1]);
        hh = gh; hl = gl; gh = fh; gl = fl; fh = eh; fl = el;
        var ed = add64(dh, dl, t1[0], t1[1]); eh = ed[0]; el = ed[1];
        dh = ch; dl = cl; ch = bh; cl = bl; bh = ah; bl = al;
        var ad = add64(t1[0], t1[1], t2[0], t2[1]); ah = ad[0]; al = ad[1];
      }
      var r;
      r = add64(H[0], H[1], ah, al); H[0] = r[0]; H[1] = r[1];
      r = add64(H[2], H[3], bh, bl); H[2] = r[0]; H[3] = r[1];
      r = add64(H[4], H[5], ch, cl); H[4] = r[0]; H[5] = r[1];
      r = add64(H[6], H[7], dh, dl); H[6] = r[0]; H[7] = r[1];
      r = add64(H[8], H[9], eh, el); H[8] = r[0]; H[9] = r[1];
      r = add64(H[10], H[11], fh, fl); H[10] = r[0]; H[11] = r[1];
      r = add64(H[12], H[13], gh, gl); H[12] = r[0]; H[13] = r[1];
      r = add64(H[14], H[15], hh, hl); H[14] = r[0]; H[15] = r[1];
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
    for (var i = 1; i <= blocks; i++) {
      var block = new Uint8Array(salt.length + 4);
      block.set(salt);
      block[salt.length] = (i >>> 24) & 255;
      block[salt.length + 1] = (i >>> 16) & 255;
      block[salt.length + 2] = (i >>> 8) & 255;
      block[salt.length + 3] = i & 255;
      var u = hmacSha512(password, block);
      var t = u.slice();
      for (var j = 1; j < iter; j++) {
        u = hmacSha512(password, u);
        for (var x = 0; x < 64; x++) t[x] ^= u[x];
      }
      out.set(t, (i - 1) * 64);
    }
    return out.subarray(0, dkLen);
  }

  var P = (1n << 255n) - 19n;
  var D = 37095705934669439343138083508754565189542113879843219016388785533085940283555n;
  var GX = 15112221349535400772501151409588531511454012693041857206046113283949847762202n;
  var GY = 46316835694926478169428394003475163141307993866256225615783033603165251855960n;

  function mod(a) { a %= P; return a >= 0n ? a : a + P; }
  function powMod(a, e) {
    var r = 1n;
    a = mod(a);
    while (e > 0n) {
      if (e & 1n) r = mod(r * a);
      a = mod(a * a);
      e >>= 1n;
    }
    return r;
  }
  function inv(a) { return powMod(a, P - 2n); }

  function pointAdd(p, q) {
    var A = mod((p.y - p.x) * (q.y - q.x));
    var B = mod((p.y + p.x) * (q.y + q.x));
    var C = mod(p.t * 2n * D * q.t);
    var DD = mod(p.z * 2n * q.z);
    var E = mod(B - A);
    var F = mod(DD - C);
    var G = mod(DD + C);
    var H = mod(B + A);
    return { x: mod(E * F), y: mod(G * H), z: mod(F * G), t: mod(E * H) };
  }

  function pointDbl(p) { return pointAdd(p, p); }

  function pointMul(k, p) {
    var r = { x: 0n, y: 1n, z: 1n, t: 0n };
    for (var i = 255; i >= 0; i--) {
      r = pointDbl(r);
      if ((k >> BigInt(i)) & 1n) r = pointAdd(r, p);
    }
    return r;
  }

  function bytesLeToBig(b) {
    var n = 0n;
    for (var i = b.length - 1; i >= 0; i--) n = (n << 8n) | BigInt(b[i]);
    return n;
  }

  function ed25519PublicFromSeed(seed) {
    var h = sha512(seed);
    h[0] &= 248; h[31] &= 127; h[31] |= 64;
    var a = bytesLeToBig(h.subarray(0, 32));
    var G = { x: GX, y: GY, z: 1n, t: mod(GX * GY) };
    var p = pointMul(a, G);
    var zinv = inv(p.z);
    var x = mod(p.x * zinv);
    var y = mod(p.y * zinv);
    var out = new Uint8Array(32);
    for (var i = 0; i < 32; i++) out[i] = Number((y >> BigInt(8 * i)) & 0xffn);
    if (x & 1n) out[31] |= 0x80;
    return out;
  }

  root.AltruistHash = { sha256: sha256, sha512: sha512, pbkdf2Sha512: pbkdf2Sha512, ed25519PublicFromSeed: ed25519PublicFromSeed };
})(typeof window !== 'undefined' ? window : globalThis);

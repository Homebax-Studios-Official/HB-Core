// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "graphics/image.hpp"
#include <algorithm>
#include <cstring>

namespace HB {

namespace {

struct BitReader {
    const uint8_t* d;
    size_t n;
    size_t pos = 0;
    uint32_t bitBuf = 0;
    int bitCnt = 0;
    bool error = false;

    int bit() {
        if (bitCnt == 0) {
            if (pos >= n) { error = true; return 0; }
            bitBuf = d[pos++];
            bitCnt = 8;
        }
        int b = bitBuf & 1;
        bitBuf >>= 1;
        --bitCnt;
        return b;
    }
    uint32_t bits(int count) {
        uint32_t v = 0;
        for (int i = 0; i < count; ++i) v |= static_cast<uint32_t>(bit()) << i;
        return v;
    }
};

struct Huffman {
    uint16_t count[16] = {};
    uint16_t symbol[288] = {};
};

bool buildHuffman(Huffman& h, const uint8_t* lengths, int n) {
    std::memset(h.count, 0, sizeof(h.count));
    for (int i = 0; i < n; ++i) ++h.count[lengths[i]];
    if (h.count[0] == n) return true;  // no codes: allowed (incomplete)
    int left = 1;
    for (int len = 1; len < 16; ++len) {
        left <<= 1;
        left -= h.count[len];
        if (left < 0) return false;  // over-subscribed
    }
    uint16_t offs[16];
    offs[1] = 0;
    for (int len = 1; len < 15; ++len) offs[len + 1] = static_cast<uint16_t>(offs[len] + h.count[len]);
    for (int i = 0; i < n; ++i)
        if (lengths[i]) h.symbol[offs[lengths[i]]++] = static_cast<uint16_t>(i);
    return true;
}

int decodeSymbol(BitReader& br, const Huffman& h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len < 16; ++len) {
        code |= br.bit();
        if (br.error) return -1;
        int count = h.count[len];
        if (code - count < first) return h.symbol[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    return -1;
}

const uint16_t kLenBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const uint16_t kLenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const uint16_t kDistBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const uint16_t kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

bool inflateCodes(BitReader& br, const Huffman& lit, const Huffman& dist, std::vector<uint8_t>& out, size_t maxOut) {
    for (;;) {
        int sym = decodeSymbol(br, lit);
        if (sym < 0) return false;
        if (sym < 256) {
            if (out.size() >= maxOut) return false;
            out.push_back(static_cast<uint8_t>(sym));
        } else if (sym == 256) {
            return true;
        } else {
            sym -= 257;
            if (sym >= 29) return false;
            size_t len = kLenBase[sym] + br.bits(kLenExtra[sym]);
            int ds = decodeSymbol(br, dist);
            if (ds < 0 || ds >= 30) return false;
            size_t d = kDistBase[ds] + br.bits(kDistExtra[ds]);
            if (br.error || d > out.size() || out.size() + len > maxOut) return false;
            size_t from = out.size() - d;
            for (size_t i = 0; i < len; ++i) out.push_back(out[from + i]);
        }
    }
}

} // namespace

bool zlibInflate(const uint8_t* data, size_t size, std::vector<uint8_t>& out, size_t maxOut) {
    if (size < 6) return false;
    if ((data[0] & 0x0F) != 8 || ((data[0] << 8) | data[1]) % 31 != 0 || (data[1] & 0x20)) return false;
    BitReader br{data + 2, size - 2};
    out.clear();
    int last;
    do {
        last = br.bit();
        uint32_t type = br.bits(2);
        if (br.error) return false;
        if (type == 0) {
            br.bitCnt = 0;
            if (br.pos + 4 > br.n) return false;
            uint32_t len = br.d[br.pos] | (br.d[br.pos + 1] << 8);
            uint32_t nlen = br.d[br.pos + 2] | (br.d[br.pos + 3] << 8);
            br.pos += 4;
            if ((len ^ 0xFFFF) != nlen || br.pos + len > br.n || out.size() + len > maxOut) return false;
            out.insert(out.end(), br.d + br.pos, br.d + br.pos + len);
            br.pos += len;
        } else if (type == 1) {
            uint8_t lengths[288];
            int i = 0;
            for (; i < 144; ++i) lengths[i] = 8;
            for (; i < 256; ++i) lengths[i] = 9;
            for (; i < 280; ++i) lengths[i] = 7;
            for (; i < 288; ++i) lengths[i] = 8;
            Huffman lit, dist;
            buildHuffman(lit, lengths, 288);
            uint8_t dl[30];
            std::fill(dl, dl + 30, 5);
            buildHuffman(dist, dl, 30);
            if (!inflateCodes(br, lit, dist, out, maxOut)) return false;
        } else if (type == 2) {
            int nlen = static_cast<int>(br.bits(5)) + 257;
            int ndist = static_cast<int>(br.bits(5)) + 1;
            int ncode = static_cast<int>(br.bits(4)) + 4;
            if (nlen > 286 || ndist > 30 || br.error) return false;
            static const uint8_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
            uint8_t lengths[320] = {};
            for (int i = 0; i < ncode; ++i) lengths[order[i]] = static_cast<uint8_t>(br.bits(3));
            Huffman lencode;
            if (!buildHuffman(lencode, lengths, 19)) return false;
            int idx = 0;
            uint8_t all[320] = {};
            while (idx < nlen + ndist) {
                int sym = decodeSymbol(br, lencode);
                if (sym < 0) return false;
                if (sym < 16) all[idx++] = static_cast<uint8_t>(sym);
                else {
                    int rep, val = 0;
                    if (sym == 16) { if (idx == 0) return false; val = all[idx - 1]; rep = 3 + static_cast<int>(br.bits(2)); }
                    else if (sym == 17) rep = 3 + static_cast<int>(br.bits(3));
                    else rep = 11 + static_cast<int>(br.bits(7));
                    if (idx + rep > nlen + ndist) return false;
                    while (rep--) all[idx++] = static_cast<uint8_t>(val);
                }
            }
            Huffman lit, dist;
            if (all[256] == 0) return false;
            if (!buildHuffman(lit, all, nlen)) return false;
            if (!buildHuffman(dist, all + nlen, ndist)) return false;
            if (!inflateCodes(br, lit, dist, out, maxOut)) return false;
        } else {
            return false;
        }
    } while (!last);
    return true;
}

namespace {

uint32_t be32(const uint8_t* p) { return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3]; }

int paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

}  // namespace

std::optional<Image> decodePng(const uint8_t* d, size_t size) {
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (!d || size < 33 || std::memcmp(d, sig, 8) != 0) return std::nullopt;
    size_t pos = 8;
    uint32_t w = 0, h = 0;
    int depth = 0, ctype = 0, interlace = 0;
    bool haveHeader = false;
    std::vector<uint8_t> palette;      // RGB triplets
    std::vector<uint8_t> trnsPalette;  // alpha per palette entry
    bool hasTrnsKey = false;
    uint16_t trnsKey[3] = {};
    std::vector<uint8_t> idat;
    while (pos + 8 <= size) {
        uint32_t len = be32(d + pos);
        const uint8_t* type = d + pos + 4;
        if (len > size - pos - 8 || size - pos - 8 - len < 4) return std::nullopt;
        const uint8_t* body = d + pos + 8;
        if (!std::memcmp(type, "IHDR", 4)) {
            if (len != 13) return std::nullopt;
            w = be32(body); h = be32(body + 4);
            depth = body[8]; ctype = body[9]; interlace = body[12];
            if (body[10] != 0 || body[11] != 0 || interlace > 1) return std::nullopt;
            haveHeader = true;
        } else if (!std::memcmp(type, "PLTE", 4)) {
            if (len % 3 != 0 || len > 768) return std::nullopt;
            palette.assign(body, body + len);
        } else if (!std::memcmp(type, "tRNS", 4)) {
            if (ctype == 3) trnsPalette.assign(body, body + len);
            else if (ctype == 0 && len >= 2) { hasTrnsKey = true; trnsKey[0] = static_cast<uint16_t>((body[0] << 8) | body[1]); }
            else if (ctype == 2 && len >= 6) { hasTrnsKey = true; for (int i = 0; i < 3; ++i) trnsKey[i] = static_cast<uint16_t>((body[2 * i] << 8) | body[2 * i + 1]); }
        } else if (!std::memcmp(type, "IDAT", 4)) {
            idat.insert(idat.end(), body, body + len);
        } else if (!std::memcmp(type, "IEND", 4)) {
            break;
        }
        pos += 12 + len;
    }
    if (!haveHeader || w == 0 || h == 0 || w > 16384 || h > 16384 || uint64_t(w) * h > 64ull * 1000 * 1000) return std::nullopt;
    int channels;
    switch (ctype) {
        case 0: channels = 1; if (depth != 1 && depth != 2 && depth != 4 && depth != 8 && depth != 16) return std::nullopt; break;
        case 2: channels = 3; if (depth != 8 && depth != 16) return std::nullopt; break;
        case 3: channels = 1; if (depth != 1 && depth != 2 && depth != 4 && depth != 8) return std::nullopt; if (palette.empty()) return std::nullopt; break;
        case 4: channels = 2; if (depth != 8 && depth != 16) return std::nullopt; break;
        case 6: channels = 4; if (depth != 8 && depth != 16) return std::nullopt; break;
        default: return std::nullopt;
    }
    const int bitsPerPixel = channels * depth;
    const int bpp = std::max(1, bitsPerPixel / 8);

    struct Pass { uint32_t x0, y0, dx, dy; };
    static const Pass adam7[7] = {{0, 0, 8, 8}, {4, 0, 8, 8}, {0, 4, 4, 8}, {2, 0, 4, 4}, {0, 2, 2, 4}, {1, 0, 2, 2}, {0, 1, 1, 2}};
    static const Pass single[1] = {{0, 0, 1, 1}};
    const Pass* passes = interlace ? adam7 : single;
    const int passCount = interlace ? 7 : 1;

    // Compute expected raw size.
    size_t expected = 0;
    for (int p = 0; p < passCount; ++p) {
        uint32_t pw = (w - passes[p].x0 + passes[p].dx - 1) / passes[p].dx;
        uint32_t ph = (h - passes[p].y0 + passes[p].dy - 1) / passes[p].dy;
        if (w <= passes[p].x0 || h <= passes[p].y0) continue;
        expected += size_t(ph) * (1 + (size_t(pw) * bitsPerPixel + 7) / 8);
    }
    std::vector<uint8_t> raw;
    if (!zlibInflate(idat.data(), idat.size(), raw, expected) || raw.size() != expected) return std::nullopt;

    Image img;
    img.width = static_cast<int>(w);
    img.height = static_cast<int>(h);
    img.rgba.assign(size_t(w) * h * 4, 0);

    size_t rp = 0;
    for (int p = 0; p < passCount; ++p) {
        if (w <= passes[p].x0 || h <= passes[p].y0) continue;
        uint32_t pw = (w - passes[p].x0 + passes[p].dx - 1) / passes[p].dx;
        uint32_t ph = (h - passes[p].y0 + passes[p].dy - 1) / passes[p].dy;
        size_t rowBytes = (size_t(pw) * bitsPerPixel + 7) / 8;
        std::vector<uint8_t> prev(rowBytes, 0), cur(rowBytes);
        for (uint32_t y = 0; y < ph; ++y) {
            int filter = raw[rp++];
            std::memcpy(cur.data(), &raw[rp], rowBytes);
            rp += rowBytes;
            for (size_t i = 0; i < rowBytes; ++i) {
                int a = i >= static_cast<size_t>(bpp) ? cur[i - bpp] : 0;
                int b = prev[i];
                int c = i >= static_cast<size_t>(bpp) ? prev[i - bpp] : 0;
                switch (filter) {
                    case 0: break;
                    case 1: cur[i] = static_cast<uint8_t>(cur[i] + a); break;
                    case 2: cur[i] = static_cast<uint8_t>(cur[i] + b); break;
                    case 3: cur[i] = static_cast<uint8_t>(cur[i] + ((a + b) >> 1)); break;
                    case 4: cur[i] = static_cast<uint8_t>(cur[i] + paeth(a, b, c)); break;
                    default: return std::nullopt;
                }
            }
            for (uint32_t x = 0; x < pw; ++x) {
                auto sample = [&](int ch) -> uint16_t {
                    if (depth == 16) { size_t o = (size_t(x) * channels + ch) * 2; return static_cast<uint16_t>((cur[o] << 8) | cur[o + 1]); }
                    if (depth == 8) return cur[size_t(x) * channels + ch];
                    size_t bitPos = size_t(x) * depth;
                    return static_cast<uint16_t>((cur[bitPos >> 3] >> (8 - depth - (bitPos & 7))) & ((1 << depth) - 1));
                };
                auto to8 = [&](uint16_t v) -> uint8_t {
                    if (depth == 16) return static_cast<uint8_t>(v >> 8);
                    if (depth == 8) return static_cast<uint8_t>(v);
                    return static_cast<uint8_t>(v * 255 / ((1 << depth) - 1));
                };
                uint8_t r, g, b, al = 255;
                switch (ctype) {
                    case 0: { uint16_t v = sample(0); r = g = b = to8(v); if (hasTrnsKey && v == trnsKey[0]) al = 0; break; }
                    case 2: { uint16_t rr = sample(0), gg = sample(1), bb = sample(2); r = to8(rr); g = to8(gg); b = to8(bb); if (hasTrnsKey && rr == trnsKey[0] && gg == trnsKey[1] && bb == trnsKey[2]) al = 0; break; }
                    case 3: {
                        uint16_t idx = sample(0);
                        if (size_t(idx) * 3 + 2 >= palette.size()) { r = g = b = 0; }
                        else { r = palette[idx * 3]; g = palette[idx * 3 + 1]; b = palette[idx * 3 + 2]; }
                        if (idx < trnsPalette.size()) al = trnsPalette[idx];
                        break;
                    }
                    case 4: r = g = b = to8(sample(0)); al = to8(sample(1)); break;
                    default: r = to8(sample(0)); g = to8(sample(1)); b = to8(sample(2)); al = to8(sample(3)); break;
                }
                uint32_t px = passes[p].x0 + x * passes[p].dx, py = passes[p].y0 + y * passes[p].dy;
                uint8_t* o = &img.rgba[(size_t(py) * w + px) * 4];
                o[0] = r; o[1] = g; o[2] = b; o[3] = al;
            }
            prev = cur;
        }
    }
    return img;
}

std::optional<Image> decodeImage(const uint8_t* data, size_t size) { return decodePng(data, size); }

} // namespace HB

// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "graphics/truetype.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace HB {

namespace {

struct Reader {
    const std::vector<uint8_t>& d;
    bool u8(size_t o, uint8_t& v) const { if (o >= d.size()) return false; v = d[o]; return true; }
    uint16_t u16(size_t o) const { return o + 2 <= d.size() ? static_cast<uint16_t>((d[o] << 8) | d[o + 1]) : 0; }
    int16_t i16(size_t o) const { return static_cast<int16_t>(u16(o)); }
    uint32_t u32(size_t o) const { return o + 4 <= d.size() ? (uint32_t(d[o]) << 24) | (uint32_t(d[o + 1]) << 16) | (uint32_t(d[o + 2]) << 8) | d[o + 3] : 0; }
};

}  // namespace

std::unique_ptr<TrueTypeFont> TrueTypeFont::load(std::vector<uint8_t> data) {
    std::unique_ptr<TrueTypeFont> f(new TrueTypeFont());
    f->d_ = std::move(data);
    if (!f->parse()) return nullptr;
    return f;
}

bool TrueTypeFont::parse() {
    Reader r{d_};
    if (d_.size() < 12) return false;
    uint32_t base = 0;
    uint32_t tag = r.u32(0);
    if (tag == 0x74746366) base = r.u32(12);  // 'ttcf': first font
    tag = r.u32(base);
    if (tag != 0x00010000 && tag != 0x74727565) return false;  // not TrueType outlines
    int numTables = r.u16(base + 4);
    uint32_t head = 0, maxp = 0, hhea = 0;
    for (int i = 0; i < numTables; ++i) {
        size_t rec = base + 12 + size_t(i) * 16;
        uint32_t t = r.u32(rec), off = r.u32(rec + 8), len = r.u32(rec + 12);
        if (off > d_.size() || len > d_.size() - off) continue;
        switch (t) {
            case 0x68656164: head = off; break;
            case 0x6D617870: maxp = off; break;
            case 0x68686561: hhea = off; break;
            case 0x686D7478: hmtx_ = off; break;
            case 0x6C6F6361: loca_ = off; break;
            case 0x676C7966: glyf_ = off; glyfLen_ = len; break;
            case 0x636D6170: cmap_ = off; break;
            default: break;
        }
    }
    if (!head || !maxp || !hhea || !hmtx_ || !loca_ || !glyf_ || !cmap_) return false;
    unitsPerEm_ = r.u16(head + 18);
    if (unitsPerEm_ < 16 || unitsPerEm_ > 16384) return false;
    longLoca_ = r.i16(head + 50) != 0;
    numGlyphs_ = r.u16(maxp + 4);
    ascent_ = static_cast<float>(r.i16(hhea + 4));
    descent_ = static_cast<float>(-r.i16(hhea + 6));
    if (ascent_ <= 0 || ascent_ + descent_ <= 0) { ascent_ = unitsPerEm_ * 0.8f; descent_ = unitsPerEm_ * 0.2f; }
    numHMetrics_ = r.u16(hhea + 34);
    if (numHMetrics_ == 0 || numGlyphs_ == 0) return false;

    int nSub = r.u16(cmap_ + 2);
    uint32_t best = 0; int bestScore = -1;
    for (int i = 0; i < nSub; ++i) {
        uint16_t pid = r.u16(cmap_ + 4 + size_t(i) * 8), eid = r.u16(cmap_ + 6 + size_t(i) * 8);
        uint32_t off = r.u32(cmap_ + 8 + size_t(i) * 8);
        int fmt = r.u16(cmap_ + off);
        if (fmt != 4 && fmt != 12) continue;
        int score = 0;
        if (pid == 3 && eid == 10) score = 4;
        else if (pid == 3 && eid == 1) score = 3;
        else if (pid == 0) score = 2;
        else continue;
        if (score > bestScore) { bestScore = score; best = cmap_ + off; cmapFormat_ = fmt; }
    }
    if (bestScore < 0) return false;
    cmapSub_ = best;
    return true;
}

uint16_t TrueTypeFont::glyphIndex(char32_t cp) const {
    Reader r{d_};
    if (cmapFormat_ == 4) {
        if (cp > 0xFFFF) return 0;
        int segX2 = r.u16(cmapSub_ + 6);
        int seg = segX2 / 2;
        size_t endCodes = cmapSub_ + 14;
        size_t startCodes = endCodes + segX2 + 2;
        size_t deltas = startCodes + segX2;
        size_t rangeOffs = deltas + segX2;
        int lo = 0, hi = seg - 1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            uint16_t end = r.u16(endCodes + mid * 2);
            if (cp > end) { lo = mid + 1; continue; }
            uint16_t start = r.u16(startCodes + mid * 2);
            if (cp < start) { hi = mid - 1; continue; }
            uint16_t ro = r.u16(rangeOffs + mid * 2);
            int16_t delta = r.i16(deltas + mid * 2);
            if (ro == 0) return static_cast<uint16_t>((cp + delta) & 0xFFFF);
            size_t gp = rangeOffs + mid * 2 + ro + (cp - start) * 2;
            uint16_t g = r.u16(gp);
            return g ? static_cast<uint16_t>((g + delta) & 0xFFFF) : 0;
        }
        return 0;
    }
    uint32_t n = r.u32(cmapSub_ + 12);
    size_t groups = cmapSub_ + 16;
    uint32_t lo = 0, hi = n;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        uint32_t s = r.u32(groups + size_t(mid) * 12), e = r.u32(groups + size_t(mid) * 12 + 4);
        if (cp < s) hi = mid;
        else if (cp > e) lo = mid + 1;
        else return static_cast<uint16_t>(r.u32(groups + size_t(mid) * 12 + 8) + (cp - s));
    }
    return 0;
}

float TrueTypeFont::advance(uint16_t glyph) const {
    Reader r{d_};
    int idx = std::min<int>(glyph, numHMetrics_ - 1);
    return static_cast<float>(r.u16(hmtx_ + size_t(idx) * 4));
}

bool TrueTypeFont::loadGlyph(uint16_t glyph, std::vector<Contour>& out, int depth) const {
    if (depth > 4 || glyph >= numGlyphs_) return false;
    Reader r{d_};
    uint32_t off0, off1;
    if (longLoca_) { off0 = r.u32(loca_ + size_t(glyph) * 4); off1 = r.u32(loca_ + size_t(glyph) * 4 + 4); }
    else { off0 = r.u16(loca_ + size_t(glyph) * 2) * 2u; off1 = r.u16(loca_ + size_t(glyph) * 2 + 2) * 2u; }
    if (off1 <= off0) return true;  // empty glyph (space)
    if (off1 > glyfLen_ || off0 + 10 > glyfLen_) return false;
    size_t g = glyf_ + off0;
    int nContours = r.i16(g);
    size_t p = g + 10;
    if (nContours >= 0) {
        std::vector<uint16_t> ends(nContours);
        for (int i = 0; i < nContours; ++i) { ends[i] = r.u16(p); p += 2; }
        int nPoints = nContours ? ends.back() + 1 : 0;
        if (nPoints > 20000) return false;
        int insLen = r.u16(p);
        p += 2 + insLen;
        std::vector<uint8_t> flags(nPoints);
        for (int i = 0; i < nPoints;) {
            uint8_t fl;
            if (!r.u8(p++, fl)) return false;
            flags[i++] = fl;
            if (fl & 8) {
                uint8_t rep;
                if (!r.u8(p++, rep)) return false;
                while (rep-- && i < nPoints) flags[i++] = fl;
            }
        }
        std::vector<float> xs(nPoints), ys(nPoints);
        int v = 0;
        for (int i = 0; i < nPoints; ++i) {
            uint8_t fl = flags[i];
            if (fl & 2) { uint8_t dx; if (!r.u8(p++, dx)) return false; v += (fl & 16) ? dx : -int(dx); }
            else if (!(fl & 16)) { v += r.i16(p); p += 2; }
            xs[i] = static_cast<float>(v);
        }
        v = 0;
        for (int i = 0; i < nPoints; ++i) {
            uint8_t fl = flags[i];
            if (fl & 4) { uint8_t dy; if (!r.u8(p++, dy)) return false; v += (fl & 32) ? dy : -int(dy); }
            else if (!(fl & 32)) { v += r.i16(p); p += 2; }
            ys[i] = static_cast<float>(v);
        }
        int start = 0;
        for (int c = 0; c < nContours; ++c) {
            if (ends[c] < start || ends[c] >= nPoints) return false;
            Contour ct;
            for (int i = start; i <= ends[c]; ++i) ct.push_back({xs[i], ys[i], (flags[i] & 1) != 0});
            out.push_back(std::move(ct));
            start = ends[c] + 1;
        }
        return true;
    }
    // Composite glyph
    for (int guard = 0; guard < 64; ++guard) {
        uint16_t flags = r.u16(p), gi = r.u16(p + 2);
        p += 4;
        float dx = 0, dy = 0;
        if (flags & 1) { if (flags & 2) { dx = r.i16(p); dy = r.i16(p + 2); } p += 4; }
        else { if (flags & 2) { dx = static_cast<int8_t>(d_[p]); dy = static_cast<int8_t>(d_[p + 1]); } p += 2; }
        float a = 1, b = 0, c = 0, dd = 1;
        if (flags & 8) { a = dd = r.i16(p) / 16384.0f; p += 2; }
        else if (flags & 0x40) { a = r.i16(p) / 16384.0f; dd = r.i16(p + 2) / 16384.0f; p += 4; }
        else if (flags & 0x80) { a = r.i16(p) / 16384.0f; b = r.i16(p + 2) / 16384.0f; c = r.i16(p + 4) / 16384.0f; dd = r.i16(p + 6) / 16384.0f; p += 8; }
        std::vector<Contour> sub;
        if (!loadGlyph(gi, sub, depth + 1)) return false;
        for (auto& ct : sub) {
            for (auto& pt : ct) {
                float nx = a * pt.x + c * pt.y + dx;
                float ny = b * pt.x + dd * pt.y + dy;
                pt.x = nx; pt.y = ny;
            }
            out.push_back(std::move(ct));
        }
        if (!(flags & 0x20)) break;
    }
    return true;
}

bool TrueTypeFont::outline(uint16_t glyph, std::vector<Contour>& out) const {
    out.clear();
    return loadGlyph(glyph, out, 0);
}

namespace {

struct Edge { float x0, y0, x1, y1; int dir; };

void flattenContour(const Contour& c, float scale, float shear, std::vector<std::pair<float, float>>& poly) {
    // Points in font units; convert to pixel space (y down) while flattening quadratic curves.
    size_t n = c.size();
    if (n < 2) return;
    auto pt = [&](size_t i) { return std::make_pair((c[i].x + shear * c[i].y) * scale, -c[i].y * scale); };
    // find a starting on-curve point
    size_t start = 0;
    bool found = false;
    for (size_t i = 0; i < n; ++i) if (c[i].onCurve) { start = i; found = true; break; }
    std::pair<float, float> startPt;
    size_t first;
    if (found) { startPt = pt(start); first = start + 1; }
    else { auto a = pt(0), b = pt(n - 1); startPt = {(a.first + b.first) / 2, (a.second + b.second) / 2}; first = 0; }
    poly.push_back(startPt);
    std::pair<float, float> cur = startPt;
    bool haveCtrl = false;
    std::pair<float, float> ctrl{};
    size_t count = found ? n - 1 : n;
    auto quad = [&](std::pair<float, float> p0, std::pair<float, float> p1, std::pair<float, float> p2) {
        float len = std::fabs(p0.first - p2.first) + std::fabs(p0.second - p2.second) + std::fabs(p1.first - p0.first) + std::fabs(p1.second - p0.second);
        int steps = std::clamp(static_cast<int>(std::ceil(std::sqrt(len) * 1.5f)), 2, 24);
        for (int s = 1; s <= steps; ++s) {
            float t = static_cast<float>(s) / steps, u = 1 - t;
            poly.push_back({u * u * p0.first + 2 * u * t * p1.first + t * t * p2.first, u * u * p0.second + 2 * u * t * p1.second + t * t * p2.second});
        }
    };
    for (size_t k = 0; k < count; ++k) {
        size_t i = (first + k) % n;
        auto p = pt(i);
        if (c[i].onCurve) {
            if (haveCtrl) { quad(cur, ctrl, p); haveCtrl = false; }
            else poly.push_back(p);
            cur = p;
        } else {
            if (haveCtrl) {
                std::pair<float, float> mid{(ctrl.first + p.first) / 2, (ctrl.second + p.second) / 2};
                quad(cur, ctrl, mid);
                cur = mid;
            }
            ctrl = p;
            haveCtrl = true;
        }
    }
    if (haveCtrl) quad(cur, ctrl, startPt);
}

}  // namespace

GlyphBitmap TrueTypeFont::rasterize(uint16_t glyph, float pixelSize, float shear, bool embolden) const {
    GlyphBitmap bm;
    std::vector<Contour> contours;
    if (!outline(glyph, contours) || contours.empty()) return bm;
    float scale = pixelSize / static_cast<float>(unitsPerEm_);
    std::vector<Edge> edges;
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (const auto& c : contours) {
        std::vector<std::pair<float, float>> poly;
        flattenContour(c, scale, shear, poly);
        for (size_t i = 0; i < poly.size(); ++i) {
            auto a = poly[i], b = poly[(i + 1) % poly.size()];
            minX = std::min({minX, a.first, b.first}); maxX = std::max({maxX, a.first, b.first});
            minY = std::min({minY, a.second, b.second}); maxY = std::max({maxY, a.second, b.second});
            if (a.second == b.second) continue;
            if (a.second < b.second) edges.push_back({a.first, a.second, b.first, b.second, 1});
            else edges.push_back({b.first, b.second, a.first, a.second, -1});
        }
    }
    if (edges.empty()) return bm;
    int x0 = static_cast<int>(std::floor(minX)) - 1, x1 = static_cast<int>(std::ceil(maxX)) + 2;
    int y0 = static_cast<int>(std::floor(minY)) - 1, y1 = static_cast<int>(std::ceil(maxY)) + 1;
    int w = x1 - x0, h = y1 - y0;
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return bm;
    std::vector<float> acc(size_t(w) * h, 0.0f);
    const int SUB = 4;
    std::vector<std::pair<float, int>> xs;
    for (int row = 0; row < h; ++row) {
        for (int s = 0; s < SUB; ++s) {
            float sy = y0 + row + (s + 0.5f) / SUB;
            xs.clear();
            for (const auto& e : edges) {
                if (sy < e.y0 || sy >= e.y1) continue;
                float t = (sy - e.y0) / (e.y1 - e.y0);
                xs.push_back({e.x0 + t * (e.x1 - e.x0), e.dir});
            }
            std::sort(xs.begin(), xs.end());
            int wind = 0;
            for (size_t i = 0; i + 1 < xs.size(); ++i) {
                wind += xs[i].second;
                if (wind == 0) continue;
                float a = xs[i].first - x0, b = xs[i + 1].first - x0;
                if (b <= a) continue;
                int ia = static_cast<int>(std::floor(a)), ib = static_cast<int>(std::floor(b));
                ia = std::max(ia, 0); ib = std::min(ib, w - 1);
                for (int px = ia; px <= ib; ++px) {
                    float l = std::max(a, static_cast<float>(px)), rgt = std::min(b, static_cast<float>(px + 1));
                    if (rgt > l) acc[size_t(row) * w + px] += (rgt - l) / SUB;
                }
            }
        }
    }
    bm.left = x0; bm.top = y0; bm.width = w; bm.height = h;
    bm.alpha.resize(acc.size());
    for (size_t i = 0; i < acc.size(); ++i) bm.alpha[i] = static_cast<uint8_t>(std::lround(std::min(1.0f, acc[i]) * 255));
    if (embolden) {
        // Smear 1 pixel to the right (weight boost for faces without a bold file).
        for (int row = 0; row < h; ++row)
            for (int px = w - 1; px > 0; --px) {
                uint8_t& cur = bm.alpha[size_t(row) * w + px];
                uint8_t prev = bm.alpha[size_t(row) * w + px - 1];
                cur = static_cast<uint8_t>(std::min(255, cur + prev * 6 / 10));
            }
    }
    return bm;
}

} // namespace HB

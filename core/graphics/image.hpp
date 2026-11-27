// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace HB {

struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgba;  // straight alpha, width*height*4 bytes
};

// Own PNG decoder (own inflate implementation). Supports all color types, bit depths 1-16, tRNS,
// Adam7 interlacing. Chunk CRCs are not verified. Rejects images larger than 16384 px per side or
// 64 Mpx. Returns nullopt on any malformed input; never reads out of bounds.
std::optional<Image> decodePng(const uint8_t* data, size_t size);

// Format sniffing: only PNG is decoded in 0.2.0 (JPEG/GIF/WebP are not supported yet).
std::optional<Image> decodeImage(const uint8_t* data, size_t size);

// Raw zlib stream inflate (exposed for tests). Returns false on error or if output exceeds maxOut.
bool zlibInflate(const uint8_t* data, size_t size, std::vector<uint8_t>& out, size_t maxOut);

} // namespace HB

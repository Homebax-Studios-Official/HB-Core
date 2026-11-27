// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace HB {

std::string toLowerAscii(std::string_view s);
bool equalsIgnoreCase(std::string_view a, std::string_view b);
std::string_view trim(std::string_view s);
bool startsWith(std::string_view s, std::string_view prefix);
bool endsWith(std::string_view s, std::string_view suffix);
std::vector<std::string> splitWhitespace(std::string_view s);
bool isAsciiSpace(char c);
bool isAsciiAlpha(char c);
bool isAsciiDigit(char c);
bool isAsciiAlnum(char c);

// UTF-8. Decoding never reads past the end; malformed bytes yield U+FFFD and consume one byte.
char32_t decodeUtf8(std::string_view s, size_t& pos);
std::u32string utf8ToUtf32(std::string_view s);
void appendUtf8(std::string& out, char32_t cp);

// Decodes standard base64 (whitespace ignored, padding optional). ok=false on invalid characters.
std::string base64Decode(std::string_view in, bool* ok = nullptr);

} // namespace HB

// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "base/result.hpp"
namespace HB {
const char* errorCodeName(ErrorCode c) {
    switch (c) {
        case ErrorCode::Ok: return "Ok";
        case ErrorCode::InvalidArgument: return "InvalidArgument";
        case ErrorCode::NotFound: return "NotFound";
        case ErrorCode::IoError: return "IoError";
        case ErrorCode::ParseError: return "ParseError";
        case ErrorCode::Unsupported: return "Unsupported";
        case ErrorCode::Internal: return "Internal";
    }
    return "Unknown";
}
} // namespace HB

// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#ifndef HBCORE_HB_ERROR_H
#define HBCORE_HB_ERROR_H

#include <string>

namespace HB {

enum class ErrorCode { Ok = 0, InvalidArgument, NotFound, IoError, ParseError, Unsupported, Internal };

struct Error {
    ErrorCode code = ErrorCode::Ok;
    std::string message;
    bool ok() const { return code == ErrorCode::Ok; }
};

} // namespace HB

#endif

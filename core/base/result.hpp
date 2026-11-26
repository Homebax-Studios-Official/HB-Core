// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <string>
#include <utility>
#include <variant>
#include <cstdlib>
#include "hbcore/hb_error.h"

namespace HB {

struct Unit {};

// Value-or-error type. The engine does not throw across module boundaries.
template <class T>
class Result {
public:
    Result(T value) : v_(std::move(value)) {}
    Result(Error error) : v_(std::move(error)) {}

    bool ok() const { return v_.index() == 0; }
    explicit operator bool() const { return ok(); }

    T& value() { if (!ok()) std::abort(); return std::get<0>(v_); }
    const T& value() const { if (!ok()) std::abort(); return std::get<0>(v_); }
    const Error& error() const { if (ok()) std::abort(); return std::get<1>(v_); }

private:
    std::variant<T, Error> v_;
};

using Status = Result<Unit>;

inline Status okStatus() { return Status(Unit{}); }
inline Error makeError(ErrorCode c, std::string msg) { return Error{c, std::move(msg)}; }
const char* errorCodeName(ErrorCode c);

} // namespace HB

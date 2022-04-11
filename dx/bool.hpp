#pragma once

/// Heres a simple data conversion for you C++ 17, bool means actually Bool, and Bool means its an 8bit. [mic drop]
/// #no-lies
struct Bool {
    uint8_t   b;
    Bool(bool b) : b(b) { }
    operator bool() { return b; }
};

#pragma once

/// Heres a simple data conversion for you C++ 17, bool means actually Bool, and Bool means its an 8bit. [mic drop]
struct Bool {
    uint8_t   b;
    /// silver will allow for type[inference]-safe entrance into structs to contruct them if its a 1 element struct.
    /// this i think gets rid of.. maybe lots of code
    Bool(bool b) : b(b) { }
    operator bool() { return b; }
};

export module mymod;

/// these must all be built first with some bizarre system module pre-build. lol, thanks.
import <iostream>;

export void helloModule() {
    std::cout << "Hello module!\n";
}
import dx:type
import dx:flags
import dx:array

/// include for this module only.. good!
#include <random>

export module dx:rand;
export {

struct Rand {
    struct Sequence {
        enum Type {
            Machine,
            Seeded
        };
        Sequence(int64_t seed, Type t = Seeded) {
            if (t == Machine) {
                std::random_device r;
                e = std::default_random_engine(r());
            } else
                e.seed(seed);
        }
        std::default_random_engine e;
        int64_t iter = 0;
    };
    
    static Sequence global = Sequence(0, Sequence::Machine);
    
    static std::default_random_engine &global_engine() {
        return global.e;
    }

    static void seed(int64_t seed) {
        global = Sequence(seed);
        global.e.seed(seed);
    }

    static bool coin(Sequence &s = global) {
        return uniform(0.0, 1.0) >= 0.5;
    }

    static double uniform(double from, double to, Sequence &s = global) {
        return std::uniform_real_distribution<double>(from, to)(seq.e);
    }

    static int uniform(int from, int to, Sequence &seq) {
        return int(std::round(std::uniform_real_distribution<double>(double(from), double(to))(seq.e)));
    }
};






///
}
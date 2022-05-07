export module rand;

import io;

export struct Rand {
    struct Sequence {
        enum Type {
            Machine,
            Seeded
        };
        inline Sequence(int64_t seed, Type t = Seeded) {
            if (t == Machine) {
                std::random_device r;
                e = std::default_random_engine(r());
            } else
                e.seed(uint32_t(seed)); // windows doesnt have the same seeding 
        }
        std::default_random_engine e;
        int64_t iter = 0;
    };
    
    static inline Sequence global = Sequence(0, Sequence::Machine);
    
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
        return std::uniform_real_distribution<double>(from, to)(s.e);
    }

    static int uniform(int from, int to, Sequence &s) {
        return int(std::round(std::uniform_real_distribution<double>(double(from), double(to))(s.e)));
    }
};

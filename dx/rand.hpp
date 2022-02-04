#pragma once
#include <random>

struct Rand {
    struct Sequence {
        enum Type {
            Machine,
            Seeded
        };
        Sequence(int64_t seed, Type t = Seeded);
        std::default_random_engine e;
        int64_t iter = 0;
    };
    
    static Sequence global;
    static double uniform(double from, double to, Sequence &s = global);
    static std::default_random_engine &global_engine();
    static int    uniform(int from, int to, Sequence &s = global);
    static void      seed(int64_t seed); // once called, this resets global as seeded
};

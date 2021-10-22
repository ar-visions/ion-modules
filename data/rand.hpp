#pragma once
#include <random>

struct Rand {
    static std::default_random_engine e;
    static double uniform(double from, double to);
    static int    uniform(int from, int to);
    static void      seed(int seed);
};

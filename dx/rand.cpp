#include <dx/rand.hpp>

std::default_random_engine Rand::e;

double Rand::uniform(double from, double to) {
    return std::uniform_real_distribution<double>(from, to)(e);
}

int Rand::uniform(int from, int to) {
    return int(std::round(std::uniform_real_distribution<double>(double(from), double(to))(e)));
}

void Rand::seed(int seed) {
    e.seed(seed);
}

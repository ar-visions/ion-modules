#include <dx/dx.hpp>

Rand::Sequence Rand::global = Rand::Sequence(0, Rand::Sequence::Machine);

Rand::Sequence::Sequence(int64_t seed, Rand::Sequence::Type t) {
    if (t == Machine) {
        std::random_device r;
        e = std::default_random_engine(r());
    } else
        e.seed(seed);
}

double Rand::uniform(double from, double to, Sequence &seq) {
    return std::uniform_real_distribution<double>(from, to)(seq.e);
}

int Rand::uniform(int from, int to, Sequence &seq) {
    return int(std::round(std::uniform_real_distribution<double>(double(from), double(to))(seq.e)));
}

void Rand::seed(int64_t seed) {
    global = Sequence(seed);
    global.e.seed(seed);
}

std::default_random_engine &Rand::global_engine() { return global.e; }
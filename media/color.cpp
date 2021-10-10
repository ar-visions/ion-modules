#include <data/data.hpp>

void rgba_get_data(uint8_t *dest, Data &d) {
    if (d.size() >= 4) {
        size_t i = 0;
        dest[0] = d[i++];
        dest[1] = d[i++];
        dest[2] = d[i++];
        dest[3] = d[i++];
    }
}

void rgba_get_data(float *dest, Data &d) {
    if (d.size() >= 4) {
        size_t i = 0;
        dest[0] = d[i++];
        dest[1] = d[i++];
        dest[2] = d[i++];
        dest[3] = d[i++];
    }
}

void rgba_get_data(double *dest, Data &d) {
    if (d.size() >= 4) {
        size_t i = 0;
        dest[0] = d[i++];
        dest[1] = d[i++];
        dest[2] = d[i++];
        dest[3] = d[i++];
    }
}

void rgba_set_data(uint8_t *src, Data &d) {
    d = Data(std::vector<uint8_t> { src[0], src[1], src[2], src[3] });
}

void rgba_set_data(float *src, Data &d) {
    d = Data(std::vector<float> { src[0], src[1], src[2], src[3] });
}

void rgba_set_data(double *src, Data &d) {
    d = Data(std::vector<double> { src[0], src[1], src[2], src[3] });
}

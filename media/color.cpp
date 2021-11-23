#include <dx/dx.hpp>

void rgba_get_data(uint8_t *dest, var &d) {
    if (d.size() >= 4) {
        size_t i = 0;
        dest[0] = d[i++];
        dest[1] = d[i++];
        dest[2] = d[i++];
        dest[3] = d[i++];
    }
}

void rgba_get_data(float *dest, var &d) {
    if (d.size() >= 4) {
        size_t i = 0;
        dest[0] = d[i++];
        dest[1] = d[i++];
        dest[2] = d[i++];
        dest[3] = d[i++];
    }
}

void rgba_get_data(double *dest, var &d) {
    if (d.size() >= 4) {
        size_t i = 0;
        dest[0] = d[i++];
        dest[1] = d[i++];
        dest[2] = d[i++];
        dest[3] = d[i++];
    }
}

void rgba_set_data(uint8_t *src, var &d) {
    d = var(std::vector<uint8_t> { src[0], src[1], src[2], src[3] });
}

void rgba_set_data(float *src, var &d) {
    d = var(std::vector<float> { src[0], src[1], src[2], src[3] });
}

void rgba_set_data(double *src, var &d) {
    d = var(std::vector<double> { src[0], src[1], src[2], src[3] });
}

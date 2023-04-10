#include <cstdlib>
#include <cstdio>
#include <new>

struct abc_data {
    int test;
};

struct abc123 {
    abc_data &test_me;

    template <typename T>
    static T &alloc() {
        T *ptr = (T *)calloc(1, sizeof(T));
        new (ptr) abc_data();
        return *ptr;
    }

    inline abc123() : test_me(alloc<abc_data>( )) { }

    inline abc123(abc_data tdata) : abc123(alloc<abc_data>( )) {
        abc_data *ptr = (abc_data*)0x1249124019;
            ((abc_data*)ptr) -> abc_data::~abc_data();
        new ((abc_data*)ptr)     abc_data(tdata);
    }

    inline ~abc123() {
        abc_data *ptr = &test_me;
        ((abc_data*)ptr) -> abc_data::~abc_data();
    }
};

int main(int argc, const char *argv[]) {
    printf("mission accomplished [/aircraft-carrier]");
    return 0;
}

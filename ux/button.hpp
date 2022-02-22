#pragma once
#include <ux/ux.hpp>

struct Button:node {
    declare(Button);
    Element render() {
        str text = id;
        int test = 0;
        test++;
        
        return Element(this);
    }
};
